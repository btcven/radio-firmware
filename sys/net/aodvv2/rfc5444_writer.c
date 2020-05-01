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
 * @brief       RFC5444 RREQ
 *
 * @author      Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 * @author      Gustavo Grisales <gustavosinbandera1@hotmail.com>
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 * @}
 */

#include "net/aodvv2/rfc5444.h"
#include "net/aodvv2/metric.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

static void _cb_add_message_header(struct rfc5444_writer *wr,
                                   struct rfc5444_writer_message *message);
static void _cb_rreq_add_addresses(struct rfc5444_writer *wr);
static void _cb_rrep_add_addresses(struct rfc5444_writer *wr);

/*
 * message content provider that will add message TLVs,
 * addresses and address block TLVs to all messages of type RREQ.
 */
static struct rfc5444_writer_content_provider _rreq_message_content_provider =
{
    .msg_type = RFC5444_MSGTYPE_RREQ,
    .addAddresses = _cb_rreq_add_addresses,
};

/* declaration of all address TLVs added to the RREQ message */
static struct rfc5444_writer_tlvtype _rreq_addrtlvs[] =
{
    [RFC5444_MSGTLV_ORIGSEQNUM] = { .type = RFC5444_MSGTLV_ORIGSEQNUM },
    [RFC5444_MSGTLV_METRIC] = {
        .type = RFC5444_MSGTLV_METRIC,
        .exttype = CONFIG_AODVV2_DEFAULT_METRIC
    },
};

/*
 * message content provider that will add message TLVs,
 * addresses and address block TLVs to all messages of type RREQ.
 */
static struct rfc5444_writer_content_provider _rrep_message_content_provider =
{
    .msg_type = RFC5444_MSGTYPE_RREP,
    .addAddresses = _cb_rrep_add_addresses,
};

/* declaration of all address TLVs added to the RREP message */
static struct rfc5444_writer_tlvtype _rrep_addrtlvs[] =
{
    [RFC5444_MSGTLV_ORIGSEQNUM] = { .type = RFC5444_MSGTLV_ORIGSEQNUM},
    [RFC5444_MSGTLV_TARGSEQNUM] = { .type = RFC5444_MSGTLV_TARGSEQNUM},
    [RFC5444_MSGTLV_METRIC] = {
        .type = RFC5444_MSGTLV_METRIC,
        .exttype = CONFIG_AODVV2_DEFAULT_METRIC
    },
};

static struct rfc5444_writer_message *_rreq_msg;
static struct rfc5444_writer_message *_rrep_msg;

static aodvv2_writer_target_t *_target;

static void _cb_add_message_header(struct rfc5444_writer *wr,
                                   struct rfc5444_writer_message *message)
{
    /* no originator, no hopcount, has hoplimit, no seqno */
    rfc5444_writer_set_msg_header(wr, message, false, false, true, false);
    rfc5444_writer_set_msg_hoplimit(wr, message, _target->packet_data.hoplimit);
}

static void _cb_rreq_add_addresses(struct rfc5444_writer *wr)
{
    enum rfc5444_result res;
    struct rfc5444_writer_address *orig_node_addr;
    struct rfc5444_writer_address *targ_node_addr;
    struct netaddr tmp;

    /* add orig_node address (has no address tlv); is mandatory address */
    ipv6_addr_to_netaddr(&_target->packet_data.orig_node.addr, &tmp);
    orig_node_addr =
        rfc5444_writer_add_address(wr, _rreq_message_content_provider.creator,
                                   &tmp, true);

    assert(orig_node_addr != NULL);

    /* add targ_node address (has no address tlv); is mandatory address */
    ipv6_addr_to_netaddr(&_target->packet_data.targ_node.addr, &tmp);
    targ_node_addr =
        rfc5444_writer_add_address(wr, _rreq_message_content_provider.creator,
                                   &tmp, true);

    assert(targ_node_addr != NULL);

    /* add SeqNum TLV and metric TLV to OrigNode */
    res =
        rfc5444_writer_add_addrtlv(wr, orig_node_addr,
                                   &_rreq_addrtlvs[RFC5444_MSGTLV_ORIGSEQNUM],
                                   &_target->packet_data.orig_node.seqnum,
                                   sizeof(_target->packet_data.orig_node.seqnum),
                                   false);

    assert(res == RFC5444_OKAY);

    res =
        rfc5444_writer_add_addrtlv(wr, orig_node_addr,
                                   &_rreq_addrtlvs[RFC5444_MSGTLV_METRIC],
                                   &_target->packet_data.orig_node.metric,
                                   sizeof(_target->packet_data.orig_node.metric),
                                   false);

    assert(res == RFC5444_OKAY);
}

static void _cb_rrep_add_addresses(struct rfc5444_writer *wr)
{
    enum rfc5444_result res;
    struct rfc5444_writer_address *orig_node_addr;
    struct rfc5444_writer_address *targ_node_addr;
    struct netaddr tmp;

    uint16_t orig_node_seqnum = _target->packet_data.orig_node.seqnum;
    uint16_t targ_node_seqnum = aodvv2_seqnum_get();
    aodvv2_seqnum_inc();
    uint8_t targ_node_hopct = _target->packet_data.targ_node.metric;

    /* add orig_node address (has no address tlv); is mandatory address */
    ipv6_addr_to_netaddr(&_target->packet_data.orig_node.addr, &tmp);
    orig_node_addr =
        rfc5444_writer_add_address(wr, _rrep_message_content_provider.creator,
                                   &tmp, true);

    assert(orig_node_addr != NULL);

    /* add targ_node address (has no address tlv); is mandatory address */
    ipv6_addr_to_netaddr(&_target->packet_data.targ_node.addr, &tmp);
    targ_node_addr =
        rfc5444_writer_add_address(wr, _rrep_message_content_provider.creator,
                                   &tmp, true);

    assert(targ_node_addr != NULL);

    /* add OrigNode and TargNode SeqNum TLVs */
    /* TODO: allow_dup true or false? */
    res =
        rfc5444_writer_add_addrtlv(wr, orig_node_addr,
                                   &_rrep_addrtlvs[RFC5444_MSGTLV_ORIGSEQNUM],
                                   &orig_node_seqnum, sizeof(orig_node_seqnum),
                                   false);

    assert(res == RFC5444_OKAY);

    res =
        rfc5444_writer_add_addrtlv(wr, targ_node_addr,
                                   &_rrep_addrtlvs[RFC5444_MSGTLV_TARGSEQNUM],
                                   &targ_node_seqnum, sizeof(targ_node_seqnum),
                                   false);

    assert(res == RFC5444_OKAY);

    /* Add Metric TLV to targ_node Address */
    res =
        rfc5444_writer_add_addrtlv(wr, targ_node_addr,
                                   &_rrep_addrtlvs[RFC5444_MSGTLV_METRIC],
                                   &targ_node_hopct, sizeof(targ_node_hopct),
                                   false);

    assert(res == RFC5444_OKAY);
}

void aodvv2_rfc5444_writer_register(struct rfc5444_writer *writer,
                                    aodvv2_writer_target_t *target)
{
    int res;

    assert(writer != NULL && target != NULL);

    _target = target;

    res =
        rfc5444_writer_register_msgcontentprovider(writer,
                                                   &_rreq_message_content_provider,
                                                   _rreq_addrtlvs,
                                                   ARRAY_SIZE(_rreq_addrtlvs));
    if (res < 0) {
        DEBUG("rfc5444_writer: couldn't reigster RREQ message provider\n");
        return;
    }

    res =
        rfc5444_writer_register_msgcontentprovider(writer,
                                                   &_rrep_message_content_provider,
                                                   _rrep_addrtlvs,
                                                   ARRAY_SIZE(_rrep_addrtlvs));
    if (res < 0) {
        DEBUG("rfc5444_writer: couldn't reigster RREQ message provider\n");
        return;
    }

    _rreq_msg = rfc5444_writer_register_message(writer, RFC5444_MSGTYPE_RREQ,
                                                false, RFC5444_MAX_ADDRLEN);
    if (_rreq_msg == NULL) {
        DEBUG("rfc5444_writer: couldn't reigster RREQ message\n");
        return;
    }

    _rrep_msg = rfc5444_writer_register_message(writer, RFC5444_MSGTYPE_RREP,
                                                false, RFC5444_MAX_ADDRLEN);
    if (_rrep_msg == NULL) {
        DEBUG("rfc5444_writer: couldn't reigster RREP message\n");
        return;
    }

    _rreq_msg->addMessageHeader = _cb_add_message_header;
    _rrep_msg->addMessageHeader = _cb_add_message_header;
}
