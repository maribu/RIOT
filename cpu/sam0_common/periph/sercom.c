/*
 * Copyright (C) 2024 ML!PA Consulting GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_periph_sercom
 * @{
 *
 * @file
 * @brief       Low-level SERCOM driver implementation
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@posteo.net>
 *
 * @}
 */

#include "bitfield.h"
#include "compiler_hints.h"
#include "cpu.h"
#include "modules.h"
#include "mutex.h"
#include "periph_cpu.h"

#if MODULE_PM_LAYERED && defined(SAM0_SERCOM_PM_BLOCK)
#  include "pm_layered.h"
#endif

#include "include/sercom_internal.h"

#define ENABLE_DEBUG 0
#include "debug.h"

#if defined (CPU_COMMON_SAML1X) || defined (CPU_COMMON_SAMD5X)
#  define SERCOM_HAS_DEDICATED_TX_ISR 1
#endif

static mutex_t _locks[SERCOM_NUMOF];
static sercom_irq_cb_t _cbs[SERCOM_NUMOF];
static void *_args[SERCOM_NUMOF];

static BITFIELD(_sercoms_acquired, SERCOM_NUMOF);

__attribute__((const))
Sercom *sercom_get_baseaddr(sercom_t sercom)
{
    assume((unsigned)sercom < SERCOM_NUMOF);
    static Sercom * const addrs[] = {
#if SERCOM_NUMOF > 0
        SERCOM0,
#endif
#if SERCOM_NUMOF > 1
        SERCOM1,
#endif
#if SERCOM_NUMOF > 2
        SERCOM2,
#endif
#if SERCOM_NUMOF > 3
        SERCOM3,
#endif
#if SERCOM_NUMOF > 4
        SERCOM4,
#endif
#if SERCOM_NUMOF > 5
        SERCOM5,
#endif
#if SERCOM_NUMOF > 6
        SERCOM6,
#endif
#if SERCOM_NUMOF > 7
        SERCOM7,
#endif
    };

    return addrs[sercom];
}

/**
 * @brief   Enable peripheral clock for given SERCOM device
 *
 * @param[in] sercom    SERCOM device
 * @param[in] gclk      Generator clock
 */
static void _sercom_clk_enable(sercom_t sercom, uint8_t gclk)
{
#if defined(CPU_COMMON_SAMD21)
    PM->APBCMASK.reg |= (PM_APBCMASK_SERCOM0 << sercom);
#elif defined (CPU_COMMON_SAMD5X)
    if (sercom < 2) {
        MCLK->APBAMASK.reg |= (1 << (sercom + 12));
    } else if (sercom < 4) {
        MCLK->APBBMASK.reg |= (1 << (sercom + 7));
    } else {
        MCLK->APBDMASK.reg |= (1 << (sercom - 4));
    }
#else
    if (sercom < 5) {
        MCLK->APBCMASK.reg |= (MCLK_APBCMASK_SERCOM0 << sercom);
    }
#if defined(CPU_COMMON_SAML21)
    else {
        MCLK->APBDMASK.reg |= (MCLK_APBDMASK_SERCOM5);
    }
#endif /* CPU_COMMON_SAML21 */
#endif

    sam0_gclk_enable(gclk);
#if defined(CPU_COMMON_SAMD21)
    GCLK->CLKCTRL.reg = (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN(gclk) |
                         (SERCOM0_GCLK_ID_CORE + sercom));
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}
#elif defined(CPU_COMMON_SAMD5X)
    GCLK->PCHCTRL[_sercom_gclk_id_core(sercom)].reg = (GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN(gclk));
#else
    if (sercom < 5) {
        GCLK->PCHCTRL[SERCOM0_GCLK_ID_CORE + sercom].reg = (GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN(gclk));
    }
#if defined(CPU_COMMON_SAML21)
    else {
        GCLK->PCHCTRL[SERCOM5_GCLK_ID_CORE].reg = (GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN(gclk));
    }
#endif /* CPU_COMMON_SAML21 */
#endif
}

/**
 * @brief   Disable peripheral clock for given SERCOM device
 *
 * @param[in] sercom    SERCOM device
 */
static void _sercom_clk_disable(sercom_t sercom)
{
#if defined(CPU_COMMON_SAMD21)
    PM->APBCMASK.reg &= ~(PM_APBCMASK_SERCOM0 << sercom);
#elif defined (CPU_COMMON_SAMD5X)
    if (sercom < 2) {
        MCLK->APBAMASK.reg &= ~(1 << (sercom + 12));
    } else if (sercom < 4) {
        MCLK->APBBMASK.reg &= ~(1 << (sercom + 7));
    } else {
        MCLK->APBDMASK.reg &= ~(1 << (sercom - 4));
    }
#else
    if (sercom < 5) {
        MCLK->APBCMASK.reg &= ~(MCLK_APBCMASK_SERCOM0 << sercom);
    }
#if defined (CPU_COMMON_SAML21)
    else {
        MCLK->APBDMASK.reg &= ~(MCLK_APBDMASK_SERCOM5);
    }
#endif /* CPU_COMMON_SAML21 */
#endif
}

/**
 * @brief   Wait for a previous reset to be completed
 */
static void _wait_for_reset_to_complete(Sercom *dev)
{
#ifdef SERCOM_SPI_STATUS_SYNCBUSY
    while (dev->SPI.STATUS.reg & SERCOM_SPI_STATUS_SYNCBUSY) {}
#else
    while (dev->SPI.SYNCBUSY.reg) {}
#endif

    while (dev->SPI.CTRLA.reg & SERCOM_SPI_CTRLA_SWRST) {}
}

void sercom_acquire(sercom_t sercom, const sercom_conf_t *conf,
                    sercom_irq_cb_t irq_cb, void *irq_arg)
{
    assume((unsigned)sercom < SERCOM_NUMOF);
    mutex_lock(&_locks[sercom]);
    Sercom *dev = sercom_get_baseaddr(sercom);
    DEBUG("[sercom] acquiring SERCOM%d @ %p\n", (unsigned)sercom, (void *)dev);

    /* track state to detect possible double frees */
    if (IS_ACTIVE(DEVELHELP)) {
        bf_set_atomic(_sercoms_acquired, sercom);
    }

    _cbs[sercom] = irq_cb;
    _args[sercom] = irq_arg;

    _sercom_clk_enable(sercom, conf->gclk);

    /* A potential previous call to sercom_release() did not wait for the reset
     * to be complete to not waste time. But we need to do so now, in case it
     * is not yet finished anyway */
    _wait_for_reset_to_complete(dev);

    /* SERCOM should now be reseted: Either, because this is the first
     * time we acquire it or because sercom_release() did a reset of the
     * SERCOM */
    assume(!dev->SPI.CTRLA.reg);

    /* Note: The BAUD register is either 8 bit (in case of SPI/I2C) or 16 bit
     *       wide (in case of UART). Due to conf->baud being in little endian,
     *       valid values for conf->baud for SPI/I2C mode will write zeros to
     *       the part exceeding the 8 bit BAUD field. So while this write may
     *       write to addresses that have no meaning, it happens to just work
     *       in all modes.
     */
    dev->USART.BAUD.reg = conf->baud;
    dev->USART.CTRLA.reg = conf->ctrla;
    dev->USART.CTRLB.reg = conf->ctrlb;

    if (irq_cb) {
#if SERCOM_HAS_DEDICATED_TX_ISR
        NVIC_EnableIRQ(SERCOM0_2_IRQn + (sercom * 4));
        NVIC_EnableIRQ(SERCOM0_0_IRQn + (sercom * 4));
#else
        NVIC_EnableIRQ(SERCOM0_IRQn + sercom);
#endif
    }

#if MODULE_PM_LAYERED && defined(SAM0_SERCOM_PM_BLOCK)
    pm_block(SAM0_SERCOM_PM_BLOCK);
#endif
}

void sercom_release(sercom_t sercom)
{
    assume((unsigned)sercom < SERCOM_NUMOF);

    Sercom *dev = sercom_get_baseaddr(sercom);
    DEBUG("[sercom] releasing SERCOM%d @ %p\n", (unsigned)sercom, (void *)dev);

    /* detect possible double releases: */
    if (IS_ACTIVE(DEVELHELP)) {
        assume(bf_isset(_sercoms_acquired, sercom));
        bf_unset_atomic(_sercoms_acquired, sercom);
    }

#if SERCOM_HAS_DEDICATED_TX_ISR
    NVIC_DisableIRQ(SERCOM0_2_IRQn + (sercom * 4));
    NVIC_DisableIRQ(SERCOM0_0_IRQn + (sercom * 4));
#else
    NVIC_DisableIRQ(SERCOM0_IRQn + sercom);
#endif

    dev->SPI.CTRLA.reg = SERCOM_SPI_CTRLA_SWRST;
    _sercom_clk_disable(sercom);
    _cbs[sercom] = NULL;
    _args[sercom] = NULL;

#if IS_ACTIVE(MODULE_PM_LAYERED) && defined(SAM0_SERCOM_PM_BLOCK)
    pm_unblock(SAM0_SERCOM_PM_BLOCK);
#endif
    mutex_unlock(&_locks[sercom]);
}

static void _irq_handler(sercom_t sercom)
{
    _cbs[sercom](_args[sercom]);
    cortexm_isr_end();
}

#if SERCOM_NUMOF > 0
#  if SERCOM_HAS_DEDICATED_TX_ISR
void isr_sercom0_2(void)
{
    _irq_handler(0);
}
void isr_sercom0_0(void)
{
    _irq_handler(0);
}
#  else
void isr_sercom0(void)
{
    _irq_handler(0);
}
#  endif
#endif

#if SERCOM_NUMOF > 1
#  if SERCOM_HAS_DEDICATED_TX_ISR
void isr_sercom1_2(void)
{
    _irq_handler(1);
}
void isr_sercom1_0(void)
{
    _irq_handler(1);
}
#  else
void isr_sercom1(void)
{
    _irq_handler(1);
}
#  endif
#endif

#if SERCOM_NUMOF > 2
#  if SERCOM_HAS_DEDICATED_TX_ISR
void isr_sercom2_2(void)
{
    _irq_handler(2);
}
void isr_sercom2_0(void)
{
    _irq_handler(2);
}
#  else
void isr_sercom2(void)
{
    _irq_handler(2);
}
#  endif
#endif

#if SERCOM_NUMOF > 3
#  if SERCOM_HAS_DEDICATED_TX_ISR
void isr_sercom3_2(void)
{
    _irq_handler(3);
}
void isr_sercom3_0(void)
{
    _irq_handler(3);
}
#  else
void isr_sercom3(void)
{
    _irq_handler(3);
}
#  endif
#endif

#if SERCOM_NUMOF > 4
#  if SERCOM_HAS_DEDICATED_TX_ISR
void isr_sercom4_2(void)
{
    _irq_handler(4);
}
void isr_sercom4_0(void)
{
    _irq_handler(4);
}
#  else
void isr_sercom4(void)
{
    _irq_handler(4);
}
#  endif
#endif

#if SERCOM_NUMOF > 5
#  if SERCOM_HAS_DEDICATED_TX_ISR
void isr_sercom5_2(void)
{
    _irq_handler(5);
}
void isr_sercom5_0(void)
{
    _irq_handler(5);
}
#  else
void isr_sercom5(void)
{
    _irq_handler(5);
}
#  endif
#endif

#if SERCOM_NUMOF > 6
#  if SERCOM_HAS_DEDICATED_TX_ISR
void isr_sercom6_2(void)
{
    _irq_handler(6);
}
void isr_sercom6_0(void)
{
    _irq_handler(6);
}
#  else
void isr_sercom6(void)
{
    _irq_handler(6);
}
#  endif
#endif

#if SERCOM_NUMOF > 7
#  if SERCOM_HAS_DEDICATED_TX_ISR
void isr_sercom7_2(void)
{
    _irq_handler(7);
}
void isr_sercom7_0(void)
{
    _irq_handler(7);
}
#  else
void isr_sercom7(void)
{
    _irq_handler(7);
}
#  endif
#endif
