/*!
 @file spi.h
 @brief Public definitions for generic SPI driver for STM32F4
 @author Ben Nahill <bnahill@gmail.com>
 */

#ifndef __SPI_H_
#define __SPI_H_

#include "sensor_config.h"

/*!
 @addtogroup spi SPI
 @brief A simple driver for SPI master
 
 This SPI driver is intended for master-mode multi-slave operation offering
 asynchronous transfers.
 
 @section spi-sec-config Configuration
 Each SPI peripheral can be configured with a @ref spi_config_t (const
 configuration) structure assigned to a @ref spi_t (state) structure. This
 allows selection of each of the pins and DMA hardware to be used. The
 configuration is made statically in spi.c.
 
 @section spi-sec-init Initialization
 Each component requiring use of the SPI driver can call spi_init() to prepare
 the device. Each component must also call spi_init_slave() to initialize its
 slave-select (nSS) pin.
 
 @section spi-sec-transfers Transfers
 Transfers (@ref spi_transfer_t) are placed in a linked-list queue to
 serialize transfers without causing delays to callers. All linked-list
 operations are protected by disabling interrupts. When each transfer is
 completed, the 'done' flag is set to 1 and the next transfer (if any) is
 started.
 
 @{
 */

//! State structure for an SPI transfer
typedef struct _spi_transfer {
	//! The data from the slave
	uint8_t *read_buff;
	//! The data to be written to the slave
	uint8_t *write_buff;
	//! The NSS pin to use, may be null
	gpio_pin_t const * nss;
	//! The number of bytes to transfer
	uint16_t count;
	//! A flag to indicate completion
	uint8_t done;
	//! The next transfer in the queue
	struct _spi_transfer *next;
} spi_transfer_t;


//! @brief A set of hardware-related constants for each SPI device
typedef struct {
	//! Alternate function mode
	uint8_t af;
	//! MISO pin
	af_gpio_pin_t miso;
	//! MOSI pin
	af_gpio_pin_t mosi;
	//! SCLK pin
	af_gpio_pin_t sclk;
	//! Inbound DMA stream
	DMA_Stream_TypeDef *dma_rx_stream;
	//! Outbound DMA stream
	DMA_Stream_TypeDef *dma_tx_stream;
	//! DMA device
	DMA_TypeDef        *dma;
	//! DMA channel
	uint32_t dma_channel;
	//! DMA inbound transfer complete interrupt flag
	uint32_t dma_rx_tcif;
	//! DMA inbound transfer complete flag
	uint32_t dma_rx_tc_flag;
	//! DMA outbound transfer complete flag
	uint32_t dma_tx_tc_flag;
	//! DMA inbound IRQ
	IRQn_Type dma_irq;
	//! Clock for DMA
	uint32_t dma_clock;
	//! Command to enable the SPI clock
	void (*clock_cmd)(uint32_t, FunctionalState);
	//! Clock for SPI
	uint32_t clock;
} spi_config_t;


//! @brief The state and configuration of an SPI device
typedef struct {
	//! The SPI device
	SPI_TypeDef * const spi;
	//! The current transfer
	spi_transfer_t *xfer;
	//! A flag to indicate that the device is already configured
	uint8_t is_init;
	//! A pointer to the hardware configuration
	spi_config_t const * const config;
} spi_t;

/*!
 @brief Initialize a SPI peripheral
 @param spi The SPI peripheral to initialize
 */
void spi_init(spi_t *spi);

/*!
 @brief Configure a GPIO pin for use to control a slave device
 @param pin The pin to configure
 */
void spi_init_slave(gpio_pin_t *pin);

/*!
 @brief Begin a SPI transfer
 @param spi The SPI device to use
 @param xfer The transfer structure to use

 This will start a new SPI transfer for the device provided. If the device is
 busy, the transfer will be added to the transfer queue. To check for
 completion, the caller is expected to wait on the spi_transfer_t 'done' flag.
 */
void spi_transfer(spi_t *spi, spi_transfer_t *RESTRICT xfer);

/*!
 @brief Populate an SPI transfer structure
 @param xfer The transfer structure to populate
 @param count The number of bytes to transfer
 @param nss The slave select pin to use (can be NULL)
 @param read_buff The buffer to read to
 @param write_buff The buffer to transmit over SPI
 */
void spi_mk_transfer(spi_transfer_t *xfer,
                     uint8_t count,
                     gpio_pin_t const *nss,
                     uint8_t *read_buff,
                     uint8_t *write_buff);

//! @}

#if USE_SPI1
extern spi_t spi1;
#endif

#if USE_SPI2
extern spi_t spi2;
#endif


#endif

