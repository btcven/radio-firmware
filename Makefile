ifeq ($(RENODE),1)
  TARGET = cc2538dk
  CFLAGS += -DRENODE
else
  TARGET = simplelink
  BOARD = launchpad/cc1312r1
endif

CFLAGS += -g

CONTIKI_PROJECT = radio-firmware
all: $(CONTIKI_PROJECT)

ifneq ($(RENODE),1)
  PROJECT_SOURCEFILES += uart1-arch.c
endif

CONTIKI = ./contiki-ng
include $(CONTIKI)/Makefile.include
