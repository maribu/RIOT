/*
 * SPDX-FileCopyrightText: 2025 ML!PA Consulting GmbH
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#pragma once

/**
 * @ingroup     drivers_rv_3028_c7
 * @brief       Driver for the Micro Crystal RV-3028-C7 external RTC
 *
 * @{
 * @file
 * @brief       Interface definition for the Micro Crystal RV-3028-C7 external
 *              RTC
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@posteo.net>
 */

#include <time.h>

#include "bitarithm.h"
#include "periph/gpio.h"
#include "periph/i2c.h"
#include "periph/rtc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   I²C address of the RV-3028-C7 RTC
 */
#define RV_3028_C7_I2C_ADDR         0x52

/**
 * @name    RV-3028-C7 status register bits
 *
 * Except for @ref RC_3028_C7_STATUS_EE_BUSY all flags need to be cleared in
 * software.
 * @{
 */
#define RC_3028_C7_STATUS_EE_BUSY               BIT7    /**< EEPROM is written to when set */
#define RC_3028_C7_STATUS_CLK_FLAG              BIT6    /**< A clock output interrupt has occurred */
#define RC_3028_C7_STATUS_BACKUP_SWITCH_FLAG    BIT5    /**< RTC has been operating from back supply */
#define RC_3028_C7_STATUS_TIME_UPDATE_FLAG      BIT4    /**< A time update interrupt has occurred */
#define RC_3028_C7_STATUS_COUNTDOWN_FLAG        BIT3    /**< A periodic timer interrupt has */
#define RC_3028_C7_STATUS_ALARAM_FLAG           BIT2    /**< An alarm interrupt has occurred */
#define RC_3028_C7_STATUS_EVENT_FLAG            BIT1    /**< An external event interrupt has occurred */
#define RC_3028_C7_STATUS_POR_FLAG              BIT0    /**< A power-on reset has occurred */
/** @} */

/**
 * @brief   Different options for the resistor to use in the trickle charge circuit
 */
typedef enum {
    RV_3028_C7_TRICKLE_CHARGE_RESISTOR_3K = 0x0,        /**< Use a 3 kOhm resistor for  trickle charging */
    RV_3028_C7_TRICKLE_CHARGE_RESISTOR_5K = 0x1,        /**< Use a 5 kOhm resistor for  trickle charging */
    RV_3028_C7_TRICKLE_CHARGE_RESISTOR_9K = 0x2,        /**< Use a 9 kOhm resistor for  trickle charging */
    RV_3028_C7_TRICKLE_CHARGE_RESISTOR_15K = 0x3,       /**< Use a 15 kOhm resistor for  trickle charging */
} rv_3028_c7_trickle_charge_resistor_t;

/**
 * @brief   Configuration parameters for an RV-3028-C7 RTC
 */
typedef struct {
    i2c_t bus;                          /**< I2C bus the device is connected to */
    gpio_t int_pin;                     /**< GPIO the interrupt pin of the RTC is connected to */
    rv_3028_c7_trickle_charge_resistor_t trickle_charge_resistor; /**< resistor to use in the trickle charge circuit */
    /**
     * @brief   A backup power supplu (e.g. a battery or a super capacitor)
     *          is present
     *
     * If enabled, the RTC will switch to the backup power supply on power loss
     * to keep ticking when the MCU is unpowered.
     */
    bool backup_power_present       : 1;
    /**
     * @brief   Enable the trickle charger
     *
     * Only set, if the backup power supply is rechargeable (e.g. a super cap)
     * to prevent damage. If set, the super cap will be charged via the RTC
     * while the main power supply is present.
     */
    bool enable_trickle_charger     : 1;
    bool int_pin_internal_pull_up   : 1; /**< Whether to use an internal pull up */
} rv_3028_c7_params_t;

/**
 * @brief   RV-3028-C7 device handle
 */
typedef struct {
    const rv_3028_c7_params_t *params;  /**< Configuration of the RV-3028-C7 */
#if MODULE_RV_3028_C7_ALARM || DOXYGEN
    rtc_alarm_cb_t alarm_cb;            /**< Function to call on alarm */
    void *alarm_cb_arg;                 /**< Argument to pass to the callback */
#endif
} rv_3028_c7_t;

/**
 * @brief   Initialize the given RV-3028-C7 RTC
 *
 * @param       dev     device descriptor of the RTC
 * @param[in]   params  RTC configuration
 *
 * @retval      0       RTC successfully initialized
 * @retval      <0      Initialization failed; returned value is a negative
 *                      `errno` constant hinting the cause
 */
int rv_3028_c7_init(rv_3028_c7_t *dev, const rv_3028_c7_params_t *params);

/**
 * @brief   Get the current status of the RV-3028-C7 RTC
 *
 * @param       dev     device descriptor of the RTC
 * @param[out]  flags   write status flags here
 *
 * @retval      0       Status successfully retrieved
 * @retval      <0      Failed to retrieve the status
 */
int rv_3028_c7_get_status(rv_3028_c7_t *dev, uint8_t *flags);

/**
 * @brief   Clear the currently set flags from the status register
 *
 * @param       dev     device descriptor of the RTC
 *
 * @note    The EEPROM busy flag will not be cleared; it is controlled by
 *          hardware and clears when the current EEPROM transfer completes.
 *
 * @retval      0       All flags from the status register successfully cleared
 * @retval      <0      Failed to clear the status flags
 */
int rv_3028_c7_clear_status(rv_3028_c7_t *dev);

/**
 * @brief       Get the current value of the UNIX time register
 * @param       dev     device descriptor of the RTC
 * @param[out]  dest    store the time read from the RTC in here
 *
 * @warning This value has no inherent correlation to the time returned by
 *          @ref rv_3028_c7_get_time; an application needs to carefully set them
 *          both to different representation of the same time if it desires them
 *          to match.
 *
 * @retval      0       Successfully read current UNIX time from RTC
 * @retval      <0      Failed to retrieve UNIX time
 */
int rv_3028_c7_get_time_unix(rv_3028_c7_t *dev, uint32_t *dest);

/**
 * @brief   Set the current value of the UNIX time register
 * @param       dev     device descriptor of the RTC
 * @param[in]   target  target value to set the UNIT time to
 *
 * @warning The values in the year, month, day, hour, and second registers will
 *          not be changed by this.
 *
 * @retval      0       Successfully updated the UNIX time to @p target
 * @retval      <0      Failed to update the UNIX time
 */
int rv_3028_c7_set_time_unix(rv_3028_c7_t *dev, uint32_t target);

/**
 * @brief       Get the current date/time from the calendar time registers
  *
 * @param       dev     device descriptor of the RTC
 * @param[out]  dest    store the time read from the RTC in here
 *
 * @retval      0       Successfully read current calendar time from RTC
 * @retval      <0      Failed to retrieve calendar time
 */
int rv_3028_c7_get_time(rv_3028_c7_t *dev, struct tm *dest);

/**
 * @brief       Update the current date/time in the calendar time registers
  *
 * @param           dev     device descriptor of the RTC
 * @param[in,out]   target  time to normalize and store
 *
 * @warning     This will nomalize the value in @p target in-place
 *
 * @retval      0       Successfully updated current calendar time in RTC
 * @retval      <0      Failed to update calendar time
 */
int rv_3028_c7_set_time(rv_3028_c7_t *dev, struct tm *target);

#ifdef __cplusplus
}
#endif

/** @} */
