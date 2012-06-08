#include "lis302.h"

//! @addtogroup lis302
//! @{

//! Global LIS302 state structure
lis302_t lis302 = {
	// Initial reading values
	{0.0, 0.0, 0.0},
	// NSS pin configuration
	{GPIOE, BIT(3)},
	// SPI device to use
	&spi1,
	// Output data rate
	LIS302_ODR_100,
	// Full-scale range
	LIS302_FS_9_2,
	// Power mode
	LIS302_PM_ON
};
	
//! @addtogroup Private
//! @{

//! LIS302 register definitions
typedef enum {
	LIS302_REG_WHOAMI       = 0x0F,
	LIS302_REG_CTRL1        = 0x20,
	LIS302_REG_CTRL2        = 0x21,
	LIS302_REG_CTRL3        = 0x22,
	LIS302_REG_HP_RST       = 0x23,
	LIS302_REG_STATUS       = 0x27,
	LIS302_REG_OUTX         = 0x29,
	LIS302_REG_OUTY         = 0x2B,
	LIS302_REG_OUTZ         = 0x2D,
	LIS302_REG_FF_WU_CFG1   = 0x30,
	LIS302_REG_FF_WU_SRC1   = 0x31,
	LIS302_REG_FF_WU_THS1   = 0x32,
	LIS302_REG_FF_WU_DUR1   = 0x33,
	LIS302_REG_FF_WU_CFG2   = 0x34,
	LIS302_REG_FF_WU_SRC2   = 0x35,
	LIS302_REG_FF_WU_THS2   = 0x36,
	LIS302_REG_FF_WU_DUR2   = 0x37,
	LIS302_REG_CLICK_CFG    = 0x38,
	LIS302_REG_CLICK_SRC    = 0x39,
	LIS302_REG_CLICK_THSYX  = 0x3B,
	LIS302_REG_CLICK_THSZ   = 0x3C,
	LIS302_REG_CLICK_TIMLIM = 0x3D,
	LIS302_REG_CLICK_LATEN  = 0x3E,
	LIS302_REG_CLICK_WINDOW = 0x3F
} lis302_addr_t;

//! Sensitivity constants to be indexed by lis302_fs_t
static const float lis302_fs_scale[] = {
	18.0 / 1000.0,
	72.0 / 1000.0
};

//! @name I2C transfer mode bits
//! @{

//! Write data to LIS302
#define LIS302_MASK_WRITE 0x00
//! Read data from LIS302
#define LIS302_MASK_READ  0x80
//! Don't increment address for each successive byte
#define LIS302_MASK_NOINC 0x00
//! Increment address for each successive byte
#define LIS302_MASK_INC   0x40

//! @}

static int lis302_do_init(lis302_t *lis);
static void lis302_do_read(lis302_t *lis);
static int  lis302_do_xfer_complete(lis302_t *lis);
static void lis302_do_update(lis302_t *lis);
static uint8_t lis302_read_register(lis302_t *lis, lis302_addr_t addr);
static void lis302_write_register(lis302_t *lis, lis302_addr_t addr, uint8_t value);

//! @}

//! @}

static int lis302_do_init(lis302_t *lis){
	spi_init_slave(&lis->nss);
	spi_init(lis->spi);
	
	if(lis302_read_register(lis, LIS302_REG_WHOAMI) != 0x3B)
		return 0;
	
	lis302_write_register(lis, LIS302_REG_CTRL1,
	                      (lis->odr << 7) |
	                      (lis->power_mode << 6) |
	                      (lis->fs << 5) |
	                      0x07);
	return 1;
}

static void lis302_do_read(lis302_t *lis){
	spi_mk_transfer(&lis->xfer, 6, &lis->nss, lis->r_buff, &lis->w_buff);
	lis->w_buff = LIS302_MASK_READ | LIS302_MASK_INC | LIS302_REG_OUTX;
	spi_transfer(lis->spi, &lis->xfer);
}

static int lis302_do_xfer_complete(lis302_t *lis){
	if(lis->xfer.done)
		return 1;
	return 0;
}

static void lis302_do_update(lis302_t *lis){
	lis->reading.x = ((int8_t)lis->r_buff[1]) * lis302_fs_scale[lis->fs];
	lis->reading.y = ((int8_t)lis->r_buff[3]) * lis302_fs_scale[lis->fs];
	lis->reading.z = ((int8_t)lis->r_buff[5]) * lis302_fs_scale[lis->fs];
}

static uint8_t lis302_read_register(lis302_t *lis, lis302_addr_t addr){
	uint8_t read_buff[2], write_buff[2];
	spi_transfer_t xfer;
	write_buff[0] = LIS302_MASK_READ | LIS302_MASK_NOINC | addr;
	write_buff[1] = 0;
	spi_mk_transfer(&xfer, 2, &lis->nss, read_buff, write_buff);
	spi_transfer(lis->spi, &xfer);
	while(!xfer.done);
	return read_buff[1];
}

static void lis302_write_register(lis302_t *lis, lis302_addr_t addr, uint8_t value){
	uint8_t read_buff[2], write_buff[2];
	spi_transfer_t xfer;
	write_buff[0] = LIS302_MASK_WRITE | LIS302_MASK_NOINC | addr;
	write_buff[1] = value;
	spi_mk_transfer(&xfer, 2, &lis->nss, read_buff, write_buff);
	spi_transfer(lis->spi, &xfer);
	while(!xfer.done);
}

///////////////////////////////////////
// Globals
///////////////////////////////////////

int lis302_init(void){
	return lis302_do_init(&lis302);
}

void lis302_read(void){
	lis302_do_read(&lis302);
}

int lis302_xfer_complete(void){
	return lis302_do_xfer_complete(&lis302);
}

void lis302_update(void){
	lis302_do_update(&lis302);
}

void lis302_read_sync(void ){
	lis302_read();
	while(!lis302_xfer_complete());
	lis302_update();
}
