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
 * @brief       Test application for the peripheral GPIO NG API
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>

#include "mutex.h"
#include "periph/gpio.h"
#include "periph/gpio_ng.h"
#if MODULE_PERIPH_GPIO_NG_IRQ
#include "periph/gpio_ng/irq.h"
#endif
#include "test_utils/expect.h"
#include "xtimer.h"

static const char *noyes[] = { "no", "yes" };
static gpio_port_t port_out = GPIO_PORT(PORT_OUT);
static gpio_port_t port_in = GPIO_PORT(PORT_IN);
#if MODULE_PERIPH_GPIO_NG_IRQ
static const uint64_t mutex_timeout = US_PER_MS;
#endif

static void brief_delay(void)
{
    for (volatile unsigned i = 0; i < 4; i++) { }
}

static void test_gpio_ng_init(void)
{
    puts("\n"
         "Testing gpip_ng_init()\n"
         "======================\n");
    printf("Number of pull resistor values supported: %u\n", GPIO_PULL_NUMOF);
    printf("Number of drive strengths supported: %u\n", GPIO_DRIVE_NUMOF);
    printf("Number of slew rates supported: %u\n", GPIO_SLEW_NUMOF);
    bool is_supported;

    puts("\nTesting input configurations for PIN_IN_0:");
    {
        gpio_conf_t conf = {
            .state = GPIO_INPUT,
            .pull = GPIO_PULL_UP,
        };
        is_supported = (0 == gpio_ng_init(port_in, PIN_IN_0, &conf));
    }
    printf("Support for input with pull up: %s\n", noyes[is_supported]);

    {
        gpio_conf_t conf = {
            .state = GPIO_INPUT,
            .pull = GPIO_PULL_DOWN,
        };
        is_supported = (0 == gpio_ng_init(port_in, PIN_IN_0, &conf));
    }
    printf("Support for input with pull down: %s\n", noyes[is_supported]);

    {
        gpio_conf_t conf = {
            .state = GPIO_INPUT,
            .pull = GPIO_PULL_KEEP,
        };
        is_supported = (0 == gpio_ng_init(port_in, PIN_IN_0, &conf));
    }
    printf("Support for input with pull to bus level: %s\n", noyes[is_supported]);

    {
        gpio_conf_t conf = {
            .state = GPIO_INPUT,
            .pull = GPIO_FLOATING,
        };
        is_supported = (0 == gpio_ng_init(port_in, PIN_IN_0, &conf));
    }
    printf("Support for floating input (no pull resistors): %s\n", noyes[is_supported]);
    /* Support for floating inputs is mandatory */
    expect(is_supported);

    puts("\nTesting output configurations for PIN_OUT_0:");
    {
        gpio_conf_t conf = {
            .state = GPIO_OUTPUT_PUSH_PULL,
            .initial_value = false,
        };
        is_supported = (0 == gpio_ng_init(port_out, PIN_OUT_0, &conf));
    }
    printf("Support for output (push-pull) with initial value of LOW: %s\n", noyes[is_supported]);

    if (is_supported) {
        /* Give MCU some time to correctly read the input. Some MCUs (looking at nRF5x) take quite
         * some time until the GPIOs pick up a voltage change */
        brief_delay();
        is_supported = !(gpio_ng_read(port_in) & (1ULL << PIN_IN_0));
        printf("Output is indeed LOW: %s\n", noyes[is_supported]);
        expect(is_supported);
    }

    {
        gpio_conf_t conf = {
            .state = GPIO_OUTPUT_PUSH_PULL,
            .initial_value = true,
        };
        is_supported = (0 == gpio_ng_init(port_out, PIN_OUT_0, &conf));
    }
    printf("Support for output (push-pull) with initial value of HIGH: %s\n", noyes[is_supported]);

    if (is_supported) {
        brief_delay();
        is_supported = !!(gpio_ng_read(port_in) & (1ULL << PIN_IN_0));
        printf("Output is indeed HIGH: %s\n", noyes[is_supported]);
        expect(is_supported);
    }

    {
        gpio_conf_t conf = {
            .state = GPIO_OUTPUT_OPEN_DRAIN,
            .initial_value = false,
            .pull = GPIO_PULL_UP
        };
        is_supported = (0 == gpio_ng_init(port_out, PIN_OUT_0, &conf));
    }
    printf("Support for output (open drain with pull up) with initial value of LOW: %s\n",
           noyes[is_supported]);

    if (is_supported) {
        brief_delay();
        is_supported = !(gpio_ng_read(port_in) & (1ULL << PIN_IN_0));
        printf("Output is indeed LOW: %s\n", noyes[is_supported]);
        expect(is_supported);
    }

    {
        gpio_conf_t conf = {
            .state = GPIO_OUTPUT_OPEN_DRAIN,
            .initial_value = true,
            .pull = GPIO_PULL_UP
        };
        is_supported = (0 == gpio_ng_init(port_out, PIN_OUT_0, &conf));
    }
    printf("Support for output (open drain with pull up) with initial value of HIGH: %s\n",
           noyes[is_supported]);

    if (is_supported) {
        brief_delay();
        is_supported = !!(gpio_ng_read(port_in) & (1ULL << PIN_IN_0));
        printf("Output is indeed HIGH: %s\n", noyes[is_supported]);
        expect(is_supported);
    }

    {
        gpio_conf_t conf = {
            .state = GPIO_OUTPUT_OPEN_DRAIN,
            .initial_value = false,
        };
        is_supported = (0 == gpio_ng_init(port_out, PIN_OUT_0, &conf));
    }
    printf("Support for output (open drain) with initial value of LOW: %s\n", noyes[is_supported]);

    if (is_supported) {
        brief_delay();
        is_supported = !(gpio_ng_read(port_in) & (1ULL << PIN_IN_0));
        printf("Output is indeed LOW: %s\n", noyes[is_supported]);
        expect(is_supported);
    }

    {
        gpio_conf_t conf = {
            .state = GPIO_OUTPUT_OPEN_DRAIN,
            .initial_value = true,
        };
        is_supported = (0 == gpio_ng_init(port_out, PIN_OUT_0, &conf));
    }
    printf("Support for output (open drain) with initial value of HIGH: %s\n", noyes[is_supported]);

    if (is_supported) {
        {
            gpio_conf_t conf = {
                .state = GPIO_INPUT,
                .pull = GPIO_PULL_DOWN,
            };
            is_supported = (0 == gpio_ng_init(port_in, PIN_IN_0, &conf));
        }
        if (is_supported) {
            /* allow some time to pass before pin is actually pulled low */
            unsigned retries = 100;
            for (is_supported = false; !is_supported && (retries > 0); retries--) {
                is_supported = !(gpio_ng_read(port_in) & (1ULL << PIN_IN_0));
            }
            printf("Output can indeed be pulled LOW: %s\n", noyes[is_supported]);
            expect(is_supported);
        }
        else {
            puts("WARN: Cannot enable pull down of PIN_IN_0 to verify correct Open Drain behavior");
        }
        {
            gpio_conf_t conf = {
                .state = GPIO_INPUT,
                .pull = GPIO_PULL_UP,
            };
            is_supported = (0 == gpio_ng_init(port_in, PIN_IN_0, &conf));
        }
        if (is_supported) {
            /* allow some time to pass before pin is actually pulled low */
            unsigned retries = 100;
            for (is_supported = false; !is_supported && (retries > 0); retries--) {
                is_supported = !!(gpio_ng_read(port_in) & (1ULL << PIN_IN_0));
            }
            printf("Output can indeed be pulled HIGH: %s\n", noyes[is_supported]);
            expect(is_supported);
        }
        else {
            puts("WARN: Cannot enable pull up of PIN_IN_0 to verify correct Open Drain behavior");
        }
    }

    {
        gpio_conf_t conf = {
            .state = GPIO_DISCONNECT,
        };
        is_supported = (0 == gpio_ng_init(port_out, PIN_OUT_0, &conf));
    }
    printf("Support for disconnecting GPIO: %s\n", noyes[is_supported]);
    /* This is mandatory */
    expect(is_supported);
}

static void test_input_output(void)
{
    puts("\n"
         "Testing Reading/Writing GPIO Ports\n"
         "==================================\n");
    {
        gpio_conf_t input_conf = {
            .state = GPIO_INPUT,
        };
        gpio_conf_t output_conf = {
            .state = GPIO_OUTPUT_PUSH_PULL,
            .slew_rate = GPIO_SLEW_FASTEST,
            .initial_value = false,
        };
        expect(0 == gpio_ng_init(port_in, PIN_IN_0, &input_conf));
        expect(0 == gpio_ng_init(port_in, PIN_IN_1, &input_conf));
        expect(0 == gpio_ng_init(port_out, PIN_OUT_0, &output_conf));
        expect(0 == gpio_ng_init(port_out, PIN_OUT_1, &output_conf));
    }

    uword_t mask_in_0 = (1UL << PIN_IN_0);
    uword_t mask_in_1 = (1UL << PIN_IN_1);
    uword_t mask_in_both = mask_in_0 | mask_in_1;

    puts("testing initial value of 0 after init");
    brief_delay();
    expect(0x00 == (gpio_ng_read(port_in) & mask_in_both));
    puts("...OK");
    puts("testing setting both outputs simultaneously");
    gpio_ng_set(port_out, (1UL << PIN_OUT_0) | (1UL << PIN_OUT_1));
    brief_delay();
    expect(mask_in_both == (gpio_ng_read(port_in) & mask_in_both));
    puts("...OK");
    puts("testing clearing both outputs simultaneously");
    gpio_ng_clear(port_out, (1UL << PIN_OUT_0) | (1UL << PIN_OUT_1));
    brief_delay();
    expect(0x00 == (gpio_ng_read(port_in) & mask_in_both));
    puts("...OK");
    puts("testing toggling first output (0 --> 1)");
    gpio_ng_toggle(port_out, 1UL << PIN_OUT_0);
    brief_delay();
    expect(mask_in_0 == (gpio_ng_read(port_in) & mask_in_both));
    puts("...OK");
    puts("testing toggling first output (1 --> 0)");
    gpio_ng_toggle(port_out, 1UL << PIN_OUT_0);
    brief_delay();
    expect(0x00 == (gpio_ng_read(port_in) & mask_in_both));
    puts("...OK");
    puts("testing toggling second output (0 --> 1)");
    gpio_ng_toggle(port_out, 1UL << PIN_OUT_1);
    brief_delay();
    expect(mask_in_1 == (gpio_ng_read(port_in) & mask_in_both));
    puts("...OK");
    puts("testing toggling second output (1 --> 0)");
    gpio_ng_toggle(port_out, 1UL << PIN_OUT_1);
    brief_delay();
    expect(0x00 == (gpio_ng_read(port_in) & mask_in_both));
    puts("...OK");
    puts("testing setting first output and clearing second with write");
    gpio_ng_write(port_out, 1UL << PIN_OUT_0);
    brief_delay();
    expect(mask_in_0 == (gpio_ng_read(port_in) & mask_in_both));
    puts("...OK");
    puts("testing setting second output and clearing first with write");
    gpio_ng_write(port_out, 1UL << PIN_OUT_1);
    brief_delay();
    expect(mask_in_1 == (gpio_ng_read(port_in) & mask_in_both));
    puts("...OK");
    puts("All input/output operations worked as expected");
}

#if MODULE_PERIPH_GPIO_NG_IRQ
static void irq_edge_cb(void *mut)
{
    mutex_unlock(mut);
}

static void test_irq_edge(void)
{
    mutex_t irq_mut = MUTEX_INIT_LOCKED;

    puts("Testing rising edge on PIN_IN_0");
    expect(0 == gpio_ng_irq(port_in, PIN_IN_0, GPIO_TRIGGER_EDGE_RISING, irq_edge_cb, &irq_mut));
    /* test for spurious IRQ */
    expect(-1 == xtimer_mutex_lock_timeout(&irq_mut, mutex_timeout));
    gpio_ng_set(port_out, 1UL << PIN_OUT_0);
    /* test for IRQ on rising edge */
    expect(0 == xtimer_mutex_lock_timeout(&irq_mut, mutex_timeout));
    /* test for spurious IRQ */
    expect(-1 == xtimer_mutex_lock_timeout(&irq_mut, mutex_timeout));
    gpio_ng_clear(port_out, 1UL << PIN_OUT_0);
    /* test for no IRQ on falling edge */
    expect(-1 == xtimer_mutex_lock_timeout(&irq_mut, mutex_timeout));
    puts("... OK");

    puts("Testing falling edge on PIN_IN_0");
    expect(0 == gpio_ng_irq(port_in, PIN_IN_0, GPIO_TRIGGER_EDGE_FALLING, irq_edge_cb, &irq_mut));
    /* test for spurious IRQ */
    expect(-1 == xtimer_mutex_lock_timeout(&irq_mut, mutex_timeout));
    gpio_ng_set(port_out, 1UL << PIN_OUT_0);
    /* test for no IRQ on rising edge */
    expect(-1 == xtimer_mutex_lock_timeout(&irq_mut, mutex_timeout));
    gpio_ng_clear(port_out, 1UL << PIN_OUT_0);
    /* test for IRQ on falling edge */
    expect(0 == xtimer_mutex_lock_timeout(&irq_mut, mutex_timeout));
    /* test for spurious IRQ */
    expect(-1 == xtimer_mutex_lock_timeout(&irq_mut, mutex_timeout));
    puts("... OK");

    puts("Testing both edges on PIN_IN_0");
    expect(0 == gpio_ng_irq(port_in, PIN_IN_0, GPIO_TRIGGER_EDGE_BOTH, irq_edge_cb, &irq_mut));
    /* test for spurious IRQ */
    expect(-1 == xtimer_mutex_lock_timeout(&irq_mut, mutex_timeout));
    gpio_ng_set(port_out, 1UL << PIN_OUT_0);
    /* test for IRQ on rising edge */
    expect(0 == xtimer_mutex_lock_timeout(&irq_mut, mutex_timeout));
    /* test for spurious IRQ */
    expect(-1 == xtimer_mutex_lock_timeout(&irq_mut, mutex_timeout));
    gpio_ng_clear(port_out, 1UL << PIN_OUT_0);
    /* test for IRQ on falling edge */
    expect(0 == xtimer_mutex_lock_timeout(&irq_mut, mutex_timeout));
    /* test for spurious IRQ */
    expect(-1 == xtimer_mutex_lock_timeout(&irq_mut, mutex_timeout));
    puts("... OK");

    puts("Testing masking of IRQs (still both edges on PIN_IN_0)");
    gpio_ng_irq_mask(port_in, PIN_IN_0);
    gpio_ng_set(port_out, 1UL << PIN_OUT_0);
    /* test for no IRQ while masked */
    expect(-1 == xtimer_mutex_lock_timeout(&irq_mut, mutex_timeout));
    gpio_ng_irq_unmask_clear(port_in, PIN_IN_0);
    /* test for IRQ of past event is cleared */
    expect(-1 == xtimer_mutex_lock_timeout(&irq_mut, mutex_timeout));
    /* testing for normal behavior after unmasked */
    gpio_ng_clear(port_out, 1UL << PIN_OUT_0);
    expect(0 == xtimer_mutex_lock_timeout(&irq_mut, mutex_timeout));
    /* test for spurious IRQ */
    expect(-1 == xtimer_mutex_lock_timeout(&irq_mut, mutex_timeout));
#if HAS_GPIO_NG_IRQ_UNMASK
    gpio_ng_irq_mask(port_in, PIN_IN_0);
    gpio_ng_set(port_out, 1UL << PIN_OUT_0);
    /* test for no IRQ while masked */
    expect(-1 == xtimer_mutex_lock_timeout(&irq_mut, mutex_timeout));
    gpio_ng_irq_unmask(port_in, PIN_IN_0);
    /* test for IRQ of past event */
    expect(0 == xtimer_mutex_lock_timeout(&irq_mut, mutex_timeout));
    /* test for spurious IRQ */
    expect(-1 == xtimer_mutex_lock_timeout(&irq_mut, mutex_timeout));
#endif
    puts("... OK");
    gpio_ng_irq_off(port_in, PIN_IN_0);
}

struct mutex_counter {
    mutex_t mutex;
    unsigned counter;
};

#ifdef MODULE_PERIPH_GPIO_NG_IRQ_LEVEL_TRIGGERED
static void irq_level_cb(void *_arg)
{
    struct mutex_counter *arg = _arg;

    if (!arg->counter) {
        gpio_ng_toggle(port_out, 1UL << PIN_OUT_0);
        mutex_unlock(&arg->mutex);
    }
    else {
        arg->counter--;
    }
}

static void test_irq_level(void)
{
    struct mutex_counter arg = { .mutex = MUTEX_INIT_LOCKED, .counter = 10 };
    puts("Testing level-triggered on HIGH on PIN_IN_0 (when input is LOW when setting up IRQ)");
    gpio_ng_clear(port_out, 1UL << PIN_OUT_0);
    expect(0 == gpio_ng_irq(port_in, PIN_IN_0, GPIO_TRIGGER_LEVEL_HIGH, irq_level_cb, &arg));
    /* test for spurious IRQ */
    expect(-1 == xtimer_mutex_lock_timeout(&arg.mutex, mutex_timeout));
    /* test for 10 level triggered IRQs on high */
    gpio_ng_set(port_out, 1UL << PIN_OUT_0);
    expect(0 == xtimer_mutex_lock_timeout(&arg.mutex, mutex_timeout));
    /* test for spurious IRQ */
    expect(-1 == xtimer_mutex_lock_timeout(&arg.mutex, mutex_timeout));
    gpio_ng_irq_off(port_in, PIN_IN_0);
    puts("... OK");

    puts("Testing level-triggered on HIGH on PIN_IN_0 (when input is HIGH when setting up IRQ)");
    gpio_ng_set(port_out, 1UL << PIN_OUT_0);
    expect(0 == gpio_ng_irq(port_in, PIN_IN_0, GPIO_TRIGGER_LEVEL_HIGH, irq_level_cb, &arg));
    /* test for 10 level triggered IRQs */
    expect(0 == xtimer_mutex_lock_timeout(&arg.mutex, mutex_timeout));
    /* test for spurious IRQ */
    expect(-1 == xtimer_mutex_lock_timeout(&arg.mutex, mutex_timeout));
    gpio_ng_irq_off(port_in, PIN_IN_0);
    puts("... OK");

    puts("Testing level-triggered on LOW on PIN_IN_0 (when input is HIGH when setting up IRQ)");
    gpio_ng_set(port_out, 1UL << PIN_OUT_0);
    arg.counter = 10;
    expect(0 == gpio_ng_irq(port_in, PIN_IN_0, GPIO_TRIGGER_LEVEL_LOW, irq_level_cb, &arg));
    /* test for spurious IRQ */
    expect(-1 == xtimer_mutex_lock_timeout(&arg.mutex, mutex_timeout));
    /* test for 10 level triggered IRQs on low */
    gpio_ng_clear(port_out, 1UL << PIN_OUT_0);
    expect(0 == xtimer_mutex_lock_timeout(&arg.mutex, mutex_timeout));
    /* test for spurious IRQ */
    expect(-1 == xtimer_mutex_lock_timeout(&arg.mutex, mutex_timeout));
    gpio_ng_irq_off(port_in, PIN_IN_0);
    puts("... OK");

    puts("Testing level-triggered on LOW on PIN_IN_0 (when input is LOW when setting up IRQ)");
    gpio_ng_clear(port_out, 1UL << PIN_OUT_0);
    arg.counter = 10;
    expect(0 == gpio_ng_irq(port_in, PIN_IN_0, GPIO_TRIGGER_LEVEL_LOW, irq_level_cb, &arg));
    /* test for 10 level triggered IRQs */
    expect(0 == xtimer_mutex_lock_timeout(&arg.mutex, mutex_timeout));
    /* test for spurious IRQ */
    expect(-1 == xtimer_mutex_lock_timeout(&arg.mutex, mutex_timeout));
    gpio_ng_irq_off(port_in, PIN_IN_0);
    puts("... OK");
}
#endif /* MODULE_PERIPH_GPIO_NG_IRQ_LEVEL_TRIGGERED */

static void test_irq(void)
{
    if (IS_USED(MODULE_PERIPH_GPIO_NG_IRQ)) {
        puts("\n"
             "Testing External IRQs\n"
             "=====================\n");
        {
            gpio_conf_t input_conf = {
                .state = GPIO_INPUT,
            };
            gpio_conf_t output_conf = {
                .state = GPIO_OUTPUT_PUSH_PULL,
                .slew_rate = GPIO_SLEW_FASTEST,
                .initial_value = false,
            };
            expect(0 == gpio_ng_init(port_in, PIN_IN_0, &input_conf));
            expect(0 == gpio_ng_init(port_out, PIN_OUT_0, &output_conf));
        }

        test_irq_edge();
#ifdef MODULE_PERIPH_GPIO_NG_IRQ_LEVEL_TRIGGERED
        test_irq_level();
#endif /* MODULE_PERIPH_GPIO_NG_IRQ_LEVEL_TRIGGERED */
    }
}
#endif /* MODULE_PERIPH_GPIO_NG_IRQ */

static void print_cpu_cycles(uint32_t loops, uint32_t duration)
{
#ifdef CLOCK_CORECLOCK
    uint64_t divisor = (uint64_t)US_PER_SEC * loops / CLOCK_CORECLOCK;
    uint32_t cycles = (duration + divisor / 2) / divisor;
    printf("~%" PRIu32 " CPU cycles per square wave period (+ 1/4 loop overhead)\n", cycles);
    if (cycles <= 2) {
        puts(":-D");
    }
    else if (cycles <= 4) {
        puts(":-)");
    }
    else if (cycles <= 8) {
        puts(":-|");
    }
    else if (cycles <= 16) {
        puts(":-(");
    }
    else {
        puts(":'-(");
    }
#endif
}

static void bench(void)
{
    static const uint32_t loops = 100000;
    puts("\n"
         "Benchmarking GPIO APIs\n"
         "============================");

    {
        puts("\n"
             "periph/gpio: Using gpio_set() and gpio_clear()\n"
             "----------------------------------------------");
        gpio_t p0 = GPIO_PIN(PORT_OUT, PIN_OUT_0);
        gpio_t p1 = GPIO_PIN(PORT_OUT, PIN_OUT_1);
        gpio_init(p0, GPIO_OUT);
        gpio_init(p1, GPIO_OUT);

        uint32_t start = xtimer_now_usec();
        /* the conditional branch in the loop can take a significant portion of the cycles spend
         * for each loop iteration. Reducing the portion spend on that branch by unrolling four
         * loop iterations into the body of the loop and cutting the iterations in four */
        for (uint32_t i = loops / 4; i > 0; i--) {
            gpio_set(p0);
            gpio_set(p1);
            gpio_clear(p0);
            gpio_clear(p1);
            gpio_set(p0);
            gpio_set(p1);
            gpio_clear(p0);
            gpio_clear(p1);
            gpio_set(p0);
            gpio_set(p1);
            gpio_clear(p0);
            gpio_clear(p1);
            gpio_set(p0);
            gpio_set(p1);
            gpio_clear(p0);
            gpio_clear(p1);
        }
        uint32_t duration = xtimer_now_usec() - start;

        printf("%" PRIu32 " iterations took %" PRIu32 " us\n", loops, duration);
        printf("Two square waves pins at %12" PRIu32 " Hz\n",
               (uint32_t)((uint64_t)US_PER_SEC * loops / duration));
        print_cpu_cycles(loops, duration);
    }

    {
        puts("\n"
             "periph/gpio_ng: Using gpio_ng_set() and gpio_ng_clear()\n"
             "-------------------------------------------------------");
        gpio_conf_t conf = { .state = GPIO_OUTPUT_PUSH_PULL, .slew_rate = GPIO_SLEW_FASTEST };
        expect(0 == gpio_ng_init(port_out, PIN_OUT_0, &conf));
        expect(0 == gpio_ng_init(port_out, PIN_OUT_1, &conf));

        uint32_t start = xtimer_now_usec();
        for (uint32_t i = loops / 4; i > 0; i--) {
            gpio_ng_set(port_out, (1UL << PIN_OUT_0) | (1UL << PIN_OUT_1));
            gpio_ng_clear(port_out, (1UL << PIN_OUT_0) | (1UL << PIN_OUT_1));
            gpio_ng_set(port_out, (1UL << PIN_OUT_0) | (1UL << PIN_OUT_1));
            gpio_ng_clear(port_out, (1UL << PIN_OUT_0) | (1UL << PIN_OUT_1));
            gpio_ng_set(port_out, (1UL << PIN_OUT_0) | (1UL << PIN_OUT_1));
            gpio_ng_clear(port_out, (1UL << PIN_OUT_0) | (1UL << PIN_OUT_1));
            gpio_ng_set(port_out, (1UL << PIN_OUT_0) | (1UL << PIN_OUT_1));
            gpio_ng_clear(port_out, (1UL << PIN_OUT_0) | (1UL << PIN_OUT_1));
        }
        uint32_t duration = xtimer_now_usec() - start;

        printf("periph/gpio_ng: %" PRIu32 " iterations took %" PRIu32 " us\n", loops, duration);
        printf("Two square waves pins at %12" PRIu32 " Hz\n",
               (uint32_t)((uint64_t)US_PER_SEC * loops / duration));
        print_cpu_cycles(loops, duration);
    }

    {
        puts("\n"
             "periph/gpio: Using 2x gpio_toggle()\n"
             "-----------------------------------");
        gpio_t p0 = GPIO_PIN(PORT_OUT, PIN_OUT_0);
        gpio_t p1 = GPIO_PIN(PORT_OUT, PIN_OUT_1);
        gpio_init(p0, GPIO_OUT);
        gpio_init(p1, GPIO_OUT);

        uint32_t start = xtimer_now_usec();
        for (uint32_t i = loops / 4; i > 0; i--) {
            gpio_toggle(p0);
            gpio_toggle(p1);
            gpio_toggle(p0);
            gpio_toggle(p1);
            gpio_toggle(p0);
            gpio_toggle(p1);
            gpio_toggle(p0);
            gpio_toggle(p1);
            gpio_toggle(p0);
            gpio_toggle(p1);
            gpio_toggle(p0);
            gpio_toggle(p1);
            gpio_toggle(p0);
            gpio_toggle(p1);
            gpio_toggle(p0);
            gpio_toggle(p1);
        }
        uint32_t duration = xtimer_now_usec() - start;

        printf("%" PRIu32 " iterations took %" PRIu32 " us\n", loops, duration);
        printf("Two square waves pins at %12" PRIu32 " Hz\n",
               (uint32_t)((uint64_t)US_PER_SEC * loops / duration));
        print_cpu_cycles(loops, duration);
    }

    {
        puts("\n"
             "periph/gpio_ng: Using 2x gpio_ng_toggle()\n"
             "-----------------------------------------");
        gpio_conf_t conf = { .state = GPIO_OUTPUT_PUSH_PULL, .slew_rate = GPIO_SLEW_FASTEST };
        expect(0 == gpio_ng_init(port_out, PIN_OUT_0, &conf));
        expect(0 == gpio_ng_init(port_out, PIN_OUT_1, &conf));

        uint32_t start = xtimer_now_usec();
        for (uint32_t i = loops / 4; i > 0; i--) {
            gpio_ng_toggle(port_out, (1UL << PIN_OUT_0) | (1UL << PIN_OUT_1));
            gpio_ng_toggle(port_out, (1UL << PIN_OUT_0) | (1UL << PIN_OUT_1));
            gpio_ng_toggle(port_out, (1UL << PIN_OUT_0) | (1UL << PIN_OUT_1));
            gpio_ng_toggle(port_out, (1UL << PIN_OUT_0) | (1UL << PIN_OUT_1));
            gpio_ng_toggle(port_out, (1UL << PIN_OUT_0) | (1UL << PIN_OUT_1));
            gpio_ng_toggle(port_out, (1UL << PIN_OUT_0) | (1UL << PIN_OUT_1));
            gpio_ng_toggle(port_out, (1UL << PIN_OUT_0) | (1UL << PIN_OUT_1));
            gpio_ng_toggle(port_out, (1UL << PIN_OUT_0) | (1UL << PIN_OUT_1));
        }
        uint32_t duration = xtimer_now_usec() - start;

        printf("periph/gpio_ng: %" PRIu32 " iterations took %" PRIu32 " us\n", loops, duration);
        printf("Two square waves pins at %12" PRIu32 " Hz\n",
               (uint32_t)((uint64_t)US_PER_SEC * loops / duration));
        print_cpu_cycles(loops, duration);
    }

    {
        puts("\n"
             "periph/gpio: Using 2x gpio_write()\n"
             "----------------------------------");
        gpio_t p0 = GPIO_PIN(PORT_OUT, PIN_OUT_0);
        gpio_t p1 = GPIO_PIN(PORT_OUT, PIN_OUT_1);
        gpio_init(p0, GPIO_OUT);
        gpio_init(p1, GPIO_OUT);

        uint32_t start = xtimer_now_usec();
        for (uint32_t i = loops / 4; i > 0; i--) {
            gpio_write(p0, 1);
            gpio_write(p1, 1);
            gpio_write(p0, 0);
            gpio_write(p1, 0);
            gpio_write(p0, 1);
            gpio_write(p1, 1);
            gpio_write(p0, 0);
            gpio_write(p1, 0);
            gpio_write(p0, 1);
            gpio_write(p1, 1);
            gpio_write(p0, 0);
            gpio_write(p1, 0);
            gpio_write(p0, 1);
            gpio_write(p1, 1);
            gpio_write(p0, 0);
            gpio_write(p1, 0);
        }
        uint32_t duration = xtimer_now_usec() - start;

        printf("%" PRIu32 " iterations took %" PRIu32 " us\n", loops, duration);
        printf("Two square waves pins at %12" PRIu32 " Hz\n",
               (uint32_t)((uint64_t)US_PER_SEC * loops / duration));
        print_cpu_cycles(loops, duration);
    }

    {
        puts("\n"
             "periph/gpio_ng: Using 2x gpio_ng_write()\n"
             "----------------------------------------");
        gpio_conf_t conf = { .state = GPIO_OUTPUT_PUSH_PULL, .slew_rate = GPIO_SLEW_FASTEST };
        expect(0 == gpio_ng_init(port_out, PIN_OUT_0, &conf));
        expect(0 == gpio_ng_init(port_out, PIN_OUT_1, &conf));

        uint32_t start = xtimer_now_usec();
        for (uint32_t i = loops / 4; i > 0; i--) {
            gpio_ng_write(port_out, (1UL << PIN_OUT_0) | (1UL << PIN_OUT_1));
            gpio_ng_write(port_out, 0);
            gpio_ng_write(port_out, (1UL << PIN_OUT_0) | (1UL << PIN_OUT_1));
            gpio_ng_write(port_out, 0);
            gpio_ng_write(port_out, (1UL << PIN_OUT_0) | (1UL << PIN_OUT_1));
            gpio_ng_write(port_out, 0);
            gpio_ng_write(port_out, (1UL << PIN_OUT_0) | (1UL << PIN_OUT_1));
            gpio_ng_write(port_out, 0);
        }
        uint32_t duration = xtimer_now_usec() - start;

        printf("periph/gpio_ng: %" PRIu32 " iterations took %" PRIu32 " us\n", loops, duration);
        printf("Two square waves pins at %12" PRIu32 " Hz\n",
               (uint32_t)((uint64_t)US_PER_SEC * loops / duration));
        print_cpu_cycles(loops, duration);
    }
}

int main(void)
{
    test_gpio_ng_init();
    test_input_output();
#if MODULE_PERIPH_GPIO_NG_IRQ
    test_irq();
#endif
    bench();

    /* if no expect() didn't blow up until now, the test is passed */

    puts("\n\nTEST SUCCEEDED");
    return 0;
}
