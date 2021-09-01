/*
 * Copyright (C) 2021 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     cpu_cortexm_common
 * @ingroup     drivers_periph_busywait
 * @{
 *
 * @file
 * @brief       Implementation of periph/busywait using the SysTick timer
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 * @}
 */

#ifndef BUSYWAIT_ARCH_H
#define BUSYWAIT_ARCH_H

#include "cpu_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Hide platform-specific implementation from Doxygen */
#ifndef DOXYGEN

#ifndef BUSYWAIT_COMPENSATION
#define BUSYWAIT_COMPENSATION 20
#endif

static inline void busywait(busyticks_t ticks)
{
    if (ticks > BUSYWAIT_COMPENSATION) {
        SysTick->LOAD = ticks - BUSYWAIT_COMPENSATION;
        SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
        /* Force reload */
        SysTick->VAL = 0;
        /* Next reload should not restart clock */
        SysTick->LOAD = 0;
        while (SysTick->VAL) { }
        SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk;
    }
}

#endif /* DOXYGEN */

#ifdef __cplusplus
}
#endif

#endif /* BUSYWAIT_ARCH_H */
/** @} */
