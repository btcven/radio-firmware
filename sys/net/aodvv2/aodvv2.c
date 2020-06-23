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

#include "net/aodvv2.h"
#include "net/aodvv2/msg.h"
#include "net/aodvv2/lrs.h"
#include "net/aodvv2/mcmsg.h"
#include "net/aodvv2/metric.h"
#include "net/aodvv2/rcs.h"
#include "net/aodvv2/seqnum.h"

#include "net/gnrc/netif.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/ipv6/nib.h"

#include "aodvv2_reader.h"
#include "aodvv2_writer.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

static void _route_info(unsigned type, const ipv6_addr_t *ctx_addr,
                        const void *ctx);
static void _rrq(const ipv6_addr_t *target, gnrc_pktsnip_t *pkt);
static void  _find_route(const ipv6_addr_t *orig_prefix, uint8_t orig_pfx_len,
                         const uint8_t orig_metric,
                         const ipv6_addr_t *targ_prefix);

static char addr_str[IPV6_ADDR_MAX_STR_LEN];

#if IS_USED(MODULE_AUTO_INIT_AODVV2)
void aodvv2_auto_init(void)
{
    aodvv2_init();
}
#endif

void aodvv2_init(void)
{
    /* Initialize AODVv2 internal structures */
    aodvv2_seqnum_init();
    aodvv2_lrs_init();
    aodvv2_rcs_init();
    aodvv2_mcmsg_init();
    aodvv2_buffer_init();

    /* Register AODVv2 messages reader */
    rfc5444_reader_acquire();
    aodvv2_reader_init(&rfc5444_protocol.reader);
    rfc5444_reader_release();

    rfc5444_writer_acquire();
    aodvv2_writer_init(&rfc5444_protocol.writer);
    rfc5444_writer_release();
}

void aodvv2_netif_register(gnrc_netif_t *netif)
{
    assert(netif != NULL);

    assert(manet_netif_ipv6_group_join(netif) == 0);
    assert(rfc5444_register_target(&ipv6_addr_all_manet_routers_link_local,
                                   netif_get_id(&netif->netif), 0) != NULL);

    /* Install route info callback, this is called from the NIB when a route is
     * needed, this provides us information about needed routes for this network
     * interface */
    netif->ipv6.route_info_cb = _route_info;
}

static void _route_info(unsigned type, const ipv6_addr_t *ctx_addr,
                        const void *ctx)
{
    DEBUG("aodvv2: route info: %s\n",
            ipv6_addr_to_str(addr_str, ctx_addr, sizeof(addr_str)));

    switch (type) {
        case GNRC_IPV6_NIB_ROUTE_INFO_TYPE_UNDEF:
            DEBUG_PUTS("aodvv2: GNRC_IPV6_NIB_ROUTE_INFO_TYPE_UNDEF");
            break;

        case GNRC_IPV6_NIB_ROUTE_INFO_TYPE_RRQ:
            DEBUG_PUTS("aodvv2: GNRC_IPV6_NIB_ROUTE_INFO_TYPE_RRQ");
            _rrq(ctx_addr, (gnrc_pktsnip_t *)ctx);
            break;

        case GNRC_IPV6_NIB_ROUTE_INFO_TYPE_RN:
            DEBUG_PUTS("aodvv2: GNRC_IPV6_NIB_ROUTE_INFO_TYPE_RN");
            break;

        case GNRC_IPV6_NIB_ROUTE_INFO_TYPE_NSC:
            DEBUG_PUTS("aodvv2: GNRC_IPV6_NIB_ROUTE_INFO_TYPE_NSC");
            break;

        default:
            DEBUG_PUTS("aodvv2: unknown route info");
            break;
    }
}

static void _rrq(const ipv6_addr_t *target, gnrc_pktsnip_t *pkt)
{
    DEBUG("aodvv2: rrq: %s\n",
          ipv6_addr_to_str(addr_str, target, sizeof(addr_str)));

    assert(target != NULL);
    assert(pkt != NULL);

    ipv6_hdr_t *ipv6_hdr = gnrc_ipv6_get_header(pkt);
    if (ipv6_hdr == NULL) {
        DEBUG_PUTS("aodvv2: rrq: IPv6 header not found on packet");
        return;
    }

    aodvv2_rcs_entry_t *client;
    if ((client = aodvv2_rcs_is_client(&ipv6_hdr->src)) == NULL) {
        DEBUG("aodvv2: rrq: %s is not our client\n",
              ipv6_addr_to_str(addr_str, &ipv6_hdr->src, sizeof(addr_str)));
        return;
    }

    DEBUG("aodvv2: rrq: found matching client %s/%d\n",
            ipv6_addr_to_str(addr_str, &client->addr, sizeof(addr_str)),
            client->pfx_len);

    if (aodvv2_buffer_pkt_add(target, pkt) != 0) {
        DEBUG_PUTS("aodvv2: packet buffer is full");
        return;
    }

    DEBUG_PUTS("aodvv2: rrq: finding route");
    _find_route(&client->addr, client->pfx_len, client->cost, target);
}

static void  _find_route(const ipv6_addr_t *orig_prefix, uint8_t orig_pfx_len,
                         const uint8_t orig_metric,
                         const ipv6_addr_t *targ_prefix)
{
    assert(orig_prefix != NULL);
    assert(targ_prefix != NULL);

    /* Guard against invalid values */
    if (orig_pfx_len > 128) {
        orig_pfx_len = 128;
    }

    aodvv2_seqnum_t seqnum = aodvv2_seqnum_get();
    aodvv2_seqnum_inc();

    aodvv2_message_t rreq = {
        .msg_hop_limit = CONFIG_AODVV2_MAX_HOPCOUNT,
        /* TODO(jeandudey): this field is meaningless, only used when receiving
         * and requires a shitty hack to set it, set this on rfc5444_protocol_t
         * instead when reading a packet and remove this shit */
        .sender = {},
        .metric_type = CONFIG_AODVV2_DEFAULT_METRIC,
        .orig_node = {
            .addr = *orig_prefix,
            .pfx_len = orig_pfx_len,
            .metric = orig_metric,
            .seqnum = seqnum,
        },
        .targ_node = {
            .addr = *targ_prefix,
            .pfx_len = 128,
            .metric = 0,
            /* TODO(jeandudey): use TargPrefix if an INVALID Local Route exists
             * for this target, otherwise ignore it. Packet format needs to be
             * updated first (and LRS) */
            .seqnum = 0,
        },
        /* TODO(jeandudey): add SeqNoRtr if it exists, this is "this" node global
         * address, in other words, the router that generated the RREQ */
        .seqnortr = {},
        /* TODO(jeandudey): update LRS and remove this, we shouldn't be managing
         * timestamp on received messages, instead, use the _correct_ timestamp
         * when processing it (xtimer_now_timex). */
        .timestamp = {},
    };

    rfc5444_writer_acquire();
    aodvv2_writer_send_rreq(&rfc5444_protocol.writer, &rreq);
    rfc5444_writer_release();
}
