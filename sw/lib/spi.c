/*!
 @file spi.c
 @brief Generic SPI driver implementation for STM32F4
 @author Ben Nahill <bnahill@gmail.com>
 */

#include "stm32f4xx_conf.h"
#include "spi.h"

/*!
 @addtogroup spi
 @{
 */

#if USE_SPI1
//! Configuration for SPI1
static const spi_config_t spi1_config = {
	// AF
	GPIO_AF_SPI1,
	// MISO
	{GPIOA, BIT(6), GPIO_PinSource6},
	// MOSI
	{GPIOA, BIT(7), GPIO_PinSource7},
	// SCLK
	{GPIOA, BIT(5), GPIO_PinSource5},
	// DMA RX Stream
	DMA2_Stream0,
	// DMA TX Stream
	DMA2_Stream3,
	// DMA Device
	DMA2,
	// DMA Channel
	DMA_Channel_3,
	// DMA RX TCIF
	DMA_IT_TCIF0,
	// DMA RX TC Flag
	DMA_FLAG_TCIF0,
	// DMA TX TC Flag
	DMA_FLAG_TCIF3,
	// DMA IRQ
	DMA2_Stream0_IRQn,
	// DMA Clock
	RCC_AHB1Periph_DMA2,
	// Clock Command
	RCC_APB2PeriphClockCmd,
	// SPI Clock
	RCC_APB2Periph_SPI1
};

//! State structure for SPI1
spi_t spi1 = {
	// SPI Device
	SPI1,
	// Transfer
	NULL,
	// Is Initialized
	0,
	// Config
	&spi1_config
};

//! DMA ISR for SPI1
#define SPI1_DMA_ISR DMA2_Stream0_IRQHandler
#endif

//! @addtogroup spi_priv Private
//! @{

/*!
 @brief Assign and start an SPI transfer
 @param spi The SPI device to use
 @param xfer The transfer to start
 */
void spi_run_xfer(spi_t *spi, spi_transfer_t *xfer);

/*!
 @brief DMA ISR -- Finish a SPI transfer and start another if queued
 @param spi The SPI device calling the ISR
 */
void spi_dma_isr(spi_t *spi);

//! @}

//! @}

void spi_init_slave(gpio_pin_t *pin){
	GPIO_InitTypeDef gpio_init_s;
	
	GPIO_StructInit(&gpio_init_s);
	
	gpio_init_s.GPIO_Speed = GPIO_Speed_100MHz;
	gpio_init_s.GPIO_Mode = GPIO_Mode_OUT;
	gpio_init_s.GPIO_OType = GPIO_OType_PP;
	gpio_init_s.GPIO_Pin = pin->pin;

	pin->gpio->BSRRL = pin->pin;
	
	GPIO_Init(pin->gpio, &gpio_init_s);
}

void spi_init(spi_t *spi){
	SPI_InitTypeDef  spi_init_s;
	GPIO_InitTypeDef gpio_init_s;
	NVIC_InitTypeDef nvic_init_s;
	DMA_InitTypeDef  dma_init_s;

	spi_config_t const * const conf = spi->config;
	if(spi->xfer != NULL)
		return;
	
	GPIO_StructInit(&gpio_init_s);
	SPI_StructInit(&spi_init_s);
	DMA_StructInit(&dma_init_s);
	
	__disable_irq();
	if(spi->is_init){
		__enable_irq();
		return;
	}

	// Enable clock for DMA and SPI
	RCC_AHB1PeriphClockCmd(conf->dma_clock, ENABLE);
	conf->clock_cmd(conf->clock, ENABLE);

	// Configure GPIOs
	gpio_init_s.GPIO_Speed = GPIO_Speed_100MHz;
	gpio_init_s.GPIO_Mode = GPIO_Mode_AF;
	gpio_init_s.GPIO_OType = GPIO_OType_PP;

	// SCLK
	GPIO_PinAFConfig(conf->sclk.gpio, conf->sclk.pinsrc, conf->af);
	gpio_init_s.GPIO_Pin = conf->sclk.pin;
	GPIO_Init(conf->sclk.gpio, &gpio_init_s);

	// MISO
	GPIO_PinAFConfig(conf->miso.gpio, conf->miso.pinsrc, conf->af);
	gpio_init_s.GPIO_Pin = conf->miso.pin;
	GPIO_Init(conf->miso.gpio, &gpio_init_s);
	
	// MOSI
	GPIO_PinAFConfig(conf->mosi.gpio, conf->mosi.pinsrc, conf->af);
	gpio_init_s.GPIO_Pin = conf->mosi.pin;
	GPIO_Init(conf->mosi.gpio, &gpio_init_s);

	DMA_DeInit(conf->dma_rx_stream);
	DMA_DeInit(conf->dma_tx_stream);

	// Configure DMA streams
	dma_init_s.DMA_Channel = conf->dma_channel;
	dma_init_s.DMA_PeripheralBaseAddr = (uint32_t) &spi->spi->DR;
	dma_init_s.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	dma_init_s.DMA_MemoryInc = DMA_MemoryInc_Enable;
	dma_init_s.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	dma_init_s.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	dma_init_s.DMA_Mode = DMA_Mode_Normal;
	dma_init_s.DMA_Priority = DMA_Priority_High;
	dma_init_s.DMA_FIFOMode = DMA_FIFOMode_Disable;
	dma_init_s.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	dma_init_s.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	dma_init_s.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	dma_init_s.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_Init(conf->dma_rx_stream, &dma_init_s);

	dma_init_s.DMA_DIR = DMA_DIR_MemoryToPeripheral;
	DMA_Init(conf->dma_tx_stream, &dma_init_s);


	// Configure SPI
	SPI_I2S_DeInit(spi->spi);
	spi_init_s.SPI_Mode = SPI_Mode_Master;
	spi_init_s.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	spi_init_s.SPI_DataSize = SPI_DataSize_8b;
	spi_init_s.SPI_CPOL = SPI_CPOL_High;
	spi_init_s.SPI_CPHA = SPI_CPHA_2Edge;
	spi_init_s.SPI_NSS = SPI_NSS_Soft;
	spi_init_s.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
	spi_init_s.SPI_FirstBit = SPI_FirstBit_MSB;
	spi_init_s.SPI_CRCPolynomial = 7;

	// Enable SPI
	SPI_Init(spi->spi, &spi_init_s);
	SPI_Cmd(spi->spi, ENABLE);

	SPI_DMACmd(spi->spi, SPI_DMAReq_Rx, ENABLE);
	SPI_DMACmd(spi->spi, SPI_DMAReq_Tx, ENABLE);

	// Configure DMA interrupts
	nvic_init_s.NVIC_IRQChannel = conf->dma_irq;
	nvic_init_s.NVIC_IRQChannelPreemptionPriority = 2;
	nvic_init_s.NVIC_IRQChannelSubPriority = 2;
	nvic_init_s.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic_init_s);

	// Only interrupt on RX complete for entire transaction
	DMA_ITConfig(conf->dma_rx_stream, DMA_IT_TC, ENABLE);

	spi->is_init = 1;
	__enable_irq();
}

void spi_run_xfer(spi_t *spi, spi_transfer_t *xfer){
	spi_config_t const * const conf = spi->config;

	spi->xfer = xfer;

	if(xfer->nss){
		xfer->nss->gpio->BSRRH = xfer->nss->pin;
	}

	conf->dma_rx_stream->M0AR = (uint32_t)xfer->read_buff;
	conf->dma_tx_stream->M0AR = (uint32_t)xfer->write_buff;
	conf->dma_rx_stream->NDTR = xfer->count;
	conf->dma_tx_stream->NDTR = xfer->count;


	conf->dma_rx_stream->CR |= DMA_SxCR_EN;
	conf->dma_tx_stream->CR |= DMA_SxCR_EN;
}

void spi_transfer(spi_t *spi, spi_transfer_t *RESTRICT xfer){
	spi_transfer_t *old_xfer;

	// Clear the 'done' flag for this chain of transfers
	for(old_xfer = xfer; old_xfer != NULL; old_xfer = old_xfer->next){
		old_xfer->done = 0;
	}
	__disable_irq();
	
	old_xfer = spi->xfer;
	if(old_xfer == xfer)
		while(1);
	if(old_xfer != NULL){
		for(;old_xfer->next != NULL; old_xfer = old_xfer->next){
			if(old_xfer == xfer)
				while(1);
		}
		old_xfer->next = xfer;
	} else {
		spi_run_xfer(spi, xfer);	
	}
	__enable_irq();
}

void spi_mk_transfer(spi_transfer_t * xfer,
                     uint8_t count,
                     gpio_pin_t const *nss,
                     uint8_t *read_buff,
                     uint8_t *write_buff){
	xfer->count = count;
	xfer->done = 0;
	xfer->next = NULL,
	xfer->nss = nss,
	xfer->read_buff = read_buff;
	xfer->write_buff = write_buff;
}

void spi_dma_isr(spi_t *spi){
	spi_config_t const * const conf = spi->config;
	if(DMA_GetITStatus(conf->dma_rx_stream, conf->dma_rx_tcif)){
		if(spi->xfer->nss){
			spi->xfer->nss->gpio->BSRRL = spi->xfer->nss->pin;
		}
		DMA_Cmd(conf->dma_tx_stream, DISABLE);
		DMA_Cmd(conf->dma_rx_stream, DISABLE);
		DMA_ClearFlag(conf->dma_rx_stream, conf->dma_rx_tc_flag);
		DMA_ClearFlag(conf->dma_tx_stream, conf->dma_tx_tc_flag);
		DMA_ClearITPendingBit(conf->dma_rx_stream, conf->dma_rx_tcif);

		spi->xfer->done = 1;
		if(spi->xfer->next){
			spi_run_xfer(spi, spi->xfer->next);
		} else {
			spi->xfer = NULL;
		}
	}

}

void SPI1_DMA_ISR(void){spi_dma_isr(&spi1);}

//void SPI2_DMA_ISR(void){spi_dma_isr(&spi2);}

