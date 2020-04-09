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
 * @ingroup     aodvv2
 * @{
 *
 * @file
 * @brief       AODVv2 routing protocol
 *
 * @author      Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 * @author      Gustavo Grisales <gustavosinbandera1@hotmail.com>
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 * @}
 */

#include "reader.h"
#include "aodvv2/client.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#include "routingtable.h"

static enum rfc5444_result _cb_rreq_blocktlv_addresstlvs_okay(
    struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_rreq_blocktlv_messagetlvs_okay(
    struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_rreq_end_callback(
    struct rfc5444_reader_tlvblock_context *cont, bool dropped);

static enum rfc5444_result _cb_rrep_blocktlv_addresstlvs_okay(
    struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_rrep_blocktlv_messagetlvs_okay(
    struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_rrep_end_callback(
    struct rfc5444_reader_tlvblock_context *cont, bool dropped);

/* helper functions */
static uint8_t _get_link_cost(aodvv2_metric_t metricType);
static uint8_t _get_max_metric(aodvv2_metric_t metricType);

/* @brief   Calculate a metric's new value according to the specified
 *          MetricType (currently only implemented for
 *          AODVV2_DEFAULT_METRIC_TYPE (HopCt))
 */
static void _update_metric(aodvv2_metric_t metricType, uint8_t *metric);

/* This is where we store data gathered from packets */
static aodvv2_packet_data_t packet_data;
//static struct unreachable_node unreachable_nodes[AODVV2_MAX_UNREACHABLE_NODES];
//static int num_unreachable_nodes;

static struct rfc5444_reader reader;
static struct netaddr_str nbuf;

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
static struct rfc5444_reader_tlvblock_consumer_entry _rreq_rrep_address_consumer_entries[] =
{
    [RFC5444_MSGTLV_ORIGSEQNUM] = { .type = RFC5444_MSGTLV_ORIGSEQNUM},
    [RFC5444_MSGTLV_TARGSEQNUM] = { .type = RFC5444_MSGTLV_TARGSEQNUM},
    [RFC5444_MSGTLV_METRIC] = { .type = RFC5444_MSGTLV_METRIC }
};

/**
 * This block callback is called for every address
 *
 * @param cont
 * @return
 */
static enum rfc5444_result _cb_rreq_blocktlv_messagetlvs_okay(struct rfc5444_reader_tlvblock_context *cont)
{
    DEBUG("%s()\n", __func__);

    if (!cont->has_hoplimit) {
        DEBUG("\tERROR: missing hop limit\n");
        return RFC5444_DROP_PACKET;
    }

    packet_data.hoplimit = cont->hoplimit;
    if (packet_data.hoplimit == 0) {
        DEBUG("\tERROR: Hoplimit is 0.\n");
        return RFC5444_DROP_PACKET;
    }
    packet_data.hoplimit--;
    return RFC5444_OKAY;
}

/**
 * This block callback is called for every address of a RREQ Message.
 *
 * @param cont
 * @return
 */
static enum rfc5444_result _cb_rreq_blocktlv_addresstlvs_okay(struct rfc5444_reader_tlvblock_context *cont)
{
    struct rfc5444_reader_tlvblock_entry *tlv;
    bool is_origNode_addr = false;
    bool is_targNode_addr = false;

    DEBUG("\taddr: %s\n", netaddr_to_string(&nbuf, &cont->addr));

    /* handle OrigNode SeqNum TLV */
    tlv = _rreq_rrep_address_consumer_entries[RFC5444_MSGTLV_ORIGSEQNUM].tlv;
    if (tlv) {
        DEBUG("\ttlv RFC5444_MSGTLV_ORIGSEQNUM: %d\n", *tlv->single_value);
        is_origNode_addr = true;
        packet_data.origNode.addr = cont->addr;
        packet_data.origNode.seqnum = *tlv->single_value;
    }

    /* handle TargNode SeqNum TLV */
    tlv = _rreq_rrep_address_consumer_entries[RFC5444_MSGTLV_TARGSEQNUM].tlv;
    if (tlv) {
        DEBUG("\ttlv RFC5444_MSGTLV_TARGSEQNUM: %d\n", *tlv->single_value);
        is_targNode_addr = true;
        packet_data.targNode.addr = cont->addr;
        packet_data.targNode.seqnum = *tlv->single_value;
    }
    if (!tlv && !is_origNode_addr) {
        /* assume that tlv missing => targNode Address */
        is_targNode_addr = true;
        packet_data.targNode.addr = cont->addr;
    }
    if (!is_origNode_addr && !is_targNode_addr) {
        DEBUG("\tERROR: mandatory RFC5444_MSGTLV_ORIGSEQNUM TLV missing.\n");
        return RFC5444_DROP_PACKET;
    }

    /* handle Metric TLV */
    /* cppcheck: suppress false positive on non-trivially initialized arrays.
     *           this is a known bug: http://trac.cppcheck.net/ticket/5497 */
    /* cppcheck-suppress arrayIndexOutOfBounds */
    tlv = _rreq_rrep_address_consumer_entries[RFC5444_MSGTLV_METRIC].tlv;
    if (!tlv && is_origNode_addr) {
        DEBUG("\tERROR: Missing or unknown metric TLV.\n");
        return RFC5444_DROP_PACKET;
    }
    if (tlv) {
        if (!is_origNode_addr) {
            DEBUG("\tERROR: Metric TLV belongs to wrong address.\n");
            return RFC5444_DROP_PACKET;
        }
        DEBUG("\ttlv RFC5444_MSGTLV_METRIC val: %d, exttype: %d\n",
               *tlv->single_value, tlv->type_ext);
        packet_data.metricType = tlv->type_ext;
        packet_data.origNode.metric = *tlv->single_value;
    }
    return RFC5444_OKAY;
}

/**
 * This callback is called every time the _rreq_consumer finishes reading a
 * packet.
 * @param cont
 * @param dropped indicates whether the packet has been dropped previously by
 *                another callback
 */
static enum rfc5444_result _cb_rreq_end_callback(
    struct rfc5444_reader_tlvblock_context *cont, bool dropped)
{
    (void) cont;

    aodvv2_routing_entry_t *rt_entry;
    timex_t now;
    uint8_t link_cost = _get_link_cost(packet_data.metricType);

    /* Check if packet contains the required information */
    if (dropped) {
        DEBUG("\t Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }
    if ((packet_data.origNode.addr._type == AF_UNSPEC) || !packet_data.origNode.seqnum) {
        DEBUG("\tERROR: missing OrigNode Address or SeqNum. Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }
    if (packet_data.targNode.addr._type == AF_UNSPEC) {
        DEBUG("\tERROR: missing TargNode Address. Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }
    if (packet_data.hoplimit == 0) {
        DEBUG("\tERROR: Hoplimit is 0. Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }
    if ((_get_max_metric(packet_data.metricType) - link_cost)
        <= packet_data.origNode.metric) {
        DEBUG("\tMetric Limit reached. Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }

    /* The incoming RREQ MUST be checked against previously received information
     * from the RREQ Table Section 7.6.  If the information in the incoming
     * RteMsg is redundant, then then no further action is taken. */
    if (aodvv2_rreqtable_is_redundant(&packet_data)) {
        DEBUG("\tPacket is redundant. Dropping Packet. %i\n", RFC5444_DROP_PACKET);
        return RFC5444_DROP_PACKET;
    }

    _update_metric(packet_data.metricType, &packet_data.origNode.metric);
    xtimer_now_timex(&now);
    packet_data.timestamp = now;

    /* For every relevant address (RteMsg.Addr) in the RteMsg, HandlingRtr
     * searches its route table to see if there is a route table entry with the
     * same MetricType of the RteMsg, matching RteMsg.Addr.
     */
    rt_entry = aodvv2_routingtable_get_entry(&packet_data.origNode.addr,
                                             packet_data.metricType);

    if (!rt_entry || (rt_entry->metricType != packet_data.metricType)) {
        DEBUG("Creating new Routing Table entry...\n");

        aodvv2_routing_entry_t tmp = {0};

        /* Add this RREQ to routing table*/
        aodvv2_routingtable_fill_routing_entry_rreq(&packet_data, &tmp, link_cost);
        aodvv2_routingtable_add_entry(&tmp);
    }
    else {
        /* If the route is aready stored verify if this route offers an
         * improvement in path*/
        if (!aodvv2_routingtable_offers_improvement(rt_entry,
                                                    &packet_data.origNode)) {
            DEBUG("Packet offers no improvement over known route. Dropping Packet.\n");
            return RFC5444_DROP_PACKET;
        }

        /* The incoming routing information is better than existing routing
         * table information and SHOULD be used to improve the route table. */
        DEBUG("Updating Routing Table entry...\n");
        aodvv2_routingtable_fill_routing_entry_rreq(&packet_data, rt_entry,
                                                    link_cost);
    }

    /* If TargNode is a client of the router receiving the RREQ, then the
     * router generates a RREP message as specified in Section 7.4, and
     * subsequently processing for the RREQ is complete.  Otherwise,
     * processing continues as follows.
     */
    ipv6_addr_t tmp;
    netaddr_to_ipv6_addr(&packet_data.targNode.addr, &tmp);
    if (aodvv2_client_find(&tmp)) {
        DEBUG("TargNode is in client list, sending RREP\n");

        /* Make sure to start with a clean metric value */
        packet_data.targNode.metric = 0;
        aodvv2_send_rrep(&packet_data, &packet_data.sender);
    }
    else {
        DEBUG("I am not TargNode, forwarding RREQ\n");
        aodvv2_send_rreq(&packet_data);
    }

    return RFC5444_OKAY;
}

/**
 * This block callback is called for every address
 *
 * @param cont
 * @return
 */
static enum rfc5444_result _cb_rrep_blocktlv_messagetlvs_okay(struct rfc5444_reader_tlvblock_context *cont)
{
    if (!cont->has_hoplimit) {
        DEBUG("\tERROR: missing hop limit\n");
        return RFC5444_DROP_PACKET;
    }

    packet_data.hoplimit = cont->hoplimit;
    if (packet_data.hoplimit == 0) {
        DEBUG("\tERROR: Hoplimit is 0.\n");
        return RFC5444_DROP_PACKET;
    }

    packet_data.hoplimit--;
    return RFC5444_OKAY;
}

/**
 * This block callback is called for every address of a RREP Message.
 *
 * @param cont
 * @return
 */
static enum rfc5444_result _cb_rrep_blocktlv_addresstlvs_okay(struct rfc5444_reader_tlvblock_context *cont)
{
    struct netaddr_str nbuf;
    struct rfc5444_reader_tlvblock_entry *tlv;
    bool is_targNode_addr = false;

    DEBUG("\taddr: %s\n", netaddr_to_string(&nbuf, &cont->addr));

    /* handle TargNode SeqNum TLV */
    tlv = _rreq_rrep_address_consumer_entries[RFC5444_MSGTLV_TARGSEQNUM].tlv;
    if (tlv) {
        DEBUG("\ttlv RFC5444_MSGTLV_TARGSEQNUM: %d\n", *tlv->single_value);
        is_targNode_addr = true;
        packet_data.targNode.addr = cont->addr;
        packet_data.targNode.seqnum = *tlv->single_value;
    }

    /* handle OrigNode SeqNum TLV */
    tlv = _rreq_rrep_address_consumer_entries[RFC5444_MSGTLV_ORIGSEQNUM].tlv;
    if (tlv) {
        DEBUG("\ttlv RFC5444_MSGTLV_ORIGSEQNUM: %d\n", *tlv->single_value);
        is_targNode_addr = false;
        packet_data.origNode.addr = cont->addr;
        packet_data.origNode.seqnum = *tlv->single_value;
    }
    if (!tlv && !is_targNode_addr) {
        DEBUG("\tERROR: mandatory SeqNum TLV missing.\n");
        return RFC5444_DROP_PACKET;
    }

    /* handle Metric TLV */
    /* cppcheck: suppress false positive on non-trivially initialized arrays.
     *           this is a known bug: http://trac.cppcheck.net/ticket/5497 */
    /* cppcheck-suppress arrayIndexOutOfBounds */
    tlv = _rreq_rrep_address_consumer_entries[RFC5444_MSGTLV_METRIC].tlv;
    if (!tlv && is_targNode_addr) {
        DEBUG("\tERROR: Missing or unknown metric TLV.\n");
        return RFC5444_DROP_PACKET;
    }
    if (tlv) {
        if (!is_targNode_addr) {
            DEBUG("\tERROR: metric TLV belongs to wrong address.\n");
            return RFC5444_DROP_PACKET;
        }
        DEBUG("\ttlv RFC5444_MSGTLV_METRIC val: %d, exttype: %d\n",
               *tlv->single_value, tlv->type_ext);
        packet_data.metricType = tlv->type_ext;
        packet_data.origNode.metric = *tlv->single_value;
    }
    return RFC5444_OKAY;
}

/**
 * This callback is called every time the _rreq_consumer finishes reading a
 * packet.
 * @param cont
 * @param dropped indicates wehther the packet has been dropped previously by
 *                another callback
 */
static enum rfc5444_result _cb_rrep_end_callback(
    struct rfc5444_reader_tlvblock_context *cont, bool dropped)
{
    (void) cont;

    aodvv2_routing_entry_t *rt_entry;
    struct netaddr_str nbuf;
    timex_t now;
    uint8_t link_cost = _get_link_cost(packet_data.metricType);

    /* Check if packet contains the required information */
    if (dropped) {
        DEBUG("\t Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }
    if ((packet_data.origNode.addr._type == AF_UNSPEC)
        || !packet_data.origNode.seqnum) {
        DEBUG("\tERROR: missing OrigNode Address or SeqNum. Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }
    if ((packet_data.targNode.addr._type == AF_UNSPEC)
        || !packet_data.targNode.seqnum) {
        DEBUG("\tERROR: missing TargNode Address or SeqNum. Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }
    if ((_get_max_metric(packet_data.metricType) - link_cost)
        <= packet_data.targNode.metric) {
        DEBUG("\tMetric Limit reached. Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }

    _update_metric(packet_data.metricType, &packet_data.targNode.metric);
    xtimer_now_timex(&now);
    packet_data.timestamp = now;

    /* for every relevant address (RteMsg.Addr) in the RteMsg, HandlingRtr
    searches its route table to see if there is a route table entry with the
    same MetricType of the RteMsg, matching RteMsg.Addr. */

    rt_entry = aodvv2_routingtable_get_entry(&packet_data.targNode.addr,
                                             packet_data.metricType);

    if (!rt_entry || (rt_entry->metricType != packet_data.metricType)) {
        DEBUG("\tCreating new Routing Table entry...\n");

        aodvv2_routing_entry_t tmp = {0};
        aodvv2_routingtable_fill_routing_entry_rrep(&packet_data,
                                                    &tmp,
                                                    link_cost);
        aodvv2_routingtable_add_entry(&tmp);
    }
    else {
        if (!aodvv2_routingtable_offers_improvement(rt_entry,
                                                    &packet_data.targNode)) {
            DEBUG("\tPacket offers no improvement over known route. Dropping Packet.\n");
            return RFC5444_DROP_PACKET;
        }

        /* The incoming routing information is better than existing routing
         * table information and SHOULD be used to improve the route table. */
        DEBUG("\tUpdating Routing Table entry...\n");
        aodvv2_routingtable_fill_routing_entry_rrep(&packet_data,
                                                    rt_entry,
                                                    link_cost);
    }

    /* If HandlingRtr is RREQ_Gen then the RREP satisfies RREQ_Gen's earlier
     * RREQ, and RREP processing is completed. Any packets buffered for
     * OrigNode should be transmitted. */
    ipv6_addr_t tmp;
    netaddr_to_ipv6_addr(&packet_data.origNode.addr, &tmp);
    if (aodvv2_client_find(&tmp)) {
#ifdef ENABLE_DEBUG
        static struct netaddr_str nbuf2;
#endif

        DEBUG("\t{%" PRIu32 ":%" PRIu32 "} %s:  This is my RREP (SeqNum: %d). We are done here, thanks %s!\n",
              now.seconds, now.microseconds, netaddr_to_string(&nbuf, &packet_data.origNode.addr),
              packet_data.origNode.seqnum, netaddr_to_string(&nbuf2, &packet_data.targNode.addr));
    }
    else {
        /* If HandlingRtr is not RREQ_Gen then the outgoing RREP is sent to the
         * Route.NextHopAddress for the RREP.AddrBlk[OrigNodeNdx]. */
        DEBUG("Not my RREP, passing it on to the next hop\n");
        aodvv2_send_rrep(&packet_data,
                         aodvv2_routingtable_get_next_hop(&packet_data.origNode.addr,
                                                          packet_data.metricType));
    }
    return RFC5444_OKAY;
}

void aodvv2_packet_reader_init(void)
{
    /* initialize reader */
    rfc5444_reader_init(&reader);

    /* register message consumers. We have no message TLVs, so we can leave the
     * rfc5444_reader_tlvblock_consumer_entry empty */
    rfc5444_reader_add_message_consumer(&reader, &_rreq_consumer,
                                        NULL, 0);
    rfc5444_reader_add_message_consumer(&reader, &_rrep_consumer,
                                        NULL, 0);

    /* register address consumer */
    rfc5444_reader_add_message_consumer(&reader, &_rreq_address_consumer,
                                        _rreq_rrep_address_consumer_entries,
                                        ARRAYSIZE(_rreq_rrep_address_consumer_entries));
    rfc5444_reader_add_message_consumer(&reader, &_rrep_address_consumer,
                                        _rreq_rrep_address_consumer_entries,
                                        ARRAYSIZE(_rreq_rrep_address_consumer_entries));
}

void aodvv2_packet_reader_cleanup(void)
{
    rfc5444_reader_cleanup(&reader);
}

int aodvv2_packet_reader_handle_packet(void *buffer, size_t length, struct netaddr *sender)
{
    memcpy(&packet_data.sender, sender, sizeof(*sender));
    DEBUG("\t sender: %s\n", netaddr_to_string(&nbuf, &packet_data.sender));

    return rfc5444_reader_handle_packet(&reader, buffer, length);
}

/*============= HELPER FUNCTIONS =============================================*/

/*
 * Cost(L): Get Cost of a Link regarding the specified metric.
 * (currently only AODVV2_DEFAULT_METRIC_TYPE (HopCt) implemented)
 * returns cost if metric is known, NULL otherwise
 */
static uint8_t _get_link_cost(aodvv2_metric_t metricType)
{
    if (metricType == AODVV2_DEFAULT_METRIC_TYPE) {
        return 1;
    }
    return 0;
}

/*
 * MAX_METRIC[MetricType]:
 * returns maximum value of the given metric if metric is known, NULL otherwise.
 */
static uint8_t _get_max_metric(aodvv2_metric_t metricType)
{
    if (metricType == AODVV2_DEFAULT_METRIC_TYPE) {
        return AODVV2_MAX_HOPCOUNT;
    }
    return 0;
}

static void _update_metric(aodvv2_metric_t metricType, uint8_t *metric)
{
    if (metricType == AODVV2_DEFAULT_METRIC_TYPE){
        *metric = *metric + 1;
    }
}
