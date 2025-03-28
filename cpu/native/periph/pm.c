/*
 * Copyright (C) 2016 Kaspar Schleiser <kaspar@schleiser.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for
 * more details.
 */

/**
 * @file
 * @ingroup cpu_native
 * @ingroup drivers_periph_pm
 * @brief   Native Power Management implementation
 * @author  Kaspar Schleiser <kaspar@schleiser.de>
 */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include "periph/pm.h"
#include "native_internal.h"
#include "async_read.h"
#include "tty_uart.h"

#ifdef MODULE_PERIPH_SPIDEV_LINUX
#include "spidev_linux.h"
#endif
#ifdef MODULE_PERIPH_GPIO_LINUX
#include "gpiodev_linux.h"
#endif

#define ENABLE_DEBUG 0
#include "debug.h"

unsigned _native_retval = EXIT_SUCCESS;

static void _native_sleep(void)
{
    _native_pending_syscalls_up(); /* no switching here */
    real_pause();
    _native_pending_syscalls_down();

    if (_native_pending_signals > 0) {
        _native_pending_syscalls_up();
        _native_syscall_leave();
    }
}

#if !defined(MODULE_PM_LAYERED)
void pm_set_lowest(void)
{
    _native_sleep();
}
#endif

void pm_set(unsigned mode)
{
    if (mode == 0) {
        _native_sleep();
    }
}

void pm_off(void)
{
    puts("\nnative: exiting");
#ifdef MODULE_PERIPH_SPIDEV_LINUX
    spidev_linux_teardown();
#endif
#ifdef MODULE_PERIPH_GPIO_LINUX
    gpio_linux_teardown();
#endif
#ifdef MODULE_VFS_DEFAULT
    extern void auto_unmount_vfs(void);
    auto_unmount_vfs();
#endif
    real_exit(_native_retval);
}

void pm_reboot(void)
{
    printf("\n\n\t\t!! REBOOT !!\n\n");

    native_async_read_cleanup();
#ifdef MODULE_PERIPH_SPIDEV_LINUX
    spidev_linux_teardown();
#endif
#ifdef MODULE_PERIPH_GPIO_LINUX
    gpio_linux_teardown();
#endif
#ifdef MODULE_VFS_DEFAULT
    extern void auto_unmount_vfs(void);
    auto_unmount_vfs();
#endif

    if (real_execve(_native_argv[0], _native_argv, NULL) == -1) {
        err(EXIT_FAILURE, "reboot: execve");
    }

    errx(EXIT_FAILURE, "reboot: this should not have been reached");
}
