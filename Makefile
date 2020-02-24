# name of your application
APPLICATION = hello-world

BOARDSDIR = $(CURDIR)/boards

# If no BOARD is found in the environment, use this default:
BOARD ?= turpial

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/RIOT

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# Whether we want to enable the SLIP protocol
USESLIP ?= 1

ifeq ($(USESLIP),1)
USEMODULE += slipdev
SLIP_UART ?= "UART_NUMOF-1"
SLIP_BAUDRATE ?= 115200

CFLAGS += -DSLIP_UART="UART_DEV($(SLIP_UART))"
CFLAGS += -DSLIP_BAUDRATE=$(SLIP_BAUDRATE)

# This is needed so the SLIP implementation can find slip_params.h
CFLAGS += -I$(CURDIR)

# To add the UART1 peripheral
CFLAGS += -DADITIONAL_UART
endif

include $(RIOTBASE)/Makefile.include
