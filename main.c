/* Copyright 2020 Locha Inc
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @{
 * @file
 * @brief       Main radio-firmware file
 *
 * @author      Locha Mesh Developers <developers@locha.io>
 * @}
 */

#include <stdio.h>

#include "shell.h"
#include "msg.h"

#include "net/aodvv2/aodvv2.h"
#include "net/manet/manet.h"

#if IS_USED(MODULE_VAINA)
#include "net/vaina.h"
#endif

#if IS_USED(MODULE_SHELL_EXTENDED)
#include "shell_extended.h"
#endif

#ifndef CONFIG_SLIP_LOCAL_ADDR
/**
 * @brief   SLIP link local address
 */
#define CONFIG_SLIP_LOCAL_ADDR "fe80::dead:beef:cafe:babe"
#endif

/**
 * @brief   Find a network interface.
 *
 * @param[in] nettype The network type of the interface to find.
 *
 * @return The gnrc_netif_t network interface.
 * @retval NULL if no interface was found.
 */
static gnrc_netif_t *_find_netif(uint16_t nettype);

int main(void)
{
    gnrc_netif_t *ieee802154_netif = _find_netif(NETDEV_TYPE_IEEE802154);

    /* Join LL-MANET-Routers multicast group */
    if (manet_netif_ipv6_group_join(ieee802154_netif) < 0) {
        printf("Couldn't join MANET mcast group\n");
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

    /* Initialize RFC5444 */
    if (aodvv2_init(ieee802154_netif) < 0) {
        printf("Couldn't initialize RFC5444\n");
    }

#if IS_USED(MODULE_VAINA)
    gnrc_netif_t *slipdev_netif = _find_netif(NETDEV_TYPE_SLIP);
    printf("found SLIP netif %d\n", slipdev_netif->pid);
    if (slipdev_netif == NULL) {
        printf("VAINA needs a wired interface (SLIP) to work!\n");
    }
    else {
        ipv6_addr_t addr;
        if (ipv6_addr_from_str(&addr, CONFIG_SLIP_LOCAL_ADDR) == NULL) {
            printf("Malformed SLIP local address, please verify it!\n");
        }

        if (gnrc_netif_ipv6_addr_add(slipdev_netif, &addr, 128, 0) != sizeof(ipv6_addr_t)) {
            printf("Couldn't setup SLIP local address\n");
        }

        if (vaina_init(slipdev_netif) < 0) {
            printf("Couldn't initialize VAINA\n");
        }
    }
#endif

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

static gnrc_netif_t *_find_netif(uint16_t nettype)
{
    static uint16_t device_type = 0;

    static gnrc_netapi_opt_t opt = {
        .opt = NETOPT_DEVICE_TYPE,
        .context = 0,
        .data = &device_type,
        .data_len = sizeof(uint16_t),
    };

    /* Iterate over network interfaces and find one that matches */
    gnrc_netif_t *netif = NULL;
    for (netif = gnrc_netif_iter(netif);
         netif != NULL;
         netif = gnrc_netif_iter(netif)) {
        if (gnrc_netif_get_from_netdev(netif, &opt) == sizeof(uint16_t)) {
            if (device_type == nettype) {
                return netif;
            }
        }
    }

    return NULL;
}
