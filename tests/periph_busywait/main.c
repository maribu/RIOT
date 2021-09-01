/*
 * Copyright (C) 2021 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     tests
 * @{
 *
 * @file
 * @brief       Test application for the peripheral busywait API
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>

#include "periph/busywait.h"
#include "timex.h"
#include "xtimer.h"

static const uint32_t loops = 1000;

int main(void)
{
    int32_t diff_total = 0;
    int32_t iterations = 0;
    for (uint16_t duration = 100; duration <= 2 * NS_PER_US; duration += 100) {
        busyticks_t ticks = busyticks_from_ns(duration);
        if (ticks < BUSYWAIT_COMPENSATION) {
            continue;
        }
        uint32_t start = xtimer_now_usec();
        for (uint32_t i = 0; i < loops / 2; i++) {
            /* The conditional branch of the loop can cause the instruction pipeline to be flushed.
             * Having two busywait calls in the loop body should yield the average of the delay
             * with pipeline flush and without. */
            busywait(ticks);
            busywait(ticks);
        }
        uint32_t duration_actual = xtimer_now_usec() - start;
        uint32_t duration_expected = (uint32_t)duration * loops / NS_PER_US;
        printf("%" PRIu32 " durations of %" PRIu16 " ns took %" PRIu32" us\n",
               loops, duration, duration_actual);
        int32_t diff = (int32_t)duration_actual - (int32_t)duration_expected;
        diff = diff * (int32_t)NS_PER_US / (int32_t)loops;
        printf("Delays of %" PRIu16 " ns: Offset of %" PRIi32 " ns per wait\n", duration, diff);
        diff_total += diff;
        iterations++;
    }

    int32_t correction_ns = diff_total / iterations;
    int32_t correction_ticks = (correction_ns >= 0) ? busyticks_from_ns(correction_ns)
                                                    : -busyticks_from_ns(-correction_ns);
    printf("Adjust compensation by: %" PRIi32 " (= %" PRIi32 " ns)\n",
           correction_ticks, correction_ns);

    return 0;
}
