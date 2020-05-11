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

USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += netstats_l2
USEMODULE += netstats_ipv6

USEMODULE += vaina

USEMODULE += posix_inet

USEMODULE += timex

USEMODULE += slipdev
USEMODULE += slipdev_stdio

# set slip parameters to default values if unset
ifeq ($(BOARD),cc2538dk)
  SLIP_UART     ?= "UART_NUMOF-1"
else
  SLIP_UART     ?= "0"
endif
SLIP_BAUDRATE ?= 115200

# export slip parameters
CFLAGS += -DSLIP_UART="UART_DEV($(SLIP_UART))"
CFLAGS += -DSLIP_BAUDRATE=$(SLIP_BAUDRATE)

# Enable SLAAC
CFLAGS += -DCONFIG_GNRC_IPV6_NIB_SLAAC=1

CFLAGS += -I$(CURDIR)

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

include $(RIOTBASE)/Makefile.include
