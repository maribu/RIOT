/*
 * Copyright (C) 2016 Freie Universität Berlin
 *               2017 OTA keys S.A.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup         cpu_stm32
 * @ingroup         drivers_periph_gpio_ng
 * @{
 *
 * @file
 * @brief           CPU specific part of the GPIO NG API
 *
 * @author          Hauke Petersen <hauke.petersen@fu-berlin.de>
 * @author          Vincent Dupont <vincent@otakeys.com>
 */

#ifndef GPIO_NG_ARCH_H
#define GPIO_NG_ARCH_H

#include "architecture.h"
#include "periph/gpio_ng.h"
#include "periph_cpu.h"

#ifdef __cplusplus
extern "C" {
#endif
#ifndef DOXYGEN /* hide implementation specific details from Doxygen */

/**
 * @brief   Get a GPIO port by number
 */
#if defined(CPU_FAM_STM32MP1)
#define GPIO_PORT(num)      (GPIOA_BASE + ((num) << 12))
#else
#define GPIO_PORT(num)      (GPIOA_BASE + ((num) << 10))
#endif

/**
 * @brief   Get a GPIO port number by gpio_t value
 */
#if defined(CPU_FAM_STM32MP1)
#define GPIO_PORT_NUM(port) (((port) - GPIOA_BASE) >> 12)
#else
#define GPIO_PORT_NUM(port) (((port) - GPIOA_BASE) >> 10)
#endif

static inline uword_t gpio_ng_read(gpio_port_t port)
{
    GPIO_TypeDef *p = (GPIO_TypeDef *)port;
#ifdef CPU_CORE_CORTEX_M7
    /* avoid dual-issuing of two accesses to the GPIO bus by adding a nop */
    __asm__ volatile ("nop\n" : /* no outputs */ : /* no inputs */ : /* no clobber */);
#endif
    return p->IDR;
}

static inline void gpio_ng_set(gpio_port_t port, uword_t mask)
{
    GPIO_TypeDef *p = (GPIO_TypeDef *)port;
#ifdef CPU_CORE_CORTEX_M7
    /* avoid dual-issuing of two accesses to the GPIO bus by adding a nop */
    __asm__ volatile ("nop\n" : /* no outputs */ : /* no inputs */ : /* no clobber */);
#endif
    p->BSRR = mask;
}

static inline void gpio_ng_clear(gpio_port_t port, uword_t mask)
{
    GPIO_TypeDef *p = (GPIO_TypeDef *)port;
#ifdef CPU_CORE_CORTEX_M7
    /* avoid dual-issuing of two accesses to the GPIO bus by adding a nop */
    __asm__ volatile ("nop\n" : /* no outputs */ : /* no inputs */ : /* no clobber */);
#endif
    p->BSRR = mask << 16;
}

static inline void gpio_ng_toggle(gpio_port_t port, uword_t mask)
{
    GPIO_TypeDef *p = (GPIO_TypeDef *)port;
#ifdef CPU_CORE_CORTEX_M7
    /* avoid dual-issuing of two accesses to the GPIO bus by adding a nop */
    __asm__ volatile ("nop\n" : /* no outputs */ : /* no inputs */ : /* no clobber */);
#endif
    unsigned irq_state = irq_disable();
    p->ODR ^= mask;
    irq_restore(irq_state);
}

static inline void gpio_ng_write(gpio_port_t port, uword_t value)
{
    GPIO_TypeDef *p = (GPIO_TypeDef *)port;
#ifdef CPU_CORE_CORTEX_M7
    /* avoid dual-issuing of two accesses to the GPIO bus by adding a nop */
    __asm__ volatile ("nop\n" : /* no outputs */ : /* no inputs */ : /* no clobber */);
#endif
    p->ODR = value;
}

static inline gpio_port_t gpio_get_port(gpio_t pin)
{
    return pin & 0xfffffff0LU;
}

static inline uint8_t gpio_get_pin_num(gpio_t pin)
{
    return pin & 0xfLU;
}
#endif /* DOXYGEN */
#ifdef __cplusplus
}
#endif

#endif /* GPIO_NG_ARCH_H */
/** @} */
