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
 * @brief       Initialization parameters for the Micro Crystal RV-3028-C7
 *              external RTC
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@posteo.net>
 */


#include "rv_3028_c7.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    Default configuration parameters for the RV_3028_C7 driver
 * @{
 */
#ifndef RV_3028_C7_PARAM_I2C
#  define RV_3028_C7_PARAM_I2C              I2C_DEV(0)      /**< I2C bus to use */
#endif

#ifndef RV_3028_C7_PARAM_INT_PIN
#  define RV_3028_C7_PARAM_INT_PIN          (GPIO_UNDEF)    /**< GPIO connected to `/INT` pin */
#endif

#ifndef RV_3028_C7_PARAM_BACKUP_POWER
#  define RV_3028_C7_PARAM_BACKUP_POWER     false           /**< Whether back power (battery, super cap) is present */
#endif

#ifndef RV_3028_C7_PARAM_TRICKLE_CHARGER
#  define RV_3028_C7_PARAM_TRICKLE_CHARGER  false            /**< Whether to charge the backup power supply via the RTC */
#endif

#ifndef RV_3028_C7_PARAM_TRICKLE_CHARGE_RESISTOR
/**
 * @brief   Which resistor to use in the trickle charge circuit (if enabled)
 */
#  define RV_3028_C7_PARAM_TRICKLE_CHARGE_RESISTOR RV_3028_C7_TRICKLE_CHARGE_RESISTOR_3K
#endif

#ifndef RV_3028_C7_PARAMS
#  define RV_3028_C7_PARAMS \
    { \
        .bus = RV_3028_C7_PARAM_I2C, \
        .int_pin = RV_3028_C7_PARAM_INT_PIN, \
        .backup_power_present = RV_3028_C7_PARAM_BACKUP_POWER, \
        .enable_trickle_charger = RV_3028_C7_PARAM_TRICKLE_CHARGER, \
        .trickle_charge_resistor = RV_3028_C7_PARAM_TRICKLE_CHARGE_RESISTOR, \
    }
#endif /* RV_3028_C7_PARAMS */
/** @} */

/**
 * @brief   RV_3028_C7 configuration
 */
static const rv_3028_c7_params_t rv_3028_c7_params[] =
{
    RV_3028_C7_PARAMS
};

#ifdef __cplusplus
}
#endif

/** @} */
