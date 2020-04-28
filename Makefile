# name of your application
APPLICATION = radio-firmware

# If no BOARD is found in the environment, use this default:
BOARD ?= turpial
ifeq ($(BOARD),turpial)
  BOARDSDIR ?= $(CURDIR)/boards
endif

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/RIOT

# Application absolute path
APPBASE ?= $(CURDIR)

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

EXTERNAL_MODULE_DIRS += sys

# Initialize GNRC netif
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif

USEMODULE += gnrc_ipv6_default
USEMODULE += gnrc_ipv6_router_default
USEMODULE += gnrc_udp
USEMODULE += gnrc_pktdump

USEMODULE += manet
USEMODULE += aodvv2
USEMODULE += shell_extended

USEMODULE += chat

USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += netstats_l2
USEMODULE += netstats_ipv6

USEMODULE += posix_inet

USEMODULE += timex

USEMODULE += slipdev

# set slip parameters to default values if unset
SLIP_UART     ?= "UART_NUMOF-1"
SLIP_BAUDRATE ?= 115200

# export slip parameters
CFLAGS += -DSLIP_UART="UART_DEV($(SLIP_UART))"
CFLAGS += -DSLIP_BAUDRATE=$(SLIP_BAUDRATE)

CFLAGS += -I$(CURDIR)

include $(RIOTBASE)/Makefile.include
