/*
 * Copyright (C) 2015 Jan Wagner <mail@jwagner.eu>
 *               2015-2016 Freie Universität Berlin
 *               2019 Inria
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
 * @brief       CPU specific part of the GPIO NG API
 *
 * @note        This GPIO driver implementation supports only one pin to be
 *              defined as external interrupt.
 *
 * @author      Christian Kühling <kuehling@zedat.fu-berlin.de>
 * @author      Timo Ziegler <timo.ziegler@fu-berlin.de>
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 * @author      Jan Wagner <mail@jwagner.eu>
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 */

#ifndef GPIO_NG_ARCH_H
#define GPIO_NG_ARCH_H

#include <assert.h>

#include "cpu.h"
#include "periph_cpu.h"

#ifdef __cplusplus
extern "C" {
#endif
#ifndef DOXYGEN /* hide implementation specific details from Doxygen */

#define PORT_BIT            (1 << 5)
#define PIN_MASK            (0x1f)

/* Compatibility wrapper defines for nRF9160 */
#ifdef NRF_P0_S
#define NRF_P0 NRF_P0_S
#endif

#if defined(NRF_P1)
#define GPIO_PORT(num)      ((num) ? (gpio_port_t)NRF_P1 : (gpio_port_t)NRF_P0)
#define GPIO_PORT_NUM(port) ((port == (gpio_port_t)NRF_P1) ? 1 : 0)
#else
#define GPIO_PORT(num)      ((gpio_port_t)NRF_GPIO)
#define GPIO_PORT_NUM(port) 0
#endif

static inline uword_t gpio_ng_read(gpio_port_t port)
{
    NRF_GPIO_Type *p = (NRF_GPIO_Type *)port;
    return p->IN;
}

static inline void gpio_ng_set(gpio_port_t port, uword_t mask)
{
    NRF_GPIO_Type *p = (NRF_GPIO_Type *)port;
    p->OUTSET = mask;
}

static inline void gpio_ng_clear(gpio_port_t port, uword_t mask)
{
    NRF_GPIO_Type *p = (NRF_GPIO_Type *)port;
    p->OUTCLR = mask;
}

static inline void gpio_ng_toggle(gpio_port_t port, uword_t mask)
{
    NRF_GPIO_Type *p = (NRF_GPIO_Type *)port;
    p->OUT ^= mask;
}

static inline void gpio_ng_write(gpio_port_t port, uword_t value)
{
    NRF_GPIO_Type *p = (NRF_GPIO_Type *)port;
    p->OUT = value;
}

static inline gpio_port_t gpio_get_port(gpio_t pin)
{
#if defined(NRF_P1)
    return GPIO_PORT(pin >> 5);
#else
    return GPIO_PORT(0);
#endif
}

static inline uint8_t gpio_get_pin_num(gpio_t pin)
{
#if defined(NRF_P1)
    return pin & PIN_MASK;
#else
    return (uint8_t)pin;
#endif
}

#endif /* DOXYGEN */
#ifdef __cplusplus
}
#endif

#endif /* GPIO_NG_ARCH_H */
/** @} */
