ifeq ($(RENODE),1)
  TARGET = cc2538dk
  CFLAGS += -DRENODE
else
  TARGET = simplelink
  BOARD = launchpad/cc1312r1
endif

MAKE_MAC = MAKE_MAC_CSMA
MAKE_NET = MAKE_NET_IPV6
MAKE_ROUTING = MAKE_ROUTING_NULLROUTING

CFLAGS += -g

CONTIKI_PROJECT = radio-firmware
all: $(CONTIKI_PROJECT)

ifneq ($(RENODE),1)
  PROJECT_SOURCEFILES += uart1-arch.c
endif

PROJECT_SOURCEFILES += aodv-fwc.c
PROJECT_SOURCEFILES += aodv-routing.c
PROJECT_SOURCEFILES += aodv-rt.c

CONTIKI = ./contiki-ng
include $(CONTIKI)/Makefile.include
