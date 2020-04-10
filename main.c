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

#include "net/manet/manet.h"
#include "aodvv2/aodvv2.h"
#include "banner.h"

/**
 * @brief   Find a IEEE 802.15.4 networ interface.
 *
 * @return  The gnrc_netif_t network interface.
 * @retval  NULL if no interface was found.
 */
static gnrc_netif_t *_find_ieee802154_netif(void);

/**
 * @brief   Find a route to a node using a IPv6 address.
 */
static int find_route(int argc, char **argv);

/**
 * @brief   Shell command array.
 */
static const shell_command_t shell_commands[] = {
    { "find_route", "find a route to a node using IPv6 address", find_route },
    { NULL, NULL, NULL }
};


int main(void)
{
    gnrc_netif_t *ieee802154_netif = _find_ieee802154_netif();
    aodvv2_init(ieee802154_netif);

    /* Join LL-MANET-Routers multicast group */
    if (manet_netif_ipv6_group_join(ieee802154_netif) < 0) {
        printf("Couldn't join MANET mcast group\n");
    }

    printf("%s", banner);
    puts("Welcome to Turpial CC1312 Radio!");

    /* Start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

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

static int find_route(int argc, char **argv)
{
    int res = 0;

    if (argc < 2) {
        puts("find_route <target>");
        puts("find a route using AODVv2 protocol to <target>");
        goto exit;
    }

    /* Parse <target> address*/
    ipv6_addr_t target_addr;
    if (ipv6_addr_from_str(&target_addr, argv[1]) == NULL) {
        res = -1;
        printf("%s: invalid <target>!\n", argv[0]);
        goto exit;
    }

    aodvv2_find_route(&target_addr);

exit:
    return res;
}
