/*
 * Copyright (C) 2021 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @defgroup    drivers_periph_busywait     Helpers for Highly Accurate Busy Waiting
 * @ingroup     drivers_periph
 *
 * This API provides a busy wait function that should achieve delays with an accuracy that is
 * measured in nanoseconds.
 *
 * @{
 *
 * @file
 * @brief       Helpers for Highly Accurate Busy Waiting
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 */

#ifndef BUSYWAIT_H
#define BUSYWAIT_H

#include <stdint.h>

#include "board.h"
#include "periph_cpu.h"
#include "timex.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(HAVE_BUSYTICKS_T) || defined(DOXYGEN)
/**
 * @brief   An integer type that can hold at least 100 microconds worth of ticks
 */
typedef unsigned busyticks_t;
#endif

#if !defined(HAVE_BUSYTICKS_FROM_NS) || defined(DOXYGEN)
/**
 * @brief   Convert a duration in nanoseconds to busyticks
 *
 * Often ticks are CPU cycles, but the implementation is allowed to choose whatever is convenient
 */
static inline busyticks_t busyticks_from_ns(uint16_t nanosecs)
{
    return ((uint64_t)nanosecs * CLOCK_CORECLOCK + NS_PER_SEC / 2) / NS_PER_SEC;
}
#endif

/**
 * @brief   Wait for the given duration
 * @param   ticks   Number of ticks to wait for
 *
 * @note    Use @ref busyticks_from_ns to obtain the correct wait duration
 */
static inline void busywait(busyticks_t ticks);

#ifdef __cplusplus
}
#endif

/* busywait_arch.h needs types to be defined */
#include "busywait_arch.h"

#endif /* BUSYWAIT_H */
/** @} */
