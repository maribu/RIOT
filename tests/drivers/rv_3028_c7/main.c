/*
 * SPDX-FileCopyrightText: 2025 ML!PA Consulting GmbH
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @ingroup     tests
 * @{
 *
 * @file
 * @brief       Test application for the Micro Crystal RV-3028-C7 RTC driver
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 * @author      Marian Buschsieweke <marian.buschsieweke@posteo.net>
 *
 * @}
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "fmt.h"
#include "rv_3028_c7.h"
#include "rv_3028_c7_params.h"
#include "shell.h"
#include "tiny_strerror.h"
#include "ztimer.h"

static rv_3028_c7_t _dev;

static int _date_cmd(int argc, char **argv)
{
    struct tm tm;
    int retval;

    if (argc == 1) {
        retval = rv_3028_c7_get_time(&_dev, &tm);
        if (retval) {
            printf("Reading time failed: %s\n", tiny_strerror(retval));
            return retval;
        }
        char stime[32];
        retval = fmt_time_tm_iso8601(stime, &tm, 'T');
        if (retval < 0) {
            printf("Failed to convert struct tm to string: %s\n", tiny_strerror(retval));
            return retval;
        }
        stime[retval] = '\0';
        puts(stime);
        return 0;
    }

    if (argc == 2) {
        retval = scn_time_tm_iso8601(&tm, argv[1], 'T');
        if (retval < 0) {
            printf("Failed to parse given date/time: %s\n", tiny_strerror(retval));
            return retval;
        }

        if (argv[1][retval] != '\0') {
            puts("Failed to parse given date/time: Trailing garbage");
            return -EINVAL;
        }

        retval = rv_3028_c7_set_time(&_dev, &tm);

        if (retval) {
            printf("Failed to write given date/time: %s\n", tiny_strerror(retval));
            return retval;
        }

        return 0;
    }

    printf("Usage: %s [ISO8601]\n", argv[0]);
    return -EINVAL;
}
SHELL_COMMAND(data, "Get/set current date/time", _date_cmd);

int main(void)
{
    int retval;

    puts("RV-3028-C7 RTC test\n");

    /* initialize the device */
    rv_3028_c7_params_t params= rv_3028_c7_params[0];
    retval = rv_3028_c7_init(&_dev, &params);

    if (retval) {
        printf("RTC initialization failed: %s\n", tiny_strerror(retval));
        return retval;
    }

    /* start the shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
