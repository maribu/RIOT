BOARD ?= nucleo-f767zi

ifeq (nucleo-f767zi,$(BOARD))
  PORT_IN ?= 1
  PORT_OUT ?= 0
  PIN_IN_0 ?= 8
  PIN_IN_1 ?= 9
  PIN_OUT_0 ?= 5
  PIN_OUT_1 ?= 6
endif

ifeq (nrf52840dk,$(BOARD))
  PORT_IN ?= 1
  PORT_OUT ?= 0
  PIN_IN_0 ?= 10
  PIN_IN_1 ?= 11
  PIN_OUT_0 ?= 26
  PIN_OUT_1 ?= 27
endif

# Choose two input pins and two pins that do not conflict with stdio. All four *can* be on the same
# GPIO port, but the two output pins and the two inputs pins *must* be on the same port,
# respectively. Connect the first input to the first output pin and the second input pin to the
# second output pin, e.g. using jumper wires.
#
# Beware: If other pins on the output port are configured as output GPIOs, they will be written to
#         during this test.
PORT_IN ?= 1
PORT_OUT ?= 0
PIN_IN_0 ?= 10
PIN_IN_1 ?= 11
PIN_OUT_0 ?= 27
PIN_OUT_1 ?= 26

include ../Makefile.tests_common

FEATURES_REQUIRED += periph_gpio_ng
FEATURES_REQUIRED += periph_gpio
FEATURES_OPTIONAL += periph_gpio_ng_irq
FEATURES_OPTIONAL += periph_gpio_ng_irq_level_triggered

USEMODULE += xtimer

include $(RIOTBASE)/Makefile.include

CFLAGS += -DPORT_OUT=$(PORT_OUT)
CFLAGS += -DPORT_IN=$(PORT_IN)
CFLAGS += -DPIN_IN_0=$(PIN_IN_0)
CFLAGS += -DPIN_IN_1=$(PIN_IN_1)
CFLAGS += -DPIN_OUT_0=$(PIN_OUT_0)
CFLAGS += -DPIN_OUT_1=$(PIN_OUT_1)
