CONTIKI_PROJECT = radio-firmware
all: $(CONTIKI_PROJECT)

CONTIKI = ./contiki-ng
include $(CONTIKI)/Makefile.include
