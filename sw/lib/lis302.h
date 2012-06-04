#include "stm32f4xx_conf.h"
#include "sensor_config.h"
#include "spi.h"

typedef enum {
	LIS302_ODR_100 = 0,
	LIS302_ODR_400 = 1
} lis302_odr_t;

typedef enum {
	LIS302_FS_2_3 = 0,
	LIS302_FS_9_2 = 1
} lis302_fs_t;

typedef enum {
	LIS302_PM_OFF = 0,
	LIS302_PM_ON = 1
} lis302_pm_t;

typedef struct {
	//! Reading in g
	euclidean3_t        reading;
	//! The slave CS pin to use
	gpio_pin_t          nss;
	//! The SPI peripheral to use
	spi_t               * const spi;
	//! The required data rate
	lis302_odr_t        odr;
	//! The full-scale range to use
	lis302_fs_t         fs;
	//! The power mode (on or off)
	lis302_pm_t         power_mode;
	//! A buffer to store a command to be sent
	uint8_t             w_buff;
	//! A buffer to read measurement registers
	uint8_t             r_buff[6];
	//! A transfer state buffer for asynchronous transfers
	spi_transfer_t      xfer;
} lis302_t;

/*!
 @brief Initialize the LIS302 device
 @return 1 if successful; 0 otherwise
 */
int lis302_init(void);

/*!
 @brief Start an asynchronous read of measurement registers
 @post Safe to poll lis302_xfer_complete()
 @sa lis302_read_sync()
 */
void lis302_read(void);

/*!
 @brief Check to see if a transfer is done
 @return 1 if complete; 0 otherwise
 @post Safe to call lis302_update()
 */
int lis302_xfer_complete(void);

/*!
 @brief Update the readings from a completed transfer
 @pre lis302_xfer_complete() returned 1
 */
void lis302_update(void);

/*!
 @brief Perform the whole sequence of read, wait, update
 */
void lis302_read_sync(void);
