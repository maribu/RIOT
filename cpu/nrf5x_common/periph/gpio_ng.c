/*
 * Copyright (C) 2015 Jan Wagner <mail@jwagner.eu>
 *               2015-2016 Freie Universität Berlin
 *               2019 Inria
 *               2021 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_nrf5x_common
 * @ingroup     drivers_periph_gpio_ng
 * @{
 *
 * @file
 * @brief       Low-level GPIO-NG IRQ driver implementation for the nRF5x MCU family
 *
 * @author      Christian Kühling <kuehling@zedat.fu-berlin.de>
 * @author      Timo Ziegler <timo.ziegler@fu-berlin.de>
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 * @author      Jan Wagner <mail@jwagner.eu>
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 *
 * @}
 */

#include <assert.h>
#include <errno.h>

#include "cpu.h"
#include "periph/gpio_ng.h"
#include "periph_cpu.h"
#include "periph_conf.h"


int gpio_ng_init(gpio_port_t port, uint8_t pin, gpio_conf_t *conf)
{
    if (conf->pull == GPIO_PULL_KEEP) {
        return -ENOTSUP;
    }
    /* Searching "Schmitt" in https://infocenter.nordicsemi.com/pdf/nRF52840_OPS_v0.5.pdf yields
     * no matches. Assuming no Schmitt trigger present on the nRF5x MCU.
     */
    conf->schmitt_trigger = false;

    uint32_t pin_cnf = conf->pull;
    switch (conf->state) {
    case GPIO_OUTPUT_PUSH_PULL:
        /* INPUT bit needs to be *CLEARED* in input mode, so set to disconnect input buffer */
        pin_cnf |= GPIO_PIN_CNF_DIR_Msk | GPIO_PIN_CNF_INPUT_Msk;
        if (conf->drive_strength) {
            pin_cnf |= GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos;
        }
        break;
    case GPIO_OUTPUT_OPEN_DRAIN:
        pin_cnf |= GPIO_PIN_CNF_DIR_Msk;
        if (conf->drive_strength) {
            pin_cnf |= GPIO_PIN_CNF_DRIVE_H0D1 << GPIO_PIN_CNF_DRIVE_Pos;
        }
        else {
            pin_cnf |= GPIO_PIN_CNF_DRIVE_S0D1 << GPIO_PIN_CNF_DRIVE_Pos;
        }
        break;
    case GPIO_INPUT:
        break;
    case GPIO_DISCONNECT:
    default:
        /* INPUT bit needs to be *CLEARED* in input mode, so set to disconnect input buffer */
        pin_cnf |= GPIO_PIN_CNF_INPUT_Msk;
        break;
    }

    NRF_GPIO_Type *p = (NRF_GPIO_Type *)port;
    if (conf->initial_value) {
        p->OUTSET = 1UL << pin;
    }
    else {
        p->OUTCLR = 1UL << pin;
    }
    p->PIN_CNF[pin] = pin_cnf;

    return 0;
}
