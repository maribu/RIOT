/*
 * Copyright (C)  2024 Marian Buschsieweke
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     boards_olimexino-stm32f3
 * @{
 *
 * @file
 * @brief       Mapping from MCU pins to Arduino pins
 *
 * You can use the defines in this file for simplified interaction with the
 * Arduino specific pin numbers.
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@posteo.net>
 */

#ifndef ARDUINO_IOMAP_H
#define ARDUINO_IOMAP_H

#include "periph/adc.h"
#include "periph/gpio.h"
#include "periph/i2c.h"
#include "periph/pwm.h"
#include "periph/spi.h"
#include "periph/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    Arduino's UART devices
 * @{
 */
#define ARDUINO_UART_D0D1       UART_DEV(0)
/** @} */

/**
 * @name    Arduino's SPI buses
 * @{
 */
/**
 * @brief   SPI_DEV(0) is connected to the ISP header
 */
#define ARDUINO_SPI_ISP         SPI_DEV(0)
/** @} */

/**
 * @name    Arduino's I2C buses
 * @{
 */
/**
 * @brief   The first I2C bus is next to the AREF pin
 */
#define ARDUINO_I2C_UNO         I2C_DEV(0)
/** @} */

/**
 * @name    Mapping of MCU pins to Arduino pins
 * @{
 */
#define ARDUINO_PIN_0           GPIO_PIN(PORT_A, 3)
#define ARDUINO_PIN_1           GPIO_PIN(PORT_A, 2)
#define ARDUINO_PIN_2           GPIO_PIN(PORT_A, 0)
#define ARDUINO_PIN_3           GPIO_PIN(PORT_A, 1)
#define ARDUINO_PIN_4           GPIO_PIN(PORT_B, 5)
#define ARDUINO_PIN_5           GPIO_PIN(PORT_B, 6)
#define ARDUINO_PIN_6           GPIO_PIN(PORT_A, 8)
#define ARDUINO_PIN_7           GPIO_PIN(PORT_A, 9)

#define ARDUINO_PIN_8           GPIO_PIN(PORT_A, 10)
#define ARDUINO_PIN_9           GPIO_PIN(PORT_B, 7)
#define ARDUINO_PIN_10          GPIO_PIN(PORT_A, 4)
#define ARDUINO_PIN_11          GPIO_PIN(PORT_A, 7)
#define ARDUINO_PIN_12          GPIO_PIN(PORT_A, 6)
#define ARDUINO_PIN_13          GPIO_PIN(PORT_A, 5)
#define ARDUINO_PIN_14          GPIO_PIN(PORT_B, 8)

/* analog pins as digital pin: */
#define ARDUINO_PIN_15          GPIO_PIN(PORT_C, 0)
#define ARDUINO_PIN_16          GPIO_PIN(PORT_C, 1)
#define ARDUINO_PIN_17          GPIO_PIN(PORT_C, 2)
#define ARDUINO_PIN_18          GPIO_PIN(PORT_C, 3)
#define ARDUINO_PIN_19          GPIO_PIN(PORT_C, 4)
#define ARDUINO_PIN_20          GPIO_PIN(PORT_C, 5)

#define ARDUINO_PIN_LAST        20
/** @} */

/**
 * @name    Aliases for analog pins
 * @{
 */
#define ARDUINO_PIN_A0          ARDUINO_PIN_15
#define ARDUINO_PIN_A1          ARDUINO_PIN_16
#define ARDUINO_PIN_A2          ARDUINO_PIN_17
#define ARDUINO_PIN_A3          ARDUINO_PIN_18
#define ARDUINO_PIN_A4          ARDUINO_PIN_19
#define ARDUINO_PIN_A5          ARDUINO_PIN_20
/** @} */

/**
 * @name    Mapping of Arduino analog pins to RIOT ADC lines
 * @{
 */
#define ARDUINO_A0              ADC_LINE(0)
#define ARDUINO_A1              ADC_LINE(1)
#define ARDUINO_A2              ADC_LINE(2)
#define ARDUINO_A3              ADC_LINE(3)
#define ARDUINO_A4              ADC_LINE(4)
#define ARDUINO_A5              ADC_LINE(5)

#define ARDUINO_ANALOG_PIN_LAST 5
/** @} */

/**
 * @name    Arduino's SPI buses
 * @{
 */
/**
 * @brief   SPI_DEV(0) is connected to D11/D12/D13
 */
#define ARDUINO_SPI_D11D12D13   SPI_DEV(0)
/** @} */

/**
 * @name    UEXT Connector
 * @{
 */
#define UEXT0_PIN_3             GPIO_PIN(PORT_B, 10)    /* TXD */
#define UEXT0_PIN_4             GPIO_PIN(PORT_B, 11)    /* RXD */
#define UEXT0_PIN_5             GPIO_PIN(PORT_A, 9)     /* SCL */
#define UEXT0_PIN_6             GPIO_PIN(PORT_A, 10)    /* SDA */
#define UEXT0_PIN_7             GPIO_PIN(PORT_A, 6)     /* MISO */
#define UEXT0_PIN_8             GPIO_PIN(PORT_A, 7)     /* MOSI */
#define UEXT0_PIN_9             GPIO_PIN(PORT_A, 5)     /* SCK */
#define UEXT0_PIN_10            GPIO_PIN(PORT_B, 5)     /* /CS */
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ARDUINO_IOMAP_H */
/** @} */
