/*
 * Copyright (C) 2014 Freie Universität Berlin
 * Copyright (C) 2014 Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 * Copyright (C) 2020 Locha Inc
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     net_aodvv2
 * @{
 *
 * @file
 * @brief       RFC5444 server implementation
 *
 * @author      Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 * @author      Gustavo Grisales <gustavosinbandera1@hotmail.com>
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 * @}
 */

#include "net/aodvv2/conf.h"
#include "net/aodvv2.h"
#include "net/aodvv2/metric.h"
#include "net/aodvv2/rcs.h"
#include "net/aodvv2/seqnum.h"
#include "net/manet.h"

#include "mutex.h"

#include "_aodvv2-buffer.h"
#include "_aodvv2-lrs.h"
#include "_aodvv2-mcmsg.h"
#include "_aodvv2-neigh.h"
#include "_aodvv2-reader.h"
#include "_aodvv2-writer.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

static void _route_info(unsigned type, const ipv6_addr_t *ctx_addr,
                        const void *ctx);
static void _route_request(gnrc_pktsnip_t *pkt, const ipv6_addr_t *dst);

static char addr_str[IPV6_ADDR_MAX_STR_LEN];

int aodvv2_init(void)
{
    DEBUG("aodvv2: Initializing\n");

    _aodvv2_buffer_init();
    _aodvv2_lrs_init();
    _aodvv2_mcmsg_init();
    _aodvv2_neigh_init();

    aodvv2_seqnum_init();
    aodvv2_rcs_init();

    gnrc_rfc5444_reader_acquire();
    _aodvv2_reader_init();
    gnrc_rfc5444_reader_release();

    gnrc_rfc5444_writer_acquire();
    if (_aodvv2_writer_init() < 0) {
        DEBUG("  couldn't initialize writer\n");
        gnrc_rfc5444_writer_release();
        return -ENOMEM;
    }
    gnrc_rfc5444_writer_release();

    return 0;
}

int aodvv2_gnrc_netif_join(gnrc_netif_t *netif)
{
    DEBUG("aodvv2: joining netif %d to AODVv2\n", netif_get_id(&netif->netif));

    /* Install route info callback, this is called from the NIB when a route is
     * needed */
    netif->ipv6.route_info_cb = _route_info;

    /* Joint LL-MANET-Routers IPv6 multicast group, this is, to listen to
     * RFC 5444 multicast packets. */
    if (manet_netif_ipv6_group_join(netif) < 0) {
        DEBUG("  couldn't join LL-MANET-Routers group\n");
        return -1;
    }

    /* Add target to LL-MANET-Routers multicast address on this interface */
    if (gnrc_rfc5444_add_writer_target(&ipv6_addr_all_manet_routers_link_local,
                                       netif_get_id(&netif->netif)) < 0) {
        DEBUG("  couldn't add RFC 5444 target\n");
        return -1;
    }

    return 0;
}

static void _route_info(unsigned type, const ipv6_addr_t *ctx_addr,
                        const void *ctx)
{
    DEBUG("aodvv2: route info (type = %d)\n", type);

    switch (type) {
        case GNRC_IPV6_NIB_ROUTE_INFO_TYPE_UNDEF:
            DEBUG("  GNRC_IPV6_NIB_ROUTE_INFO_TYPE_UNDEF\n");
            break;

        case GNRC_IPV6_NIB_ROUTE_INFO_TYPE_RRQ:
            DEBUG("  GNRC_IPV6_NIB_ROUTE_INFO_TYPE_RRQ\n");
            _route_request((gnrc_pktsnip_t *)ctx, ctx_addr);
            break;

        case GNRC_IPV6_NIB_ROUTE_INFO_TYPE_RN:
            DEBUG("  GNRC_IPV6_NIB_ROUTE_INFO_TYPE_RN\n");
            break;

        case GNRC_IPV6_NIB_ROUTE_INFO_TYPE_NSC:
            DEBUG("  GNRC_IPV6_NIB_ROUTE_INFO_TYPE_NSC\n");
            break;

        default:
            DEBUG("  unknown route info");
            break;
    }
}

static void _route_request(gnrc_pktsnip_t *pkt, const ipv6_addr_t *dst)
{
    DEBUG("aodvv2: route request (pkt = %p, dst = %s)\n", (void *)pkt,
          (dst == NULL) ? "NULL" : ipv6_addr_to_str(addr_str, dst,
                                                    sizeof(addr_str)));

    assert(pkt != NULL);
    assert(dst != NULL);

    if (ipv6_addr_is_unspecified(dst) || !ipv6_addr_is_global(dst)) {
        DEBUG("  tried to request route to invalid address\n");
        return;
    }

    ipv6_hdr_t *ipv6_hdr = gnrc_ipv6_get_header(pkt);
    if (ipv6_hdr == NULL) {
        DEBUG("  IPv6 header not found\n");
        return;
    }

    aodvv2_router_client_t client;
    if (aodvv2_rcs_get(&client, &ipv6_hdr->src) < 0) {
        DEBUG("  no matching client for %s\n",
              ipv6_addr_to_str(addr_str, &ipv6_hdr->src, sizeof(addr_str)));
        return;
    }

    /* Buffer this packet so the LRS can check for it */
    if (_aodvv2_buffer_pkt_add(pkt) < 0) {
        DEBUG("  packet buffer is full\n");
        return;
    }

    /* Tell the LRS to lookup a route for the given buffered packet, if an
     * "Idle" route exists, it will be marked as "Active" and the buffered
     * packet(s) will be dispatched, if "Unconfirmed" routes exist, a RREP_Ack
     * request may be triggered in order to confirm bidirectionality, however
     * it's not a guarantee that the the request will get acknowledged, so we
     * do not wait here and when we get the answer, the LRS alone will dispatch
     * the packets if a route isn't already found.
     */
    //if (aodvv2_lrs_lookup_route(dst) == 0) {
    //    DEBUG("aodvv2: route found on LRS for %s\n",
    //          ipv6_addr_to_str(addr_str, dst, sizeof(addr_str)));
    //    return;
    //}


    //aodvv2_msg_rreq_t route_request = {
    //    .msg_hop_limit = CONFIG_AODVV2_MAX_HOPCOUNT,
    //    .orig_prefix = client.addr,
    //    .targ_prefix = *dst,
    //};
}
