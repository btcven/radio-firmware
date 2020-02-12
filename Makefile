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

#Import posix sockets
USEMODULE += gnrc_udp
USEMODULE += gnrc_sock_udp
USEMODULE += gnrc_sock_ip
USEMODULE += posix_inet
USEMODULE += posix_time

USEMODULE += xtimer
USEMODULE += timex

# Add also the shell, some shell commands
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += gnrc_aodvv2



export INCLUDES += -I${RIOTBASE}/sys/include/net/
export INCLUDES += -I${RIOTBASE}/sys/include/net/gnrc/aodvv2/
export INCLUDES += -I${RIOTBASE}/sys/net/gnrc/aodvv2/

include $(RIOTBASE)/Makefile.include


