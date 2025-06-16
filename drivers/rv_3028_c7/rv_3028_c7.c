/*
 * SPDX-FileCopyrightText: 2025 ML!PA Consulting GmbH
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @ingroup     drivers_rv_3028_c7
 * @{
 *
 * @file
 * @brief       Micro Crystal RV-3028-C7 RTC driver
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@posteo.net>
 *
 * @}
 */

#include <assert.h>
#include <string.h>

#include "bcd.h"
#include "bitarithm.h"
#include "endian.h"
#include "modules.h"
#include "periph/gpio.h"
#include "rtc_utils.h"
#include "rv_3028_c7.h"
#include "tiny_strerror.h"

#define ENABLE_DEBUG        0
#include "debug.h"

/* configuration switch to enable wasting CPU cycles and ROM for correctly
 * setting exotic `tm_yday` member in rv_3028_c7_get_time */
#ifndef RTC_NORMALIZE_COMPAT
#  define RTC_NORMALIZE_COMPAT 0
#endif

static const uint8_t _addr = RV_3028_C7_I2C_ADDR; /**< shorter alias to safe typing */

/**
 * @brief   Register addresses used in this driver
 *
 * @note    Using an `enum` instead of macros allows accessing them from GDB
 *          but generates just as efficient code.
 */
enum rv_c3028_c7_regs {
    _REG_SECONDS = 0x00,            /**< seconds of current calendar time */
    _REG_MINUTES = 0x01,            /**< minutes of current calendar time */
    _REG_HOURS = 0x02,              /**< hours of current calendar time */
    _REG_WEEKDAY = 0x04,            /**< weekday of current calendar time, 0 being first day in week */
    _REG_DAY = 0x04,                /**< day in month of current calendar time, 1 being 1st day of month */
    _REG_MONTH = 0x05,              /**< month of current calendar time, 1 being January */
    _REG_YEAR = 0x06,               /**< two last digits of year in current calendar time, 0 being year 2000 */
    _REG_STATUS = 0x0e,             /**< status register of the RTC */
    _REG_CONTROL1 = 0x0f,           /**< first control register of the RTC */
    _REG_CONTROL2 = 0x10,           /**< second control register of the RTC */
    _REG_GP_BITS = 0x11,            /**< the 7 lsb bits in here can be freely used by the application */
    _REG_CLK_IRQ_MASK = 0x12,       /**< selects with IRQs are used for the clock output signal (if enabled) */
    _REG_EVENT_CONTROL = 0x13,      /**< controls event handling and timestamping */
    _REG_TIMESTAMP_COUNT = 0x14,    /**< number of timestamp-triggering events that have occurred */
    _REG_TIMESTAMP_SECONDS = 0x15,  /**< capture of _REG_SECONDS */
    _REG_TIMESTAMP_MINUTES = 0x16,  /**< capture of _REG_MINUTES */
    _REG_TIMESTAMP_HOURS = 0x17,    /**< capture of _REG_HOURS */
    _REG_TIMESTAMP_DAY = 0x18,      /**< capture of _REG_DAY */
    _REG_TIMESTAMP_MONTH = 0x19,    /**< capture of _REG_MONTH */
    _REG_TIMESTAMP_YEAR = 0x1a,     /**< capture of _REG_YEAR */
    _REG_UNIX1 = 0x1b,              /**< current UNIX time, lsb */
    _REG_UNIX2 = 0x1c,              /**< current UNIX time, 2nd lsb */
    _REG_UNIX3 = 0x1d,              /**< current UNIX time, 2nd msb */
    _REG_UNIX4 = 0x1e,              /**< current UNIX time, msb */
    _REG_USER_RAM1 = 0x1f,          /**< a full byte of back RAM the application can freely use */
    _REG_USER_RAM2 = 0x20,          /**< a full byte of back RAM the application can freely use */
    _REG_ID = 0x28,                 /**< 4 bit hardware ID (msb) + 4 bit version ID (lsb) */
    _REG_EEPROM_BACKUP = 0x37,      /**< control the switch to backup power supply; this configuration is synced with EEPROM */
};

/* RTC stores year since 2000, struct tm stores year since 1900 ==> offset is +100 */
#define YEAR_OFFSET 100

/* RTC stores January as 1, struct tm stores January as 0 ==> offset is -1 */
#define MONTH_OFFSET (-1)

/**
 * @name    Control register masks
 * @{
 */
#define CONTROL1_COUNTDOWN_REPEAT_ENABLE        BIT7 /**< Enable automatic restart of the countdown timer when fired */
#define CONTROL1_COUNTDOWN_REPEAT_DISABLE       0    /**< Stop the countdown timer when fired */
#define CONTROL1_ALARM_ON_WDAY                  0    /**< Alarm specifies the day in week, rather than the day in month */
#define CONTROL1_ALARM_ON_MDAY                  BIT5 /**< Alarm specifies the day in month, rather than the day in week */
#define CONTROL1_UPDATE_IRQ_EVERY_SECOND        0    /**< The time update IRQ should fire whenever the second counter updates */
#define CONTROL1_UPDATE_IRQ_EVERY_MINUTE        BIT4 /**< The time update IRQ should fire whenever the minute counter updates */
#define CONTROL1_EEPROM_REFRESH_ENABLE          0    /**< Every 24 hours at midnight the config registers are updated from EEPROM */
#define CONTROL1_EEPROM_REFRESH_DISABLE         BIT3 /**< Configuration register retain the value until power-on reset */
#define CONTROL1_COUNTDOWN_ENABLE               BIT2 /**< Enable the countdown timer */
#define CONTROL1_COUNTDOWN_DISABLE              0    /**< Disable the countdown timer */
#define CONTROL1_COUNTDOWN_FREQ_4096_HZ         0x0  /**< Run the countdown timer at 4 096 Hz */
#define CONTROL1_COUNTDOWN_FREQ_64_HZ           0x1  /**< Run the countdown timer at 64 Hz */
#define CONTROL1_COUNTDOWN_FREQ_1_HZ            0x2  /**< Run the countdown timer at 1 Hz */
#define CONTROL1_COUNTDOWN_FREQ_EVERY_MIN       0x3  /**< Run the countdown timer at (1/60) Hz */

#define CONTROL2_TIME_STAMP_ENABLE              BIT7 /**< Capture a timestamp into the timestamp registers on events */
#define CONTROL2_TIME_STAMP_DISABLE             0    /**< Capture a timestamp into the timestamp registers on events */
#define CONTROL2_CLKOUT_ON_IRQ_ENABLE           BIT6 /**< Pulse the clock output pin whenever the countdown IRQ fires */
#define CONTROL2_CLKOUT_ON_IRQ_DISABLE          0    /**< Pulse the clock output pin whenever the countdown IRQ fires */
#define CONTROL2_UPDATE_IRQ_ENABLE              BIT5 /**< Time update IRQ is propagated to the `/INT` pin */
#define CONTROL2_UPDATE_IRQ_DISABLE             0    /**< Time update IRQ is *NOT* propagated to the `/INT` pin */
#define CONTROL2_COUNTDOWN_IRQ_ENABLE           BIT4 /**< Countdown timer IRQ is propagated to the `/INT` pin */
#define CONTROL2_COUNTDOWN_IRQ_DISABLE          0    /**< Countdown timer IRQ is *NOT* propagated to the `/INT` pin */
#define CONTROL2_ALARM_IRQ_ENABLE               BIT3 /**< Alaram IRQ is propagated to the `/INT` pin */
#define CONTROL2_ALARM_IRQ_DISABLE              0    /**< Alaram IRQ is *NOT* propagated to the `/INT` pin */
#define CONTROL2_EVENT_IRQ_ENABLE               BIT2 /**< External event IRQ is propagated to the `/INT` pin */
#define CONTROL2_EVENT_IRQ_DISABLE              0    /**< External event IRQ is *NOT* propagated to the `/INT` pin */
#define CONTROL2_HOUR_MODE_24H                  0    /**< 24 hour format */
#define CONTROL2_HOUR_MODE_12H                  BIT1 /**< 12 hour format; not supported by this driver */
#define CONTROL2_RESET_PRESCALERS               BIT0 /**< Reset all prescalers; read the manual before using this */

#define BACKUP_EEOFSET_LSB                      BIT7 /**< LSB of the EE-Offset factory calibration register */
#define BACKUP_POWER_SWITCH_IRQ_ENABLE          BIT6 /**< propagate IRQ on switching to backup power to `/INT` */
#define BACKUP_POWER_SWITCH_IRQ_DISABLE         0    /**< do *NOT* propagate IRQ on switching to backup power to `/INT` */
#define BACKUP_TRICKLE_CHARGE_ENABLE            BIT5 /**< enable the trickle charger (e.g. for when a super capacitor is used instead of a battery) */
#define BACKUP_TRICKLE_CHARGE_DISABLE           0    /**< enable the trickle charger (e.g. for when a super capacitor is used instead of a battery) */
#define BACKUP_FAST_EDGE_DETECTION_ENABLE       BIT4 /**< enable detection on fast power loss */
#define BACKUP_FAST_EDGE_DETECTION_DISABLE      0    /**< risk failing to switch to backup power and brownout */
#define BACKUP_SWITCH_MODE_NEVER                0x0  /**< never switch to backup power supply, brown out instead */
#define BACKUP_SWITCH_MODE_HIGHER_VOLTAGE       0x4  /**< switch to the higher voltage, e.g. on 3.3 V VCC and 3.7 V battery, prefer the battery (Do'h!) */
#define BACKUP_SWITCH_MODE_HYSTERESIS           0xc  /**< switch to battery when VCC drops below 2.0 V with hysteresis enabled */
#define BACKUP_TRICKLE_CHARGE_RESISTOR_MASK     0x3  /**< to sanitze trickle charge configuration value */
/** @} */

/**
 * @brief   Structure following the memory layout of the current calendar time
 *          registers
 */
typedef struct __attribute__((packed)) {
    uint8_t seconds_bcd;
    uint8_t minutes_bcd;
    uint8_t hours_bcd;
    uint8_t weekday;
    uint8_t day_bcd;
    uint8_t month_bcd;
    uint8_t year_bcd;
} rv_c3028_cal_time_t;

static int _write_control_regs(const rv_3028_c7_params_t *params)
{
    const uint8_t control_regs[2] = {
        CONTROL1_COUNTDOWN_REPEAT_DISABLE |
            CONTROL1_ALARM_ON_MDAY |
            CONTROL1_UPDATE_IRQ_EVERY_SECOND |
            CONTROL1_EEPROM_REFRESH_DISABLE |
            CONTROL1_COUNTDOWN_DISABLE |
            CONTROL1_COUNTDOWN_FREQ_4096_HZ,
        CONTROL2_TIME_STAMP_DISABLE |
            CONTROL2_CLKOUT_ON_IRQ_DISABLE |
            CONTROL2_UPDATE_IRQ_DISABLE |
            CONTROL2_COUNTDOWN_IRQ_ENABLE |
            CONTROL2_ALARM_IRQ_ENABLE |
            CONTROL2_EVENT_IRQ_DISABLE |
            CONTROL2_HOUR_MODE_24H,
    };

    int retval = i2c_write_regs(params->bus, _addr, _REG_CONTROL1,
                            control_regs, sizeof(control_regs), 0);

    if (retval) {
        DEBUG("[rv_3028_c7] writing control regs failed: %s\n", tiny_strerror(retval));
        return retval;
    }

    uint8_t tmp[sizeof(control_regs)];
    retval = i2c_read_regs(params->bus, _addr, _REG_CONTROL1, tmp, sizeof(tmp), 0);

    if (retval) {
        DEBUG("[rv_3028_c7] reading control regs failed: %s\n", tiny_strerror(retval));
        return retval;
    }

    if (memcmp(control_regs, tmp, sizeof(control_regs)) != 0) {
        DEBUG_PUTS("[rv_3028_c7] updating control regs failed: read back differently");
        return -EIO;
    }

    return 0;
}

static int _write_backup_reg(const rv_3028_c7_params_t *params)
{
    uint8_t backup;
    int retval = i2c_read_reg(params->bus, _addr, _REG_EEPROM_BACKUP, &backup, 0);
    if (retval) {
        DEBUG("[rv_3028_c7] reading backup reg failed: %s\n", tiny_strerror(retval));
        return retval;
    }

    /* clear all bits except for the factory calibration, we don't want to
     * mess with that */
    backup &= BACKUP_EEOFSET_LSB;

    /* these configuing flags are not configurable: */
    backup |= BACKUP_POWER_SWITCH_IRQ_DISABLE | BACKUP_FAST_EDGE_DETECTION_ENABLE;

    /* apply trickle charge resistor configuration */
    backup |= (params->trickle_charge_resistor & BACKUP_TRICKLE_CHARGE_RESISTOR_MASK);

    /* enable trickle charge if specified */
    if (params->enable_trickle_charger) {
        backup |= BACKUP_TRICKLE_CHARGE_ENABLE;
    }

    /* enable switchover to backup power if enabled */
    if (params->backup_power_present) {
        backup |= BACKUP_SWITCH_MODE_HYSTERESIS;
    }

    retval = i2c_write_reg(params->bus, _addr, _REG_EEPROM_BACKUP, backup, 0);

    if (retval) {
        DEBUG("[rv_3028_c7] writing control regs failed: %s\n", tiny_strerror(retval));
        return retval;
    }

    uint8_t tmp;
    retval = i2c_read_reg(params->bus, _addr, _REG_EEPROM_BACKUP, &tmp, 0);
    if (retval) {
        DEBUG("[rv_3028_c7] reading backup reg failed: %s\n", tiny_strerror(retval));
        return retval;
    }

    if (tmp != backup) {
        DEBUG_PUTS("[rv_3028_c7] updating BACKUP register failed: incorrect value read back");
        return retval;
    }

    return 0;
}

static void _int_cb(void *arg)
{
    rv_3028_c7_t *dev = arg;
    (void)dev;
}

int rv_3028_c7_init(rv_3028_c7_t *dev, const rv_3028_c7_params_t *params)
{
    int retval;
    uint8_t device_id;
    assert(dev && params);
    *dev = (rv_3028_c7_t){
        .params = params,
    };

    i2c_acquire(params->bus);
    retval = i2c_read_reg(params->bus, _addr, _REG_ID, &device_id, 0);
    if (retval) {
        i2c_release(params->bus);
        DEBUG("[rv_3028_c7] reading ID failed: %s\n", tiny_strerror(retval));
        return retval;
    }
    DEBUG("[rv_3028_c7] Hardware ID = %#02x, Version ID = %#02x\n",
          (unsigned)(device_id >> 4), (unsigned)(device_id & 0xf));

    retval = _write_control_regs(params);
    if (retval) {
        i2c_release(params->bus);
        return retval;
    }

    retval = _write_backup_reg(params);
    if (retval) {
        i2c_release(params->bus);
        return retval;
    }

    i2c_release(params->bus);

    if (IS_USED(rv_3028_c7_alarm) && gpio_is_valid(params->int_pin)) {
        gpio_mode_t mode = params->int_pin_internal_pull_up ? GPIO_IN_PU : GPIO_IN;
        retval = gpio_init_int(params->int_pin, mode, GPIO_FALLING, _int_cb, dev);
        DEBUG("[rv_3028_c7] configuring GPIO IRQ failed: %s\n", tiny_strerror(retval));
        return retval;
    }

    return 0;
}

int rv_3028_c7_get_status(rv_3028_c7_t *dev, uint8_t *flags)
{
    assert(dev && flags);
    i2c_acquire(dev->params->bus);
    int retval = i2c_read_reg(dev->params->bus, _addr, _REG_STATUS, flags, 0);
    i2c_release(dev->params->bus);

    if (retval) {
        DEBUG("[rv_3028_c7] reading status failed: %s\n", tiny_strerror(retval));
    }

    return retval;
}

int rv_3028_c7_clear_status(rv_3028_c7_t *dev)
{
    assert(dev);
    i2c_acquire(dev->params->bus);
    int retval = i2c_write_reg(dev->params->bus, _addr, _REG_STATUS, 0, 0);
    i2c_release(dev->params->bus);

    if (retval) {
        DEBUG("[rv_3028_c7] clearing status flags failed: %s\n", tiny_strerror(retval));
    }

    return retval;
}

int rv_3028_c7_get_time_unix(rv_3028_c7_t *dev, uint32_t *dest)
{
    assert(dev && dest);
    uint32_t tmp;
    i2c_acquire(dev->params->bus);
    int retval = i2c_read_regs(dev->params->bus, _addr, _REG_UNIX1, &tmp, sizeof(tmp), 0);
    i2c_release(dev->params->bus);
    if (retval) {
        DEBUG("[rv_3028_c7] Failed to get UNIX time: %s\n", tiny_strerror(retval));
        return retval;
    }

    *dest = le32toh(tmp);
    return 0;
}

int rv_3028_c7_set_time_unix(rv_3028_c7_t *dev, uint32_t target)
{
    assert(dev);
    uint32_t tmp = htole32(target);
    i2c_acquire(dev->params->bus);
    int retval = i2c_write_regs(dev->params->bus, _addr, _REG_UNIX1, &tmp, sizeof(tmp), 0);
    i2c_release(dev->params->bus);

    if (retval) {
        DEBUG("[rv_3028_c7] Failed to set UNIX time: %s\n", tiny_strerror(retval));
    }

    return retval;
}

int rv_3028_c7_get_time(rv_3028_c7_t *dev, struct tm *dest)
{
    assert(dev && dest);
    rv_c3028_cal_time_t tmp = {};
    i2c_acquire(dev->params->bus);
    int retval = i2c_read_regs(dev->params->bus, _addr, _REG_SECONDS, &tmp, sizeof(tmp), 0);
    i2c_release(dev->params->bus);
    if (retval) {
        DEBUG("[rv_3028_c7] Failed to get calendar time: %s\n", tiny_strerror(retval));
        return retval;
    }

    *dest = (struct tm){
        .tm_sec = bcd_to_byte(tmp.seconds_bcd),
        .tm_min = bcd_to_byte(tmp.minutes_bcd),
        .tm_hour = bcd_to_byte(tmp.hours_bcd),
        .tm_wday = tmp.weekday,
        .tm_mday = bcd_to_byte(tmp.day_bcd),
        .tm_mon = bcd_to_byte(tmp.month_bcd) + MONTH_OFFSET,
        .tm_year = bcd_to_byte(tmp.year_bcd) + YEAR_OFFSET
    };

    /* Days since January the 1st in `tm_yday` is currently zero. Applications
     * can define RTC_NORMALIZE_COMPAT to `1` to waste CPU cycles and ROM to
     * have that set correctly. */
    if (RTC_NORMALIZE_COMPAT) {
        rtc_tm_normalize(dest);
    }

    return 0;
}

int rv_3028_c7_set_time(rv_3028_c7_t *dev, struct tm *target)
{
    assert(dev && target);
    rtc_tm_normalize(target);
    rv_c3028_cal_time_t tmp = {
        .seconds_bcd = bcd_from_byte(target->tm_sec),
        .minutes_bcd = bcd_from_byte(target->tm_min),
        .hours_bcd = bcd_from_byte(target->tm_hour),
        .weekday = target->tm_wday,
        .month_bcd = bcd_from_byte(target->tm_mon - MONTH_OFFSET),
        .year_bcd = bcd_from_byte(target->tm_year - YEAR_OFFSET),
    };
    i2c_acquire(dev->params->bus);
    int retval = i2c_write_regs(dev->params->bus, _addr, _REG_SECONDS, &tmp, sizeof(tmp), 0);
    i2c_release(dev->params->bus);

    if (retval) {
        DEBUG("[rv_3028_c7] Failed to set calendar time: %s\n", tiny_strerror(retval));
    }

    return retval;
}
