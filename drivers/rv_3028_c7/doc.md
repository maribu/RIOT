@defgroup       drivers_rv_3028_c7      RV-3028-C7 RTC driver
@ingroup        drivers_sensor

@brief          Driver for the Micro Crystal RV-3028-C7 external RTC

# Abstract

The RV-3028-C7 is an external RTC that interfaces via I2C and supports alarms
via an interrupt pin.

For more details refer to the [datasheet][rv_3028_c7_datasheet] and the
[application manual][rv_3028_c7_manual] of the device.

[rv_3028_c7_datasheet]: https://www.microcrystal.com/fileadmin/Media/Products/RTC/Datasheet/RV-3028-C7.pdf
[rv_3028_c7_manual]: https://www.microcrystal.com/fileadmin/Media/Products/RTC/App.Manual/RV-3028-C7_App-Manual.pdf

# Usage

The driver is enabled by using `rv_3028_c7` module, e.g. by adding the following
to the application's `Makefile`:

```Makefile
USEMODULE += rv_3028_c7
```

To use the API in your code, you need to include the following header:

```C
#include "rc_3028_c7.h"
```

# Driver Initialization

Using the `rv_3028_c7` module will automatically enable the
`auto_init_rv_3028_c7` module, which takes care of automatic initialization
of the driver. This can be disabled by adding the following to your applications
`Makefile`:

```
DISABLE_MODULE += auto_init_rv_3028_c7
```

# Compile Time Configuration Options

Defining `RTC_NORMALIZE_COMPAT` to `1` will cause the `tm_yday` member in
`struct tm` to be correctly set in calls to @ref rv_3028_c7_get_time at the
expense of CPU cycles and ROM size, e.g. by adding the following to the
applications `Makefile`:

```Makefile
CFLAGS += -DRTC_NORMALIZE_COMPAT=1
```

# Notable Extra Features

The RV-3028-C7 has an integrated EEPROM of which the first 43 Bytes can be
freely used by the application; the remaining EEPROM storage is used
internally by the RTC e.g. for factory calibration values and should rather not
be written to. This feature is not exposed by the driver.

The RTC has a 7 bit general purpose register and two 8 bit user RAM registers
that can be used by the application to back up data between boots (assuming the
RTC remains powered via battery in-between) with unlimited write cycles. This
feature is not exposed by the driver.

The RTC has a password protection feature to prevent (accidental) writes to
the RTC. Password protection is not implemented by the driver.

The RTC also has an event input pin that can be used for timestamping
(resolution of 1 second) and to trigger an interrupt signal via the interrupt
pin of the RTC. This is useful to detect an IRQ even when the MCU is not
powered, as the event is detected even when the RTC is powered from the backup
power supply. The driver currently does not support this feature.
