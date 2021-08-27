/*
 * Copyright (C) Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */
#ifdef MODULE_PERIPH_GPIO_NG_IRQ

#include "periph/gpio_ng/irq.h"
#include <stdint.h>

__attribute__((weak))
void gpio_ng_irq_off(gpio_port_t port, uint8_t pin)
{
    gpio_ng_irq_mask(port, pin);
}
#endif
