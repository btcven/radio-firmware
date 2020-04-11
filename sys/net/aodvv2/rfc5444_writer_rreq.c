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

#define ENABLE_DEBUG (1)
#include "debug.h"

static void _cb_add_message_header(struct rfc5444_writer *wr,
                                   struct rfc5444_writer_message *message);
static void _cb_rreq_add_addresses(struct rfc5444_writer *wr);

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

static struct rfc5444_writer_message *_rreq_msg;

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

    /* add orig_node address (has no address tlv); is mandatory address */
    orig_node_addr =
        rfc5444_writer_add_address(wr, _rreq_message_content_provider.creator,
                                   &_target->packet_data.orig_node.addr, true);

    if (orig_node_addr == NULL) {
        DEBUG("rfc5444_writer_rreq: couldn't add OrigNode address\n");
        return;
    }

    /* add targ_node address (has no address tlv); is mandatory address */
    targ_node_addr =
        rfc5444_writer_add_address(wr, _rreq_message_content_provider.creator,
                                   &_target->packet_data.targ_node.addr, true);

    if (targ_node_addr == NULL) {
        DEBUG("rfc5444_writer_rreq: couldn't add TargNode address\n");
        return;
    }

    /* add SeqNum TLV and metric TLV to OrigNode */
    res =
        rfc5444_writer_add_addrtlv(wr, orig_node_addr,
                                   &_rreq_addrtlvs[RFC5444_MSGTLV_ORIGSEQNUM],
                                   &_target->packet_data.orig_node.seqnum,
                                   sizeof(_target->packet_data.orig_node.seqnum),
                                   false);
    if (res != RFC5444_OKAY) {
        DEBUG("rfc5444_writer_rreq: couldn't add SeqNum to OrigNode.\n");
        return;
    }

    res =
        rfc5444_writer_add_addrtlv(wr, orig_node_addr,
                                   &_rreq_addrtlvs[RFC5444_MSGTLV_METRIC],
                                   &_target->packet_data.orig_node.metric,
                                   sizeof(_target->packet_data.orig_node.metric),
                                   false);

    if (res != RFC5444_OKAY) {
        DEBUG("rfc5444_writer_rreq: couldn't add Metric to OrigNode.\n");
        return;
    }
}

void aodvv2_rfc5444_writer_rreq_register(struct rfc5444_writer *writer,
                                         aodvv2_writer_target_t *target)
{
    assert(writer != NULL && target != NULL);

    _target = target;

    int res =
        rfc5444_writer_register_msgcontentprovider(writer,
                                                   &_rreq_message_content_provider,
                                                   _rreq_addrtlvs,
                                                   ARRAY_SIZE(_rreq_addrtlvs));
    if (res < 0) {
        DEBUG("rfc5444_writer_rreq: couldn't reigster RREQ message provider\n");
        return;
    }

    _rreq_msg = rfc5444_writer_register_message(writer, RFC5444_MSGTYPE_RREQ,
                                                false, RFC5444_MAX_ADDRLEN);
    if (_rreq_msg == NULL) {
        DEBUG("rfc5444_writer_rreq: couldn't reigster RREQ message\n");
        return;
    }

    _rreq_msg->addMessageHeader = _cb_add_message_header;
}
