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
 * @brief       RFC5444 RREP
 *
 * @author      Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 * @author      Gustavo Grisales <gustavosinbandera1@hotmail.com>
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 * @}
 */

#include "net/aodvv2/aodvv2.h"
#include "net/aodvv2/client.h"
#include "net/aodvv2/metric.h"
#include "net/aodvv2/rfc5444.h"
#include "net/aodvv2/lrs.h"
#include "net/aodvv2/rreqtable.h"
#include "net/manet/manet.h"

#include "net/gnrc/ipv6/nib/ft.h"

#include "xtimer.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

#define AODVV2_ROUTE_LIFETIME \
    (CONFIG_AODVV2_ACTIVE_INTERVAL + CONFIG_AODVV2_MAX_IDLETIME)

static enum rfc5444_result _cb_rrep_blocktlv_addresstlvs_okay(
    struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_rrep_blocktlv_messagetlvs_okay(
    struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_rrep_end_callback(
    struct rfc5444_reader_tlvblock_context *cont, bool dropped);

static enum rfc5444_result _cb_rreq_blocktlv_addresstlvs_okay(
    struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_rreq_blocktlv_messagetlvs_okay(
    struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_rreq_end_callback(
    struct rfc5444_reader_tlvblock_context *cont, bool dropped);

/*
 * Message consumer, will be called once for every message of
 * type RFC5444_MSGTYPE_RREP that contains all the mandatory message TLVs
 */
static struct rfc5444_reader_tlvblock_consumer _rrep_consumer =
{
    .msg_id = RFC5444_MSGTYPE_RREP,
    .block_callback = _cb_rrep_blocktlv_messagetlvs_okay,
    .end_callback = _cb_rrep_end_callback,
};

/*
 * Address consumer. Will be called once for every address in a message of
 * type RFC5444_MSGTYPE_RREP.
 */
static struct rfc5444_reader_tlvblock_consumer _rrep_address_consumer =
{
    .msg_id = RFC5444_MSGTYPE_RREP,
    .addrblock_consumer = true,
    .block_callback = _cb_rrep_blocktlv_addresstlvs_okay,
};

/*
 * Message consumer, will be called once for every message of
 * type RFC5444_MSGTYPE_RREQ that contains all the mandatory message TLVs
 */
static struct rfc5444_reader_tlvblock_consumer _rreq_consumer =
{
    .msg_id = RFC5444_MSGTYPE_RREQ,
    .block_callback = _cb_rreq_blocktlv_messagetlvs_okay,
    .end_callback = _cb_rreq_end_callback,
};

/*
 * Address consumer. Will be called once for every address in a message of
 * type RFC5444_MSGTYPE_RREQ.
 */
static struct rfc5444_reader_tlvblock_consumer _rreq_address_consumer =
{
    .msg_id = RFC5444_MSGTYPE_RREQ,
    .addrblock_consumer = true,
    .block_callback = _cb_rreq_blocktlv_addresstlvs_okay,
};

/*
 * Address consumer entries definition
 * TLV types RFC5444_MSGTLV__SEQNUM and RFC5444_MSGTLV_METRIC
 */
static struct rfc5444_reader_tlvblock_consumer_entry _address_consumer_entries[] =
{
    [RFC5444_MSGTLV_ORIGSEQNUM] = { .type = RFC5444_MSGTLV_ORIGSEQNUM },
    [RFC5444_MSGTLV_TARGSEQNUM] = { .type = RFC5444_MSGTLV_TARGSEQNUM },
    [RFC5444_MSGTLV_METRIC] = { .type = RFC5444_MSGTLV_METRIC }
};

static struct netaddr_str nbuf;
static aodvv2_packet_data_t packet_data;

static kernel_pid_t _netif_pid = KERNEL_PID_UNDEF;

static enum rfc5444_result _cb_rrep_blocktlv_messagetlvs_okay(
        struct rfc5444_reader_tlvblock_context *cont)
{
    if (!cont->has_hoplimit) {
        DEBUG("rfc5444_reader: missing hop limit\n");
        return RFC5444_DROP_PACKET;
    }

    packet_data.hoplimit = cont->hoplimit;
    if (packet_data.hoplimit == 0) {
        DEBUG("rfc5444_reader: hop limit is 0.\n");
        return RFC5444_DROP_PACKET;
    }

    packet_data.hoplimit--;
    return RFC5444_OKAY;
}

static enum rfc5444_result _cb_rrep_blocktlv_addresstlvs_okay(
        struct rfc5444_reader_tlvblock_context *cont)
{
    struct rfc5444_reader_tlvblock_entry *tlv;
    bool is_targ_node_addr = false;

    DEBUG("rfc5444_reader: %s\n", netaddr_to_string(&nbuf, &cont->addr));

    /* handle TargNode SeqNum TLV */
    tlv = _address_consumer_entries[RFC5444_MSGTLV_TARGSEQNUM].tlv;
    if (tlv) {
        DEBUG("rfc5444_reader: RFC5444_MSGTLV_TARGSEQNUM: %d\n",
              *tlv->single_value);
        is_targ_node_addr = true;
        netaddr_to_ipv6_addr(&cont->addr, &packet_data.targ_node.addr);
        packet_data.targ_node.seqnum = *tlv->single_value;
    }

    /* handle OrigNode SeqNum TLV */
    tlv = _address_consumer_entries[RFC5444_MSGTLV_ORIGSEQNUM].tlv;
    if (tlv) {
        DEBUG("rfc5444_reader: RFC5444_MSGTLV_ORIGSEQNUM: %d\n",
              *tlv->single_value);
        is_targ_node_addr = false;
        netaddr_to_ipv6_addr(&cont->addr, &packet_data.orig_node.addr);
        packet_data.orig_node.seqnum = *tlv->single_value;
    }

    if (!tlv && !is_targ_node_addr) {
        DEBUG("rfc5444_reader: mandatory SeqNum TLV missing!\n");
        return RFC5444_DROP_PACKET;
    }

    tlv = _address_consumer_entries[RFC5444_MSGTLV_METRIC].tlv;
    if (!tlv && is_targ_node_addr) {
        DEBUG("rfc5444_reader: missing or unknown metric TLV!\n");
        return RFC5444_DROP_PACKET;
    }

    if (tlv) {
        if (!is_targ_node_addr) {
            DEBUG("rfc5444_reader: metric TLV belongs to wrong address!\n");
            return RFC5444_DROP_PACKET;
        }

        DEBUG("rfc5444_reader: RFC5444_MSGTLV_METRIC val: %d, exttype: %d\n",
               *tlv->single_value, tlv->type_ext);

        packet_data.metric_type = tlv->type_ext;
        packet_data.orig_node.metric = *tlv->single_value;
    }

    return RFC5444_OKAY;
}

static enum rfc5444_result _cb_rrep_end_callback(
        struct rfc5444_reader_tlvblock_context *cont, bool dropped)
{
    (void)cont;

    /* Check if packet contains the required information */
    if (dropped) {
        DEBUG("rfc5444_reader: dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }

    if (ipv6_addr_is_unspecified(&packet_data.orig_node.addr) ||
        !packet_data.orig_node.seqnum) {
        DEBUG("rfc5444_reader: missing OrigNode Address or SeqNum!\n");
        return RFC5444_DROP_PACKET;
    }

    if (ipv6_addr_is_unspecified(&packet_data.targ_node.addr) ||
        !packet_data.targ_node.seqnum) {
        DEBUG("rfc5444_reader: missing TargNode Address or SeqNum!\n");
        return RFC5444_DROP_PACKET;
    }

    uint8_t link_cost = aodvv2_metric_link_cost(packet_data.metric_type);

    if ((aodvv2_metric_max(packet_data.metric_type) - link_cost) <=
        packet_data.targ_node.metric) {
        DEBUG("rfc5444_reader: metric Limit reached!\n");
        return RFC5444_DROP_PACKET;
    }

    aodvv2_metric_update(packet_data.metric_type,
                         &packet_data.targ_node.metric);

    /* Update packet timestamp */
    timex_t now;
    xtimer_now_timex(&now);
    packet_data.timestamp = now;

    /* for every relevant address (RteMsg.Addr) in the RteMsg, HandlingRtr
    searches its route table to see if there is a route table entry with the
    same MetricType of the RteMsg, matching RteMsg.Addr. */

    aodvv2_local_route_t *rt_entry =
        aodvv2_lrs_get_entry(&packet_data.targ_node.addr,
                             packet_data.metric_type);

    if (!rt_entry || (rt_entry->metricType != packet_data.metric_type)) {
        DEBUG("rfc5444_reader: creating new Routing Table entry...\n");

        aodvv2_local_route_t tmp = {0};
        aodvv2_lrs_fill_routing_entry_rrep(&packet_data, &tmp, link_cost);
        aodvv2_lrs_add_entry(&tmp);

        /* Add entry to nib forwading table */
        ipv6_addr_t *dst = &packet_data.targ_node.addr;
        ipv6_addr_t *next_hop = &packet_data.sender;

        DEBUG("rfc5444_reader: adding route to NIB FT\n");
        if (gnrc_ipv6_nib_ft_add(dst, AODVV2_PREFIX_LEN, next_hop,
                                 _netif_pid, AODVV2_ROUTE_LIFETIME) < 0) {
            DEBUG("rfc5444_reader: couldn't add route!\n");
        }
    }
    else {
        if (!aodvv2_lrs_offers_improvement(rt_entry, &packet_data.targ_node)) {
            DEBUG("rfc5444_reader: RREP offers no improvement over known route.\n");
            return RFC5444_DROP_PACKET;
        }

        /* The incoming routing information is better than existing routing
         * table information and SHOULD be used to improve the route table. */
        DEBUG("rfc5444_reader: updating Routing Table entry...\n");
        aodvv2_lrs_fill_routing_entry_rrep(&packet_data, rt_entry, link_cost);

        /* Add entry to nib forwading table */
        ipv6_addr_t *dst = &rt_entry->addr;
        ipv6_addr_t *next_hop = &rt_entry->next_hop;

        gnrc_ipv6_nib_ft_del(dst, AODVV2_PREFIX_LEN);

        DEBUG("rfc5444_reader: adding route to NIB FT\n");
        if (gnrc_ipv6_nib_ft_add(dst, AODVV2_PREFIX_LEN, next_hop,
                                 _netif_pid, AODVV2_ROUTE_LIFETIME) < 0) {
            DEBUG("rfc5444_reader: couldn't add route!\n");
        }
    }

    /* If HandlingRtr is RREQ_Gen then the RREP satisfies RREQ_Gen's earlier
     * RREQ, and RREP processing is completed. Any packets buffered for
     * OrigNode should be transmitted. */
    if (aodvv2_client_find(&packet_data.orig_node.addr)) {
        DEBUG("rfc5444_reader: {%" PRIu32 ":%" PRIu32 "}\n",
              now.seconds, now.microseconds);
        DEBUG("rfc5444_reader: this is my RREP (SeqNum: %d)\n",
              packet_data.orig_node.seqnum);
        DEBUG("rfc5444_reader: We are done here, thanks!\n");

        /* Send buffered packets for this address */
        aodvv2_buffer_dispatch(&packet_data.targ_node.addr);
    }
    else {
        /* If HandlingRtr is not RREQ_Gen then the outgoing RREP is sent to the
         * Route.NextHopAddress for the RREP.AddrBlk[OrigNodeNdx]. */
        DEBUG("rfc5444_reader: not my RREP\n");
        DEBUG("rfc5444_reader: passing it on to the next hop\n");

        ipv6_addr_t *next_hop =
            aodvv2_lrs_get_next_hop(&packet_data.orig_node.addr,
                                    packet_data.metric_type);
        aodvv2_send_rrep(&packet_data, next_hop);
    }
    return RFC5444_OKAY;
}

static enum rfc5444_result _cb_rreq_blocktlv_messagetlvs_okay(
        struct rfc5444_reader_tlvblock_context *cont)
{
    if (!cont->has_hoplimit) {
        DEBUG("rfc5444_reader: missing hop limit\n");
        return RFC5444_DROP_PACKET;
    }

    packet_data.hoplimit = cont->hoplimit;
    if (packet_data.hoplimit == 0) {
        DEBUG("rfc5444_reader: Hoplimit is 0.\n");
        return RFC5444_DROP_PACKET;
    }
    packet_data.hoplimit--;
    return RFC5444_OKAY;
}

static enum rfc5444_result _cb_rreq_blocktlv_addresstlvs_okay(
        struct rfc5444_reader_tlvblock_context *cont)
{
    struct rfc5444_reader_tlvblock_entry *tlv;
    bool is_orig_node_addr = false;
    bool is_targ_node = false;

    DEBUG("rfc5444_reader: %s\n", netaddr_to_string(&nbuf, &cont->addr));

    /* handle OrigNode SeqNum TLV */
    tlv = _address_consumer_entries[RFC5444_MSGTLV_ORIGSEQNUM].tlv;
    if (tlv) {
        DEBUG("rfc5444_reader: RFC5444_MSGTLV_ORIGSEQNUM: %d\n",
              *tlv->single_value);

        is_orig_node_addr = true;
        netaddr_to_ipv6_addr(&cont->addr, &packet_data.orig_node.addr);
        packet_data.orig_node.seqnum = *tlv->single_value;
    }

    /* handle TargNode SeqNum TLV */
    tlv = _address_consumer_entries[RFC5444_MSGTLV_TARGSEQNUM].tlv;
    if (tlv) {
        DEBUG("rfc5444_reader: RFC5444_MSGTLV_TARGSEQNUM: %d\n",
              *tlv->single_value);

        is_targ_node = true;
        netaddr_to_ipv6_addr(&cont->addr, &packet_data.targ_node.addr);
        packet_data.targ_node.seqnum = *tlv->single_value;
    }

    if (!tlv && !is_orig_node_addr) {
        /* assume that tlv missing => targ_node Address */
        is_targ_node = true;
        netaddr_to_ipv6_addr(&cont->addr, &packet_data.targ_node.addr);
    }

    if (!is_orig_node_addr && !is_targ_node) {
        DEBUG("rfc5444_reader: mandatory RFC5444_MSGTLV_ORIGSEQNUM TLV missing!\n");
        return RFC5444_DROP_PACKET;
    }

    /* handle Metric TLV */
    /* cppcheck: suppress false positive on non-trivially initialized arrays.
     *           this is a known bug: http://trac.cppcheck.net/ticket/5497 */
    /* cppcheck-suppress arrayIndexOutOfBounds */
    tlv = _address_consumer_entries[RFC5444_MSGTLV_METRIC].tlv;
    if (!tlv && is_orig_node_addr) {
        DEBUG("rfc5444_reader: Missing or unknown metric TLV.\n");
        return RFC5444_DROP_PACKET;
    }

    if (tlv) {
        if (!is_orig_node_addr) {
            DEBUG("rfc5444_reader: Metric TLV belongs to wrong address.\n");
            return RFC5444_DROP_PACKET;
        }
        DEBUG("rfc5444_reader: RFC5444_MSGTLV_METRIC val: %d, exttype: %d\n",
               *tlv->single_value, tlv->type_ext);
        packet_data.metric_type = tlv->type_ext;
        packet_data.orig_node.metric = *tlv->single_value;
    }
    return RFC5444_OKAY;
}

static enum rfc5444_result _cb_rreq_end_callback(
    struct rfc5444_reader_tlvblock_context *cont, bool dropped)
{
    (void) cont;


    /* Check if packet contains the required information */
    if (dropped) {
        DEBUG("rfc5444_reader: dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }
    if (ipv6_addr_is_unspecified(&packet_data.orig_node.addr) ||
        !packet_data.orig_node.seqnum) {
        DEBUG("rfc5444_reader: missing OrigNode Address or SeqNum!\n");
        return RFC5444_DROP_PACKET;
    }
    if (ipv6_addr_is_unspecified(&packet_data.targ_node.addr)) {
        DEBUG("rfc5444_reader: missing TargNode Address!\n");
        return RFC5444_DROP_PACKET;
    }
    if (packet_data.hoplimit == 0) {
        DEBUG("rfc5444_reader: hop limit is 0!\n");
        return RFC5444_DROP_PACKET;
    }

    uint8_t link_cost = aodvv2_metric_link_cost(packet_data.metric_type);
    if ((aodvv2_metric_max(packet_data.metric_type) - link_cost) <=
        packet_data.orig_node.metric) {
        DEBUG("rfc5444_reader: metric limit reached!\n");
        return RFC5444_DROP_PACKET;
    }

    /* The incoming RREQ MUST be checked against previously received information
     * from the RREQ Table Section 7.6.  If the information in the incoming
     * RteMsg is redundant, then then no further action is taken. */
    if (aodvv2_rreqtable_is_redundant(&packet_data)) {
        DEBUG("rfc5444_reader: packet is redundant!\n");
        return RFC5444_DROP_PACKET;
    }

    aodvv2_metric_update(packet_data.metric_type, &packet_data.orig_node.metric);

    /* Update packet timestamp */
    timex_t now;
    xtimer_now_timex(&now);
    packet_data.timestamp = now;

    /* For every relevant address (RteMsg.Addr) in the RteMsg, HandlingRtr
     * searches its route table to see if there is a route table entry with the
     * same MetricType of the RteMsg, matching RteMsg.Addr.
     */
    aodvv2_local_route_t *rt_entry =
        aodvv2_lrs_get_entry(&packet_data.orig_node.addr,
                             packet_data.metric_type);

    if (!rt_entry || (rt_entry->metricType != packet_data.metric_type)) {
        DEBUG("rfc5444_reader: creating new Routing Table entry...\n");

        aodvv2_local_route_t tmp = {0};

        /* Add this RREQ to routing table*/
        aodvv2_lrs_fill_routing_entry_rreq(&packet_data, &tmp, link_cost);
        aodvv2_lrs_add_entry(&tmp);

        /* Add entry to nib forwading table */
        ipv6_addr_t *dst = &packet_data.orig_node.addr;
        ipv6_addr_t *next_hop = &packet_data.sender;

        DEBUG("rfc5444_reader: adding route to NIB FT\n");
        if (gnrc_ipv6_nib_ft_add(dst, AODVV2_PREFIX_LEN, next_hop,
                                 _netif_pid, AODVV2_ROUTE_LIFETIME) < 0) {
            DEBUG("rfc5444_reader: couldn't add route!\n");
        }
    }
    else {
        /* If the route is aready stored verify if this route offers an
         * improvement in path*/
        if (!aodvv2_lrs_offers_improvement(rt_entry, &packet_data.orig_node)) {
            DEBUG("rfc5444_reader: packet offers no improvement over known route.\n");
            return RFC5444_DROP_PACKET;
        }

        /* The incoming routing information is better than existing routing
         * table information and SHOULD be used to improve the route table. */
        DEBUG("rfc5444_reader: updating Routing Table entry...\n");
        aodvv2_lrs_fill_routing_entry_rreq(&packet_data, rt_entry, link_cost);

        /* Add entry to nib forwading table */
        ipv6_addr_t *dst = &rt_entry->addr;
        ipv6_addr_t *next_hop = &rt_entry->next_hop;

        gnrc_ipv6_nib_ft_del(dst, AODVV2_PREFIX_LEN);

        DEBUG("rfc5444_reader: adding route to NIB FT\n");
        if (gnrc_ipv6_nib_ft_add(dst, AODVV2_PREFIX_LEN, next_hop,
                                 _netif_pid, AODVV2_ROUTE_LIFETIME) < 0) {
            DEBUG("rfc5444_reader: couldn't add route!\n");
        }
    }

    /* If TargNode is a client of the router receiving the RREQ, then the
     * router generates a RREP message as specified in Section 7.4, and
     * subsequently processing for the RREQ is complete.  Otherwise,
     * processing continues as follows.
     */
    if (aodvv2_client_find(&packet_data.targ_node.addr)) {
        DEBUG("rfc5444_reader: targ_node is in client list, sending RREP\n");

        /* Make sure to start with a clean metric value */
        packet_data.targ_node.metric = 0;

        aodvv2_send_rrep(&packet_data, &packet_data.sender);
    }
    else {
        DEBUG("rfc5444_reader: i'm not targ_node, forwarding RREQ\n");
        aodvv2_send_rreq(&packet_data, &ipv6_addr_all_manet_routers_link_local);
    }

    return RFC5444_OKAY;
}

void aodvv2_rfc5444_reader_register(struct rfc5444_reader *reader,
                                    kernel_pid_t netif_pid)
{
    assert(reader != NULL && netif_pid != KERNEL_PID_UNDEF);

    if (_netif_pid == KERNEL_PID_UNDEF) {
        _netif_pid = netif_pid;
    }

    rfc5444_reader_add_message_consumer(reader, &_rrep_consumer,
                                        NULL, 0);

    rfc5444_reader_add_message_consumer(reader, &_rrep_address_consumer,
                                        _address_consumer_entries,
                                        ARRAY_SIZE(_address_consumer_entries));

    rfc5444_reader_add_message_consumer(reader, &_rreq_consumer,
                                        NULL, 0);

    rfc5444_reader_add_message_consumer(reader, &_rreq_address_consumer,
                                        _address_consumer_entries,
                                        ARRAY_SIZE(_address_consumer_entries));
}

void aodvv2_rfc5444_handle_packet_prepare(ipv6_addr_t *sender)
{
    assert(sender != NULL);

    packet_data.sender = *sender;
}
