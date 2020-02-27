# name of your application
APPLICATION = radio-firmware

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

EXTERNAL_MODULE_DIRS += oonf_api
EXTERNAL_MODULE_DIRS += aodvv2

# Initialize GNRC netif
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_ipv6_router_default

USEMODULE += gnrc_ipv6_default
USEMODULE += gnrc_udp
USEMODULE += gnrc_sock_udp

USEMODULE += oonf_api
USEMODULE += oonf_common
USEMODULE += oonf_rfc5444

USEMODULE += aodvv2

USEMODULE += shell
USEMODULE += shell_commands

USEMODULE += posix_inet

USEMODULE += timex

include $(RIOTBASE)/Makefile.include
