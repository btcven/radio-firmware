/**
 * @file main.c
 * @author Locha Mesh Developers (contact@locha.io)
 * @brief Main firmware file
 * @version 0.1
 * @date 2020-02-02
 *
 * @copyright Copyright (c) 2020 Locha Mesh project developers
 * @license Apache 2.0, see LICENSE file for details
 */

#include <stdio.h>

#include "shell.h"
#include "msg.h"

#include "net/aodvv2/aodvv2.h"
#include "net/manet/manet.h"

#if IS_USED(MODULE_SHELL_EXTENDED)
#include "shell_extended.h"
#endif

/**
 * @brief   Find a IEEE 802.15.4 networ interface.
 *
 * @return  The gnrc_netif_t network interface.
 * @retval  NULL if no interface was found.
 */
static gnrc_netif_t *_find_ieee802154_netif(void);

int main(void)
{
    gnrc_netif_t *ieee802154_netif = _find_ieee802154_netif();

    if (IS_USED(MODULE_MANET)) {
        /* Join LL-MANET-Routers multicast group */
        if (manet_netif_ipv6_group_join(ieee802154_netif) < 0) {
            printf("Couldn't join MANET mcast group\n");
        }
    }

    /* Add global address */
    eui64_t iid;
    if (gnrc_netif_ipv6_get_iid(ieee802154_netif, &iid) == sizeof(uint64_t)) {
        ipv6_addr_t addr = {{ 0x20, 0x01, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00 }};
        ipv6_addr_init_iid(&addr, (uint8_t *)&iid, 64);

        if (gnrc_netif_ipv6_addr_add(ieee802154_netif, &addr, 128, 0) != sizeof(ipv6_addr_t)) {
            printf("Couln't setup global address\n");
        }
    }

    if (IS_USED(MODULE_AODVV2)) {
        /* Initialize RFC5444 */
        if (aodvv2_init(ieee802154_netif) < 0) {
            printf("Couldn't initialize RFC5444\n");
        }
    }


    /* Initialize RFC5444 */
    if (aodvv2_init(ieee802154_netif) < 0) {
        printf("Couldn't initialize RFC5444\n");
    }

    puts("Welcome to Turpial CC1312 Radio!");

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    /* Start shell */
#if IS_USED(MODULE_SHELL_EXTENDED)
    shell_run(shell_extended_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
#else
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);
#endif

    /* Should be never reached */
    return 0;
}

static gnrc_netif_t *_find_ieee802154_netif(void)
{
    static uint16_t device_type = 0;

    static gnrc_netapi_opt_t opt = {
        .opt = NETOPT_DEVICE_TYPE,
        .context = 0,
        .data = &device_type,
        .data_len = sizeof(uint16_t),
    };

    /* Iterate over network interfaces and find one that's IEEE 802.15.4, for
     * the CC1312 there is only one interface, but we need to be sure it was
     * initialized. Also well be using other interface; probably SLIP over UART
     * so we need to be sure it's IEEE 802.15.4 */
    gnrc_netif_t *netif = NULL;
    for (netif = gnrc_netif_iter(netif);
         netif != NULL;
         netif = gnrc_netif_iter(netif)) {
        if (gnrc_netif_get_from_netdev(netif, &opt) == sizeof(uint16_t)) {
            if (device_type == NETDEV_TYPE_IEEE802154) {
                return netif;
            }
        }
    }

    return NULL;
}
