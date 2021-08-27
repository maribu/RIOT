/*
 * Copyright (C) 2020 Gunar Schorcht
 *               2021 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    drivers_periph_gpio_ng Next Generation Peripheral GPIO Interface
 * @ingroup     drivers_periph
 * @brief       Next Generation Peripheral GPIO Interface
 *
 * @warning     This API is not stable yet and intended for internal use only as of now.
 *
 * # Design Goals
 *
 * This API aims to provide low level access to GPIOs with as little abstraction and overhead
 * in place as possible for the hot code paths, while providing a relatively high-level and feature
 * complete API for the configuration of GPIO pins. The former is to enable sophisticated use cases
 * such at bit-banging parallel protocols, bit-banging at high data rates, bit-banging with strict
 * timing requirements, or any combination of these. The latter is to expose as much of the features
 * the (arguably) most important peripheral of the MCU as possible.
 *
 * It is possible to implement the high level pin-based GPIO API of RIOT, @ref drivers_periph_gpio,
 * on top of this API. It is expected that for many use cases the high level API will still remain
 * the API of choice, since it is more concise an easier to use.
 *
 * @{
 * @file
 * @brief       Next Generation Peripheral GPIO Interface
 *
 * @author      Gunar Schorcht <gunar@schorcht.net>
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 *
 * @warning     This API is not stable yet and intended for internal use only as of now.
 */

#ifndef PERIPH_GPIO_NG_H
#define PERIPH_GPIO_NG_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "architecture.h"
#include "periph_cpu.h"
#include "periph/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(HAVE_GPIO_PORT_T) || defined(DOXYGEN)
/**
 * @brief   GPIO port type
 */
typedef uintptr_t gpio_port_t;
#endif

#if !defined(GPIO_PORT_UNDEF) || defined(DOXYGEN)
/**
 * @brief   Magic "undefined GPIO port" value
 */
#define GPIO_PORT_UNDEF         UINTPTR_MAX
#endif

#ifdef DOXYGEN
/**
 * @brief   Get the @ref gpio_port_t value of the port identified by @p num
 *
 * @note    If @p num is a compile time constant, this is guaranteed to be suitable for a constant
 *          initializer.
 *
 * Typically this will be something like `(GPIO_BASE_ADDR + num * sizeof(struct vendor_gpio_reg))`
 */
#define GPIO_PORT(num)  implementation_specific
#endif

#ifdef DOXYGEN
/**
 * @brief   Get the number of the GPIO port belonging to the given @ref gpio_port_t value
 *
 * @note    If @p port is a compile time constant, this is guaranteed to be suitable for a constant
 *          initializer.
 *
 * @pre     @p port is the return value of @ref GPIO_PORT
 *
 * For every supported port number *n* the following `assert()` must not blow up:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 * assert(n == GPIO_PORT_NUM(GPIO_PORT(n)));
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define GPIO_PORT_NUM(port) implementation_specific
#endif

#if !defined(HAVE_GPIO_STATE_T) || defined(DOXYGEN)
/**
 * @brief   Enumeration of GPIO direction
 */
typedef enum {
    GPIO_OUTPUT_PUSH_PULL,  /**< Use pin as output in push-pull configuration */
    GPIO_OUTPUT_OPEN_DRAIN, /**< Use pin as output in open drain configuration */
    GPIO_INPUT,             /**< Use pin as input */
    /**
     * @brief   Disconnect pin from all peripherals
     *
     * The implementation should aim to reduce power consumption of the pin when this state is
     * entered, if this is feasible.
     *
     * @note    Pull resistors can still be requested in this mode. This can be useful e.g.
     *          for keeping an UART TXD pin from emitting noise while the UART peripheral is powered
     *          off. But not every implementation will support this.
     *
     * @details Once all GPIOs of a GPIO port are disconnected, the implementation is allowed to
     *          power of the whole GPIO port again to conserve power.
     */
    GPIO_DISCONNECT,
} gpio_state_t;
#endif

#if !defined(HAVE_GPIO_PULL_T) || defined(DOXYGEN)
/**
 * @brief   Enumeration of pull resistor configurations
 */
typedef enum {
    GPIO_FLOATING,  /**< No pull ups nor pull downs enabled */
    GPIO_PULL_UP,   /**< Pull up resistor enabled */
    GPIO_PULL_DOWN, /**< Pull down resistor enabled */
    GPIO_PULL_KEEP, /**< Keep the signal at current logic level with pull up/down resistors */
} gpio_pull_t;
#endif

#if !defined(HAVE_GPIO_PULL_STRENGTH_T) || defined(DOXYGEN)
/**
 * @brief   Enumeration of pull resistor values
 *
 * @note    Depending on the implementation, some (or even all!) constants can have the same
 *          numeric value if less than four pull resistors per direction are provided. For obvious
 *          reasons, only neighboring values are allowed to have the same numeric value.
 */
typedef enum {
    GPIO_PULL_WEAKEST,      /**< Use the weakest (highest Ohm value) resistor */
    GPIO_PULL_WEAK,         /**< Use a weak pull resistor */
    GPIO_PULL_STRONG,       /**< Use a strong pull resistor */
    GPIO_PULL_STRONGEST,    /**< Use the strongest pull resistor */
} gpio_pull_strength_t;
#endif

/**
 * @brief   The number of distinct supported pull resistor strengths
 *
 * This equals the number of pull resistor strengths actually supported and can be less than four,
 * if one or more enumeration values in @ref gpio_pull_strength_t have the same numeric value.
 * Note that: a) some pins might have more options than others and b) it could be possible that
 * there are e.g. two pull up resistors to pick from, but only one pull down resistor.
 */
#define GPIO_PULL_NUMOF (1U + (GPIO_PULL_WEAKEST != GPIO_PULL_WEAK) \
                           + (GPIO_PULL_WEAK != GPIO_PULL_STRONG) \
                           + (GPIO_PULL_STRONG != GPIO_PULL_STRONGEST))

#if !defined(HAVE_GPIO_DRIVE_STRENGTH_T) || defined(DOXYGEN)
/**
 * @brief   Enumeration of drive strength options
 *
 * @note    Depending on the implementation, some (or even all!) constants can have the same
 *          numeric value if less than four drive strength options to pick from. For obvious
 *          reasons, only neighboring values are allowed to have the same numeric value.
 */
typedef enum {
    GPIO_DRIVE_WEAKEST,     /**< Use the weakest (highest Ohm value) resistor */
    GPIO_DRIVE_WEAK,        /**< Use a weak pull resistor */
    GPIO_DRIVE_STRONG,      /**< Use a strong pull resistor */
    GPIO_DRIVE_STRONGEST,   /**< Use the strongest pull resistor */
} gpio_drive_strength_t;
#endif

/**
 * @brief   The number of distinct supported drive strengths
 *
 * This equals the number of drive strengths actually supported and can be less than four,
 * if one or more enumeration values in @ref gpio_drive_strength_t have the same numeric value.
 * Note that some pins might have more options than others.
 */
#define GPIO_DRIVE_NUMOF (1U + (GPIO_DRIVE_WEAKEST != GPIO_DRIVE_WEAK) \
                            + (GPIO_DRIVE_WEAK != GPIO_DRIVE_STRONG) \
                            + (GPIO_DRIVE_STRONG != GPIO_DRIVE_STRONGEST))

#if !defined(HAVE_GPIO_SLEW_T) || defined(DOXYGEN)
/**
 * @brief   Enumeration of slew rate settings
 *
 * Reducing the slew rate can be useful to limit the high frequency noise emitted by a GPIO pin.
 * On the other hand, a high frequency signal cannot be generated if the slew rate is too slow.
 *
 * @warning The numeric values are implementation defined and multiple constants can have the same
 *          numeric value, if an implementation supports fewer slew rates. An implementation only
 *          supporting a single slew rate can have all constants set to a value of zero.
 */
typedef enum {
    GPIO_SLEW_SLOWEST,  /**< let the output voltage level rise/fall as slow as possible */
    GPIO_SLEW_SLOW,     /**< let the output voltage level rise/fall slowly */
    GPIO_SLEW_FAST,     /**< let the output voltage level rise/fall fast */
    GPIO_SLEW_FASTEST,  /**< let the output voltage level rise/fall as fast as possible */
} gpio_slew_t;
#endif

/**
 * @brief   The number of distinct supported slew rates
 *
 * This equals the number of slew rates actually supported and can be less than four,
 * if one or more enumeration values in @ref gpio_drive_strength_t have the same numeric value.
 * Note that some pins might have more options than others.
 */
#define GPIO_SLEW_NUMOF (1U + (GPIO_SLEW_SLOWEST != GPIO_SLEW_SLOW) \
                            + (GPIO_SLEW_SLOW != GPIO_SLEW_FAST) \
                            + (GPIO_SLEW_FAST != GPIO_SLEW_FASTEST))

#if !defined(HAVE_GPIO_CONF_T) || defined(DOXYGEN)
/**
 * @brief   GPIO pin configuration
 *
 * @warning The layout of this structure is implementation dependent and additional implementation
 *          specific fields might be present. For this reason, this structure must be initialized
 *          using designated initializers or zeroing out the whole contents using `memset()` before
 *          initializing the individual fields.
 *
 * It is fully valid that an implementation extends this structure with additional implementation
 * specific fields. For example, it could be useful to also include fields to configure routing of a
 * GPIO pin to other peripherals (e.g. for us as an TXD pin of an UART). These implementation
 * specific fields **MUST** however have reasonable defaults when initialized with zero (e.g. pin is
 * not routed to another peripheral but to be used as regular GPIO). For obvious reasons, portable
 * code cannot rely on the presence and semantic of any implementation specific fields.
 * Additionally, out-of-tree users should not use these fields, as the implementation specific
 * fields cannot be considered a stable API.
 */
typedef struct {
    gpio_state_t state;         /**< State of the pin */
    gpio_pull_t pull;           /**< Pull resistor configuration */
    /**
     * @brief   Configure the slew rate of outputs
     *
     * This value is ignored *unless* @ref gpio_conf_t::direction is configured to
     * @ref GPIO_OUTPUT_PUSH_PULL or @ref GPIO_OUTPUT_OPEN_DRAIN. A call to @ref gpio_ng_init will
     * update this field to the configuration that was actually used. A caller relying on a specific
     * slew rate must hence check if the configuration was applied as requested.
     */
    gpio_slew_t slew_rate;
    /**
     * @brief   Whether to enable the input Schmitt trigger
     *
     * This value is ignored *unless* @ref gpio_conf_t::direction is configured to
     * @ref GPIO_INPUT. A call to @ref gpio_ng_init will update this field to the configuration
     * that was actually used. A caller relying on a specific Schmitt trigger configuration must
     * hence check if the configuration was applied as requested.
     */
    bool schmitt_trigger;
    /**
     * @brief   Initial value of the output
     *
     * Ignored if @ref gpio_conf_t::direction is set to @ref GPIO_INPUT or
     * @ref GPIO_DISCONNECT. If the pin was previously in a high impedance state, it is
     * guaranteed to directly transition to the given initial value.
     */
    bool initial_value;
    /**
     * @brief   Strength of the pull up/down resistor
     *
     * This value is ignored when @ref gpio_conf_t::pull is configured to @ref GPIO_FLOATING. A
     * call to @ref gpio_ng_init will select the pull resistor closet to the specified value and
     * update this field to the actually used value.
     */
    gpio_pull_strength_t pull_strength;
    /**
     * @brief   Drive strength of the GPIO
     *
     * This value is ignored when @ref gpio_conf_t::direction is configured to @ref GPIO_INPUT or
     * @ref GPIO_DISCONNECT. A call to @ref gpio_ng_init will select the drive strength closet
     * to the specified value and update this field to the actually used value.
     */
    gpio_drive_strength_t drive_strength;
} gpio_conf_t;
#endif

/**
 * @brief   Initialize the given GPIO pin as specified
 *
 * @param[in]       port    port the pin to initialize belongs to
 * @param[in]       pin     number of the pin to initialize
 * @param[in,out]   conf    configuration to apply
 *
 * @retval  0           success
 * @retval  -ENOTSUP    direction or pull register config not supported
 *
 * @note    If the configuration of the Schmitt trigger, the drive strength, or the pull resistor
 *          strength are not supported, the closest supported value will be chosen instead and
 *          `0` is returned. The value of these fields in @p conf will be updated to the actually
 *          chosen value.
 */
int gpio_ng_init(gpio_port_t port, uint8_t pin, gpio_conf_t *conf);

/**
 * @brief   Get the current value of all GPIO pins of the given port as bitmask
 *
 * @param[in] port      port to read
 *
 * @return              state of all pins as bitmask
 *
 * @note    The value of unconfigured pins and pins configured as @ref GPIO_DISCONNECT or
 *          @ref GPIO_OUTPUT_PUSH_PULL implementation defined. For pins configured as
 *          @ref GPIO_OUTPUT_OPEN_DRAIN must reflect what is read from the pin, so that a software
 *          I2C bus with support for clock stretching can be implemented.
 */
static inline uword_t gpio_ng_read(gpio_port_t port);

/**
 * @brief   Perform an `reg |= mask` operation on the I/O register of the port
 *
 * @note    Only pins configured as @ref GPIO_OUTPUT_PUSH_PULL or as @ref GPIO_OUTPUT_OPEN_DRAIN
 *          will be affected.
 *
 * @param[in] port      port to modify
 * @param[in] mask      bitmask containing the pins to set
 */
static inline void gpio_ng_set(gpio_port_t port, uword_t mask);

/**
 * @brief   Perform an `reg &= ~mask` operation on the I/O register of the port
 *
 * @note    Only pins configured as @ref GPIO_OUTPUT_PUSH_PULL or as @ref GPIO_OUTPUT_OPEN_DRAIN
 *          will be affected.
 *
 * @param[in] port      port to modify
 * @param[in] mask      bitmask containing the pins to clear
 */
static inline void gpio_ng_clear(gpio_port_t port, uword_t mask);

/**
 * @brief   Perform an `reg ^= mask` operation on the I/O register of the port
 *
 * @note    Only pins configured as @ref GPIO_OUTPUT_PUSH_PULL or as @ref GPIO_OUTPUT_OPEN_DRAIN
 *          will be affected.
 *
 * @warning Some platforms must implement this as `gpio_ng_write(port, gpio_ng_read(port) ^ mask)`
 *          rather than using a single write to a special I/O register. Hence, prefer
 *          @reg gpio_ng_clear, @ref gpio_ng_set or @reg gpio_ng_write when low latency is
 *          required.
 *
 * @param[in] port      port to modify
 * @param[in] mask      bitmask containing the pins to toggle
 */
static inline void gpio_ng_toggle(gpio_port_t port, uword_t mask);

/**
 * @brief   Perform an `reg = mask` operation on the I/O register of the port
 *
 * @note    Only pins configured as @ref GPIO_OUTPUT_PUSH_PULL or as @ref GPIO_OUTPUT_OPEN_DRAIN
 *          will be affected.
 *
 * @param[in] port      port to modify
 * @param[in] value     new state of the GPIO port to write
 */
static inline void gpio_ng_write(gpio_port_t port, uword_t value);

/**
 * @brief   Extract the `gpio_port_t` from a `gpio_t`
 */
static inline gpio_port_t gpio_get_port(gpio_t pin);

/**
 * @brief   Extract the pin number from a `gpio_t`
 */
static inline uint8_t gpio_get_pin_num(gpio_t pin);

#ifdef __cplusplus
}
#endif

/* the hardware specific implementation relies on the types such as gpio_port_t to be provided */
#include "gpio_ng_arch.h"

#endif /* PERIPH_GPIO_NG_H */
/** @} */
