# name of your application
APPLICATION = radio-firmware

# If no BOARD is found in the environment, use this default:
BOARD ?= turpial
EXTERNAL_BOARD_DIRS ?= $(CURDIR)/boards

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/RIOT

# Application absolute path
APPBASE ?= $(CURDIR)

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 0

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

EXTERNAL_MODULE_DIRS += sys

# Initialize GNRC netif
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif

USEMODULE += gnrc_ipv6_router
USEMODULE += gnrc_icmpv6
USEMODULE += gnrc_icmpv6_echo
USEMODULE += gnrc_udp
USEMODULE += gnrc_pktdump

USEMODULE += manet
USEMODULE += aodvv2
USEMODULE += shell_extended

USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += netstats_l2
USEMODULE += netstats_ipv6

USEMODULE += vaina

USEMODULE += posix_inet

USEMODULE += timex

USEMODULE += slipdev

# Enable SLAAC
CFLAGS += -DCONFIG_GNRC_IPV6_NIB_SLAAC=1

USE_SLIPTTY ?= 0
ifeq (1,$(USE_SLIPTTY))
  USEMODULE += slipdev_stdio

  SLIP_UART     ?= "0"
  SLIP_BAUDRATE ?= 115200
else
  SLIP_UART     ?= "1"
  SLIP_BAUDRATE ?= 115200
endif

# export slip parameters
CFLAGS += -DSLIPDEV_PARAM_UART="UART_DEV($(SLIP_UART))"
CFLAGS += -DSLIPDEV_PARAM_BAUDRATE=$(SLIP_BAUDRATE)

CFLAGS += -I$(CURDIR)

ifeq (1,$(USE_SLIPTTY))
.PHONY: host-tools

host-tools:
	$(Q)env -u CC -u CFLAGS make -C $(RIOTTOOLS)

sliptty:
	$(Q)env -u CC -u CFLAGS make -C $(RIOTTOOLS)/sliptty

  IPV6_PREFIX = 2001:db8::/64

# Configure terminal parameters
  TERMDEPS += host-tools
  TERMPROG ?= sudo sh $(CURDIR)/dist/tools/vaina/start_network.sh
  TERMFLAGS ?= $(FLAGS_EXTRAS) $(IPV6_PREFIX) $(PORT) $(SLIP_BAUDRATE)
endif

include $(RIOTBASE)/Makefile.include
