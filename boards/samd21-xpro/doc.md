@defgroup    boards_samd21-xpro Atmel SAM D21 Xplained Pro
@ingroup     boards
@brief       Support for the Atmel SAM D21 Xplained Pro board.

## Overview

The `SAMD21 Xplained Pro` is an ultra-low power evaluation board by Atmel
featuring an ATSAMD21J18A SoC. The SoC includes a SAMD21 ARM Cortex-M0+ micro-
controller. For programming the MCU comes with 32Kb of RAM and 256Kb of flash
memory.

The samd21-xpro is available from various hardware vendors for ~30USD (as of
2017 May).

## Hardware

![samd21-xpro image](http://www.microchip.com/_ImagedCopy/ATSAMD21-XPRO_angle.jpg)


### MCU
| MCU        | ATSAMD21J18A      |
|:------------- |:--------------------- |
| Family | ARM Cortex-M0+    |
| Vendor | Atmel |
| RAM        | 32Kb |
| Flash      | 256Kb             |
| Frequency  | up to 48MHz |
| FPU        | no                |
| Timers | 5 (16-bit)    |
| ADCs       | 1x 12-bit (20 channels)           |
| UARTs      | max 6 (shared with SPI and I2C)               |
| SPIs       | max 6 (see UART)                  |
| I2Cs       | max 6 (see UART)              |
| Vcc        | 1.62V - 3.63V         |
| Datasheet  | [Datasheet](http://ww1.microchip.com/downloads/en/DeviceDoc/SAM_D21_DA1_Family_Data%20Sheet_DS40001882E.pdf) |
| Board Manual   | [Board Manual](http://www.atmel.com/Images/Atmel-42220-SAMD21-Xplained-Pro_User-Guide.pdf)|

### User Interface

1 User button and 1 LED:

| Device | PIN |
|:------ |:--- |
| LED0   | PB30 |
| SW0 (button) | PA15 |


## Implementation Status

| Device | ID        | Supported | Comments  |
|:------------- |:------------- |:------------- |:------------- |
| MCU        | samd21    | yes    | |
| Low-level driver | GPIO    | yes       | |
|        | ADC       | yes   | |
|        | PWM       | yes   | |
|        | UART      | yes       | |
|        | I2C       | yes       | |
|        | SPI       | yes       | |
|        | USB       | yes       | |
|        | RTT       | yes       | |
|        | RTC       | yes       | |
|        | Timer     | yes       | |

## Flashing the device

Connect the device to your Micro-USB cable using the port labeled as *DEBUG
USB*.

The standard method for flashing RIOT to the samd21-xpro is using [edbg](https://github.com/ataradov/edbg).
by calling: `make BOARD=samd21-xpro -C tests/leds flash`

Note that on Linux, you will need libudev-dev package to be installed.

Users can also use openOCD to flash and/or debug the board using:
`PROGRAMMER=openocd make BOARD=samd21-xpro -C tests/leds flash`

On Linux you will have to add a **udev** rule for hidraw, like
```
bash
echo 'KERNEL=="hidraw*", SUBSYSTEM=="hidraw", MODE="0664", GROUP="plugdev"' \
    | sudo tee -a /etc/udev/rules.d/99-usb.rules
sudo service udev restart
```

### Arch Linux
With yaourt:
```
yaourt -S libudev0
yaourt -S hidapi-git
yaourt -S openocd-git
# edit PKGBUILD, add "cmsis-dap hidapi-libusb" to "_features"
```

### Ubuntu
Although this refers to setting up the SAMR21, this guide is still very
helpful to understanding how to set up a solid RIOT development environment for
the SAMD21: http://watr.li/samr21-dev-setup-ubuntu.html

## Known Issues / Problems
