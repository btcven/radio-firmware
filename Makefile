TARGET = simplelink
BOARD = launchpad/cc1312r1
CFLAGS += -g

CONTIKI_PROJECT = radio-firmware
all: $(CONTIKI_PROJECT)

PROJECT_SOURCEFILES += uart1-arch.c

CONTIKI = ./contiki-ng
include $(CONTIKI)/Makefile.include