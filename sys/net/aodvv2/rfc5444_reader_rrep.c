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
#include "net/aodvv2/routingtable.h"

#include "xtimer.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

static enum rfc5444_result _cb_rrep_blocktlv_addresstlvs_okay(
    struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_rrep_blocktlv_messagetlvs_okay(
    struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_rrep_end_callback(
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
 * Address consumer entries definition
 * TLV types RFC5444_MSGTLV__SEQNUM and RFC5444_MSGTLV_METRIC
 */
static struct rfc5444_reader_tlvblock_consumer_entry _rrep_address_consumer_entries[] =
{
    [RFC5444_MSGTLV_ORIGSEQNUM] = { .type = RFC5444_MSGTLV_ORIGSEQNUM },
    [RFC5444_MSGTLV_TARGSEQNUM] = { .type = RFC5444_MSGTLV_TARGSEQNUM },
    [RFC5444_MSGTLV_METRIC] = { .type = RFC5444_MSGTLV_METRIC }
};

static struct netaddr_str nbuf;
static aodvv2_packet_data_t packet_data;

static enum rfc5444_result _cb_rrep_blocktlv_messagetlvs_okay(
        struct rfc5444_reader_tlvblock_context *cont)
{
    if (!cont->has_hoplimit) {
        DEBUG("rfc5444_reader_rrep: missing hop limit\n");
        return RFC5444_DROP_PACKET;
    }

    packet_data.hoplimit = cont->hoplimit;
    if (packet_data.hoplimit == 0) {
        DEBUG("rfc5444_reader_rrep: hop limit is 0.\n");
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

    DEBUG("rfc5444_reader_rrep: %s\n", netaddr_to_string(&nbuf, &cont->addr));

    /* handle TargNode SeqNum TLV */
    tlv = _rrep_address_consumer_entries[RFC5444_MSGTLV_TARGSEQNUM].tlv;
    if (tlv) {
        DEBUG("rfc5444_reader_rrep: RFC5444_MSGTLV_TARGSEQNUM: %d\n",
              *tlv->single_value);
        is_targ_node_addr = true;
        packet_data.targ_node.addr = cont->addr;
        packet_data.targ_node.seqnum = *tlv->single_value;
    }

    /* handle OrigNode SeqNum TLV */
    tlv = _rrep_address_consumer_entries[RFC5444_MSGTLV_ORIGSEQNUM].tlv;
    if (tlv) {
        DEBUG("rfc5444_reader_rrep: RFC5444_MSGTLV_ORIGSEQNUM: %d\n",
              *tlv->single_value);
        is_targ_node_addr = false;
        packet_data.orig_node.addr = cont->addr;
        packet_data.orig_node.seqnum = *tlv->single_value;
    }

    if (!tlv && !is_targ_node_addr) {
        DEBUG("rfc5444_reader_rrep: mandatory SeqNum TLV missing!\n");
        return RFC5444_DROP_PACKET;
    }

    tlv = _rrep_address_consumer_entries[RFC5444_MSGTLV_METRIC].tlv;
    if (!tlv && is_targ_node_addr) {
        DEBUG("rfc5444_reader_rrep: missing or unknown metric TLV!\n");
        return RFC5444_DROP_PACKET;
    }

    if (tlv) {
        if (!is_targ_node_addr) {
            DEBUG("rfc5444_reader_rrep: metric TLV belongs to wrong address!\n");
            return RFC5444_DROP_PACKET;
        }

        DEBUG("rfc5444_reader_rrep: RFC5444_MSGTLV_METRIC val: %d, exttype: %d\n",
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
        DEBUG("rfc5444_reader_rrep: dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }

    if ((packet_data.orig_node.addr._type == AF_UNSPEC) ||
        !packet_data.orig_node.seqnum) {
        DEBUG("rfc5444_reader_rrep: missing OrigNode Address or SeqNum!\n");
        return RFC5444_DROP_PACKET;
    }

    if ((packet_data.targ_node.addr._type == AF_UNSPEC) ||
        !packet_data.targ_node.seqnum) {
        DEBUG("rfc5444_reader_rrep: missing TargNode Address or SeqNum!\n");
        return RFC5444_DROP_PACKET;
    }

    uint8_t link_cost = aodvv2_metric_link_cost(packet_data.metric_type);

    if ((aodvv2_metric_max(packet_data.metric_type) - link_cost) <=
        packet_data.targ_node.metric) {
        DEBUG("rfc5444_reader_rrep: metric Limit reached!\n");
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

    aodvv2_routing_entry_t *rt_entry =
        aodvv2_routingtable_get_entry(&packet_data.targ_node.addr,
                                      packet_data.metric_type);

    if (!rt_entry || (rt_entry->metricType != packet_data.metric_type)) {
        DEBUG("rfc5444_reader_rrep: creating new Routing Table entry...\n");

        aodvv2_routing_entry_t tmp = {0};
        aodvv2_routingtable_fill_routing_entry_rrep(&packet_data,
                                                    &tmp,
                                                    link_cost);
        aodvv2_routingtable_add_entry(&tmp);
    }
    else {
        if (!aodvv2_routingtable_offers_improvement(rt_entry,
                                                    &packet_data.targ_node)) {
            DEBUG("rfc5444_reader_rrep: RREP offers no improvement over known route.\n");
            return RFC5444_DROP_PACKET;
        }

        /* The incoming routing information is better than existing routing
         * table information and SHOULD be used to improve the route table. */
        DEBUG("rfc5444_reader_rrep: updating Routing Table entry...\n");
        aodvv2_routingtable_fill_routing_entry_rrep(&packet_data,
                                                    rt_entry,
                                                    link_cost);
    }

    /* If HandlingRtr is RREQ_Gen then the RREP satisfies RREQ_Gen's earlier
     * RREQ, and RREP processing is completed. Any packets buffered for
     * OrigNode should be transmitted. */
    ipv6_addr_t tmp;
    netaddr_to_ipv6_addr(&packet_data.orig_node.addr, &tmp);
    if (aodvv2_client_find(&tmp)) {
        DEBUG("rfc5444_reader_rrep: {%" PRIu32 ":%" PRIu32 "}\n",
              now.seconds, now.microseconds);
        DEBUG("rfc5444_reader_rrep: %s:  this is my RREP (SeqNum: %d)\n",
              netaddr_to_string(&nbuf, &packet_data.orig_node.addr),
              packet_data.orig_node.seqnum);
        DEBUG("rfc5444_reader_rrep: We are done here, thanks %s!\n",
              netaddr_to_string(&nbuf, &packet_data.targ_node.addr));
    }
    else {
        /* If HandlingRtr is not RREQ_Gen then the outgoing RREP is sent to the
         * Route.NextHopAddress for the RREP.AddrBlk[OrigNodeNdx]. */
        DEBUG("rfc5444_reader_rrep: not my RREP\n");
        DEBUG("rfc5444_reader_rrep: passing it on to the next hop\n");

        /*aodvv2_send_rrep(&packet_data,
                         aodvv2_routingtable_get_next_hop(&packet_data.orig_node.addr,
                                                          packet_data.metric_type));
                                                          */
    }
    return RFC5444_OKAY;
}

void aodvv2_rfc5444_reader_rrep_register(struct rfc5444_reader *reader)
{
    assert(reader != NULL);

    rfc5444_reader_add_message_consumer(reader, &_rrep_consumer,
                                        NULL, 0);

    rfc5444_reader_add_message_consumer(reader, &_rrep_address_consumer,
                                        _rrep_address_consumer_entries,
                                        ARRAY_SIZE(_rrep_address_consumer_entries));
}
