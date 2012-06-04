/*!
 @file sensor_config.h
 @brief Common configuration to support some sensors and peripherals
 @author Ben Nahill <bnahill@gmail.com>
 */

#ifndef __SENSOR_CONFIG_H_
#define __SENSOR_CONFIG_H_

#include "toolchain.h"

#ifndef BIT
#define BIT(x) (1 << x)
#endif

#ifndef NULL
#define NULL ((void *) 0)
#endif

typedef const struct {
	GPIO_TypeDef *const gpio;
	uint16_t const pin;
	uint8_t const pinsrc;
} af_gpio_pin_t;

typedef struct {
	GPIO_TypeDef *const gpio;
	uint16_t const pin;
} gpio_pin_t;

typedef struct {
	float x;
	float y;
	float z;
} euclidean3_t;

#define USE_SPI1 1
#define USE_SPI2 0

#define USE_I2C1 1
#define USE_I2C2 0
#define USE_I2C3 0

#endif

