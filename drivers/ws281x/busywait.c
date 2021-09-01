/*
 * Copyright 2021 Marian Buschsieweke
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_ws281x
 *
 * @{
 *
 * @file
 * @brief       Implementation of `ws281x_write_buffer()` using periph/gpio_ng and periph/busywait
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 *
 * @}
 */
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "architecture.h"
#include "irq.h"
#include "periph/busywait.h"
#include "periph/gpio_ng.h"
#include "ws281x.h"
#include "ws281x_constants.h"
#include "xtimer.h"

#define ENABLE_DEBUG 0
#include "debug.h"

void ws281x_write_buffer(ws281x_t *dev, const void *buf, size_t size)
{
    assert(dev);
    const uint8_t *pos = buf;
    const uint8_t *end = pos + size;
    gpio_port_t port = gpio_get_port(dev->params.pin);
    uword_t mask = 1UL << gpio_get_pin_num(dev->params.pin);
    busyticks_t wait_high_one = busyticks_from_ns(WS281X_T_DATA_ONE_NS);
    busyticks_t wait_high_zero = busyticks_from_ns(WS281X_T_DATA_ZERO_NS);
    busyticks_t wait_low = busyticks_from_ns(500);

    DEBUG("[ws281x] ticks one: %u, ticks zero: %u, ticks low: %u, mask = %x, port = %u\n",
          (unsigned)wait_high_one, (unsigned)wait_high_zero, (unsigned)wait_low,
          (unsigned)mask, (unsigned)GPIO_PORT_NUM(port));

    while (pos < end) {
        uint8_t cnt = 8;
        uint8_t data = *pos;
        while (cnt > 0) {
            if (data & 0x80) {
                gpio_ng_set(port, mask);
                busywait(wait_high_one);
            }
            else {
                gpio_ng_set(port, mask);
                busywait(wait_high_zero);
            }
            gpio_ng_clear(port, mask);
            busywait(wait_low);
            cnt--;
            data <<= 1;
        }
        pos++;
    }
}

int ws281x_init(ws281x_t *dev, const ws281x_params_t *params)
{
    if (!dev || !params || !params->buf) {
        return -EINVAL;
    }

    memset(dev, 0, sizeof(ws281x_t));
    dev->params = *params;

    gpio_port_t port = gpio_get_port(dev->params.pin);
    uint8_t pin = gpio_get_pin_num(dev->params.pin);

    gpio_conf_t conf = {
        .state = GPIO_OUTPUT_PUSH_PULL,
        .slew_rate = GPIO_SLEW_FASTEST,
        .initial_value = false,
        .drive_strength = GPIO_DRIVE_WEAKEST,
    };

    if (gpio_ng_init(port, pin, &conf)) {
        return -EIO;
    }

    return 0;
}
