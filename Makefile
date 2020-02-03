# name of your application
APPLICATION = aodvv2

# If no BOARD is found in the environment, use this default:
BOARD ?= cc1312-launchpad

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/RIOT

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1


USEPKG += oonf_api
USEMODULE += oonf_rfc5444 

BOARD_PROVIDES_NETIF := acd52832 airfy-beacon atmega256rfr2-xpro avr-rss2 b-l072z-lrwan1 cc2538dk fox \
        derfmega128 derfmega256 hamilton iotlab-m3 iotlab-a8-m3 lobaro-lorabox lsn50 mulle microbit msba2 \
        microduino-corerf native nrf51dk nrf51dongle nrf52dk nrf52840dk nrf52840-mdk nrf6310 \
        nucleo-f207zg nucleo-f767zi openmote-b openmote-cc2538 pba-d-01-kw2x remote-pa \
        remote-reva ruuvitag samr21-xpro samr30-xpro spark-core telosb thingy52 yunjia-nrf51822 z1

ifneq (,$(filter $(BOARD),$(BOARD_PROVIDES_NETIF)))
  # Use modules for networking
  # gnrc is a meta module including all required, basic gnrc networking modules
  USEMODULE += gnrc
  # use the default network interface for the board
  USEMODULE += gnrc_netdev_default
  # automatically initialize the network interface
  USEMODULE += auto_init_gnrc_netif
  #for basic IPV6 and 6loWPAN functionallities
  USEMODULE += gnrc_ipv6_default 

  USEMODULE += gnrc_pktdump

  
  # Import posix sockets
USEMODULE += gnrc_udp
USEMODULE += gnrc_sock_udp
USEMODULE += posix_sockets
USEMODULE += posix_inet
USEMODULE += posix_time

USEMODULE += xtimer
USEMODULE += timex

# Add also the shell, some shell commands
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps


# use IEEE 802.15.4 as link-layer protocol
 #USEMODULE += netdev_ieee802154
 #USEMODULE += netdev_test

# We use only the lower layers of the GNRC network stack, hence, we can
# reduce the size of the packet buffer a bit
CFLAGS += -DGNRC_PKTBUF_SIZE=512
endif

USEMODULE += aodvv2



export INCLUDES += -I${RIOTBASE}/sys/include/net/
export INCLUDES += -I${RIOTBASE}/sys/include/net/gnrc/aodvv2/
export INCLUDES += -I${RIOTBASE}/sys/net/gnrc/aodvv2/

# lower log-level to save memory of LOG_WARNING() in gnrc_netif
CFLAGS += -DLOG_LEVEL=LOG_ERROR
CFLAGS += -DGNRC_NETTYPE_NDP=GNRC_NETTYPE_TEST
CFLAGS += -DGNRC_PKTBUF_SIZE=512
CFLAGS += -DTEST_SUITES

# Set a custom channel if needed
#include $(RIOTMAKE)/default-radio-settings.inc.mk

include $(RIOTBASE)/Makefile.include
