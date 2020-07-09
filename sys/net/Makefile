MODULE = radio_firmware_net

ifneq (,$(filter aodvv2,$(USEMODULE)))
  DIRS += aodvv2
endif
ifneq (,$(filter manet,$(USEMODULE)))
  DIRS += manet
endif
ifneq (,$(filter rfc5444,$(USEMODULE)))
  DIRS += rfc5444
endif
ifneq (,$(filter vaina,$(USEMODULE)))
  DIRS += vaina
endif

include $(RIOTBASE)/Makefile.base
