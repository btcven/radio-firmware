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

#include <errno.h>

#include "_aodvv2-writer.h"

#include "net/aodvv2/metric.h"
#include "net/aodvv2/msg.h"

#include "rfc5444/rfc5444_iana.h"

#include "mutex.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

enum {
    IDX_ADDRTLV_PATH_METRIC = 0,
    IDX_ADDRTLV_SEQ_NUM,
    IDX_ADDRTLV_ADDRESS_TYPE,
};

static int _add_message_header(struct rfc5444_writer *wr, struct rfc5444_writer_message *message);
static void _rrep_add_addrtlvs(struct rfc5444_writer *wr);
static void _rreq_add_addrtlvs(struct rfc5444_writer *wr);
static void _rrep_ack_add_msgtlvs(struct rfc5444_writer *writer);

static struct rfc5444_writer_content_provider _rreq_message_content_provider =
{
    .msg_type = AODVV2_MSGTYPE_RREQ,
    .addAddresses = _rreq_add_addrtlvs,
};

static struct rfc5444_writer_tlvtype _rreq_addrtlvs[] =
{
    [IDX_ADDRTLV_PATH_METRIC] = {
        .type = AODVV2_ADDRTLV_PATH_METRIC,
        .exttype =  AODVV2_METRIC_TYPE_HOP_COUNT
    },
    [IDX_ADDRTLV_SEQ_NUM] = {
        .type = AODVV2_ADDRTLV_SEQ_NUM,
    },
    [IDX_ADDRTLV_ADDRESS_TYPE] = {
        .type = AODVV2_ADDRTLV_ADDRESS_TYPE,
    },
};

static struct rfc5444_writer_content_provider _rrep_message_content_provider =
{
    .msg_type = AODVV2_MSGTYPE_RREP,
    .addAddresses = _rrep_add_addrtlvs,
};

static struct rfc5444_writer_tlvtype _rrep_addrtlvs[] =
{
    [IDX_ADDRTLV_PATH_METRIC] = {
        .type = AODVV2_ADDRTLV_PATH_METRIC,
        .exttype =  AODVV2_METRIC_TYPE_HOP_COUNT
    },
    [IDX_ADDRTLV_SEQ_NUM] = {
        .type = AODVV2_ADDRTLV_SEQ_NUM,
    },
    [IDX_ADDRTLV_ADDRESS_TYPE] = {
        .type = AODVV2_ADDRTLV_ADDRESS_TYPE,
    },
};

static struct rfc5444_writer_content_provider _rrep_ack_message_content_provider =
{
    .msg_type = AODVV2_MSGTYPE_RREP_ACK,
    .addMessageTLVs = _rrep_ack_add_msgtlvs,
};


static struct rfc5444_writer_message *_rreq_msg;
static struct rfc5444_writer_message *_rrep_msg;
static struct rfc5444_writer_message *_rrep_ack_msg;

static aodvv2_message_t _msg;

static char addr_str[IPV6_ADDR_MAX_STR_LEN];

static mutex_t _lock = MUTEX_INIT;

static inline uint8_t _normalize_pfx_len(uint8_t pfx_len)
{
    return (pfx_len == 0 || pfx_len > 128) ? 128 : pfx_len;
}

static int _add_message_header(struct rfc5444_writer *writer, struct rfc5444_writer_message *msg)
{
    DEBUG("aodvv2: adding message header for %s\n",
          (msg->type == AODVV2_MSGTYPE_RREQ) ? "RREQ" : "RREP");

    /* no originator, no hopcount, has msg_hop_limit, no seqno */
    rfc5444_writer_set_msg_header(writer, msg, false, false, true, false);
    if (msg->type == AODVV2_MSGTYPE_RREQ) {
        rfc5444_writer_set_msg_hoplimit(writer, msg, _msg.rreq.msg_hop_limit);
    }
    else if (msg->type == AODVV2_MSGTYPE_RREP) {
        rfc5444_writer_set_msg_hoplimit(writer, msg, _msg.rrep.msg_hop_limit);
    }

    return 0;
}

static void _rreq_add_addrtlvs(struct rfc5444_writer *writer)
{
    struct rfc5444_writer_address *orig_prefix;
    struct rfc5444_writer_address *targ_prefix;
    struct rfc5444_writer_address *seqnortr;
    struct netaddr tmp;
    uint8_t pfx_len;
    uint8_t address_type;
    aodvv2_msg_rreq_t *rreq = &_msg.rreq;

    DEBUG("aodvv2: adding RREQ Address/TLVs\n");
    DEBUG("  OrigPrefix = %s/%d\n",
          ipv6_addr_to_str(addr_str, &rreq->orig_prefix, sizeof(addr_str)),
          rreq->orig_pfx_len);
    DEBUG("  TargPrefix = %s\n",
          ipv6_addr_to_str(addr_str, &rreq->targ_prefix, sizeof(addr_str)));


    /* Add OrigPrefix address */
    pfx_len = _normalize_pfx_len(rreq->orig_pfx_len);
    ipv6_addr_to_netaddr(&rreq->orig_prefix, pfx_len, &tmp);
    orig_prefix = rfc5444_writer_add_address(writer, _rreq_message_content_provider.creator, &tmp, true);
    if (orig_prefix == NULL) {
        DEBUG("  couldn't add OrigPrefix\n");
        return;
    }

    /* Add TargPrefix address */
    pfx_len = 128;
    ipv6_addr_to_netaddr(&rreq->targ_prefix, pfx_len, &tmp);
    targ_prefix = rfc5444_writer_add_address(writer, _rreq_message_content_provider.creator, &tmp, true);
    if (targ_prefix == NULL) {
        DEBUG("  couldn't add TargPrefix\n");
        return;
    }

    /* Add SeqNoRtr address */
    if (!ipv6_addr_is_unspecified(&rreq->seqnortr)) {
        DEBUG("  SeqNoRtr = %s",
              ipv6_addr_to_str(addr_str, &rreq->seqnortr, sizeof(addr_str)));
        pfx_len = 128;
        ipv6_addr_to_netaddr(&rreq->seqnortr, pfx_len, &tmp);
        seqnortr = rfc5444_writer_add_address(writer, _rreq_message_content_provider.creator, &tmp, true);
        if (seqnortr == NULL) {
            DEBUG("  couldn't add SeqNoRtr\n");
            return;
        }
    }

    /* Add ADDRESS_TYPE, SEQ_NUM and PATH_METRIC TLVs to OrigPrefix */
    address_type = AODVV2_ADDRTYPE_ORIGPREFIX;
    rfc5444_writer_add_addrtlv(writer, orig_prefix, &_rreq_addrtlvs[IDX_ADDRTLV_ADDRESS_TYPE], &address_type,
                               sizeof(uint8_t), false);

    rfc5444_writer_add_addrtlv(writer, orig_prefix, &_rreq_addrtlvs[IDX_ADDRTLV_SEQ_NUM], &rreq->orig_seqnum,
                               sizeof(aodvv2_seqnum_t), false);

    rfc5444_writer_add_addrtlv(writer, orig_prefix, &_rreq_addrtlvs[IDX_ADDRTLV_PATH_METRIC], &rreq->orig_metric,
                               sizeof(uint8_t), false);

    /* Add ADDRESS_TYPE and SEQNUM TLVs to TargPrefix */
    address_type = AODVV2_ADDRTYPE_TARGPREFIX;
    rfc5444_writer_add_addrtlv(writer, targ_prefix, &_rreq_addrtlvs[IDX_ADDRTLV_ADDRESS_TYPE], &address_type,
                               sizeof(uint8_t), false);

    if (rreq->targ_seqnum != 0) {
        rfc5444_writer_add_addrtlv(writer, targ_prefix, &_rreq_addrtlvs[IDX_ADDRTLV_SEQ_NUM], &rreq->targ_seqnum,
                                   sizeof(aodvv2_seqnum_t), false);
    }
}

static void _rrep_add_addrtlvs(struct rfc5444_writer *writer)
{
    struct rfc5444_writer_address *orig_prefix;
    struct rfc5444_writer_address *targ_prefix;
    struct rfc5444_writer_address *seqnortr;
    struct netaddr tmp;
    uint8_t pfx_len;
    uint8_t address_type;
    aodvv2_msg_rrep_t *rrep = &_msg.rrep;

    DEBUG("aodvv2: adding RREP Address/TLVs");
    DEBUG("  OrigPrefix = %s\n",
          ipv6_addr_to_str(addr_str, &rrep->orig_prefix, sizeof(addr_str)));
    DEBUG("  TargPrefix = %s/%d\n",
          ipv6_addr_to_str(addr_str, &rrep->targ_prefix, sizeof(addr_str)),
          rrep->targ_pfx_len);

    /* Add OrigPrefix address */
    pfx_len = 128;
    ipv6_addr_to_netaddr(&rrep->orig_prefix, pfx_len, &tmp);
    orig_prefix = rfc5444_writer_add_address(writer, _rrep_message_content_provider.creator, &tmp, true);
    if (orig_prefix == NULL) {
        DEBUG("  couldn't add OrigPrefix\n");
        return;
    }

    /* Add TargPrefix address */
    pfx_len = _normalize_pfx_len(rrep->targ_pfx_len);
    ipv6_addr_to_netaddr(&rrep->targ_prefix, pfx_len, &tmp);
    targ_prefix = rfc5444_writer_add_address(writer, _rrep_message_content_provider.creator, &tmp, true);
    if (targ_prefix == NULL) {
        DEBUG("  couldn't add TargPrefix\n");
        return;
    }

    /* Add SeqNoRtr address */
    if (!ipv6_addr_is_unspecified(&rrep->seqnortr)) {
        DEBUG("  SeqNoRtr = %s",
              ipv6_addr_to_str(addr_str, &rrep->seqnortr, sizeof(addr_str)));
        pfx_len = 128;
        ipv6_addr_to_netaddr(&rrep->seqnortr, pfx_len, &tmp);
        seqnortr = rfc5444_writer_add_address(writer, _rrep_message_content_provider.creator, &tmp, true);
        if (seqnortr == NULL) {
            DEBUG("  couldn't add SeqNoRtr\n");
            return;
        }
    }

    /* Add ADDRESS_TYPE TLV to OrigPrefix */
    address_type = AODVV2_ADDRTYPE_ORIGPREFIX;
    rfc5444_writer_add_addrtlv(writer, orig_prefix, &_rreq_addrtlvs[IDX_ADDRTLV_ADDRESS_TYPE], &address_type,
                               sizeof(uint8_t), false);

    /* Add ADDRESS_TYPE, SEQ_NUM and PATH_METRIC TLVs to TargPrefix */
    address_type = AODVV2_ADDRTYPE_TARGPREFIX;
    rfc5444_writer_add_addrtlv(writer, targ_prefix, &_rreq_addrtlvs[IDX_ADDRTLV_ADDRESS_TYPE], &address_type,
                               sizeof(uint8_t), false);

    rfc5444_writer_add_addrtlv(writer, targ_prefix, &_rreq_addrtlvs[IDX_ADDRTLV_SEQ_NUM], &rrep->targ_seqnum,
                               sizeof(aodvv2_seqnum_t), false);

    rfc5444_writer_add_addrtlv(writer, targ_prefix, &_rreq_addrtlvs[IDX_ADDRTLV_PATH_METRIC], &rrep->targ_metric,
                               sizeof(aodvv2_seqnum_t), false);
}

static void _rrep_ack_add_msgtlvs(struct rfc5444_writer *writer)
{
    DEBUG("aodvv2: adding RREP_Ack Message TLVs\n");

    /* AckReq is optional */
    if (_msg.rrep_ack.ackreq != 0) {
        DEBUG("  AckReq = %d\n", _msg.rrep_ack.ackreq);
        rfc5444_writer_add_messagetlv(writer, AODVV2_MSGTLV_ACKREQ, 0,
                                      &_msg.rrep_ack.ackreq, sizeof(uint8_t));
    }

    /* TIMESTAMP TLV */
    if (_msg.rrep_ack.timestamp != 0) {
        DEBUG("  TIMESTAMP = %d\n", _msg.rrep_ack.timestamp);
        rfc5444_writer_add_messagetlv(writer, RFC7182_MSGTLV_TIMESTAMP, 0,
                                      &_msg.rrep_ack.timestamp, sizeof(aodvv2_seqnum_t));
    }

    /* TODO(jeandudey): add Integrity-Check-Value */
}

int _aodvv2_writer_init(void)
{
    int res;

    mutex_lock(&_lock);
    gnrc_rfc5444_writer_acquire();
    struct rfc5444_writer *writer = gnrc_rfc5444_writer();

    _rreq_msg = rfc5444_writer_register_message(writer, AODVV2_MSGTYPE_RREQ, false);
    if (_rreq_msg == NULL) {
        DEBUG("rfc5444_writer: couldn't register RREQ message\n");
        mutex_unlock(&_lock);
        gnrc_rfc5444_writer_release();
        return -ENOMEM;
    }

    _rrep_msg = rfc5444_writer_register_message(writer, AODVV2_MSGTYPE_RREP, false);
    if (_rrep_msg == NULL) {
        rfc5444_writer_unregister_message(writer, _rreq_msg);
        DEBUG("rfc5444_writer: couldn't register RREP message\n");
        mutex_unlock(&_lock);
        gnrc_rfc5444_writer_release();
        return -ENOMEM;
    }

    _rrep_ack_msg = rfc5444_writer_register_message(writer, AODVV2_MSGTYPE_RREP_ACK, false);
    if (_rrep_ack_msg == NULL) {
        rfc5444_writer_unregister_message(writer, _rreq_msg);
        rfc5444_writer_unregister_message(writer, _rrep_msg);
        DEBUG("rfc5444_writer: couldn't register RREP_Ack message\n");
        mutex_unlock(&_lock);
        gnrc_rfc5444_writer_release();
        return -ENOMEM;
    }

    res = rfc5444_writer_register_msgcontentprovider(writer, &_rreq_message_content_provider, _rreq_addrtlvs,
                                                     ARRAY_SIZE(_rreq_addrtlvs));
    if (res < 0) {
        rfc5444_writer_unregister_message(writer, _rreq_msg);
        rfc5444_writer_unregister_message(writer, _rreq_msg);
        rfc5444_writer_unregister_message(writer, _rrep_ack_msg);
        DEBUG("rfc5444_writer: couldn't register RREQ message provider\n");
        mutex_unlock(&_lock);
        gnrc_rfc5444_writer_release();
        return -ENOMEM;
    }

    res = rfc5444_writer_register_msgcontentprovider(writer, &_rrep_message_content_provider, _rrep_addrtlvs,
                                                     ARRAY_SIZE(_rrep_addrtlvs));
    if (res < 0) {
        rfc5444_writer_unregister_message(writer, _rreq_msg);
        rfc5444_writer_unregister_message(writer, _rreq_msg);
        rfc5444_writer_unregister_message(writer, _rrep_ack_msg);
        DEBUG("rfc5444_writer: couldn't register RREQ message provider\n");
        mutex_unlock(&_lock);
        gnrc_rfc5444_writer_release();
        return -ENOMEM;
    }

    res = rfc5444_writer_register_msgcontentprovider(writer, &_rrep_ack_message_content_provider, NULL, 0);
    if (res < 0) {
        rfc5444_writer_unregister_message(writer, _rreq_msg);
        rfc5444_writer_unregister_message(writer, _rreq_msg);
        rfc5444_writer_unregister_message(writer, _rrep_ack_msg);
        DEBUG("rfc5444_writer: couldn't register RREP_Ack message provider\n");
        mutex_unlock(&_lock);
        gnrc_rfc5444_writer_release();
        return -ENOMEM;
    }

    _rreq_msg->addMessageHeader = _add_message_header;
    _rrep_msg->addMessageHeader = _add_message_header;

    mutex_unlock(&_lock);
    gnrc_rfc5444_writer_release();

    return 0;
}

int _aodvv2_writer_send_rreq(aodvv2_msg_rreq_t *rreq)
{
    DEBUG("aodvv2: sending RREQ message %p\n", (void *)rreq);

    mutex_lock(&_lock);
    memcpy(&_msg.rreq, rreq, sizeof(aodvv2_msg_rreq_t));
    _msg.type = AODVV2_MSGTYPE_RREQ;

    gnrc_rfc5444_writer_acquire();
    struct rfc5444_writer *writer = gnrc_rfc5444_writer();

    /* TODO(jeandudey): change alltarget to a custom target that sends only to
     * LL-MANET-Router destinations */
    if (rfc5444_writer_create_message_alltarget(writer, AODVV2_MSGTYPE_RREQ,
                                                RFC5444_MAX_ADDRLEN) != RFC5444_OKAY) {
        DEBUG("  failed to create message\n");
        mutex_unlock(&_lock);
        gnrc_rfc5444_writer_release();
        return -EIO;
    }

    mutex_unlock(&_lock);
    gnrc_rfc5444_writer_release();
    return 0;
}

int _aodvv2_writer_send_rrep(aodvv2_msg_rrep_t *rrep, const ipv6_addr_t *dst,
                             uint16_t iface)
{
    DEBUG("aodvv2: sending RREP message %p (dst = %s, iface = %d)\n",
          (void *)rrep,
          (dst == NULL) ? "NULL" : ipv6_addr_to_str(addr_str, dst,
                                                    sizeof(addr_str)), iface);

    mutex_lock(&_lock);
    gnrc_rfc5444_writer_acquire();
    struct rfc5444_writer *writer = gnrc_rfc5444_writer();
    struct rfc5444_writer_target *target = gnrc_rfc5444_get_writer_target(dst, iface);

    if (target == NULL) {
        DEBUG("  target not found\n");
        mutex_unlock(&_lock);
        gnrc_rfc5444_writer_release();
        return -ENOENT;
    }

    memcpy(&_msg.rrep, rrep, sizeof(aodvv2_msg_rrep_t));
    _msg.type = AODVV2_MSGTYPE_RREP;


    if (rfc5444_writer_create_message_singletarget(writer, AODVV2_MSGTYPE_RREP,
                                                   RFC5444_MAX_ADDRLEN,
                                                   target) != RFC5444_OKAY) {
        DEBUG("  failed to create message\n");
        mutex_unlock(&_lock);
        gnrc_rfc5444_writer_release();
        return -EIO;
    }

    mutex_unlock(&_lock);
    gnrc_rfc5444_writer_release();
    return 0;
}

int _aodvv2_writer_send_rrep_ack(aodvv2_msg_rrep_ack_t *rrep_ack,
                                 const ipv6_addr_t *dst, uint16_t iface)
{
    DEBUG("aodvv2: sending RREP_Ack message %p (dst = %s, iface = %d)\n",
          (void *)rrep_ack,
          (dst == NULL) ? "NULL" : ipv6_addr_to_str(addr_str, dst,
                                                    sizeof(addr_str)), iface);

    mutex_lock(&_lock);
    gnrc_rfc5444_writer_acquire();
    struct rfc5444_writer *writer = gnrc_rfc5444_writer();
    struct rfc5444_writer_target *target = gnrc_rfc5444_get_writer_target(dst, iface);

    if (target == NULL) {
        DEBUG("  target not found\n");
        return -ENOENT;
    }

    memcpy(&_msg.rrep_ack, rrep_ack, sizeof(aodvv2_msg_rrep_ack_t));
    _msg.type = AODVV2_MSGTYPE_RREP_ACK;

    if (rfc5444_writer_create_message_singletarget(writer,
                                                   AODVV2_MSGTYPE_RREP_ACK,
                                                   RFC5444_MAX_ADDRLEN,
                                                   target) != RFC5444_OKAY) {
        DEBUG("  failed to create message\n");
        mutex_unlock(&_lock);
        gnrc_rfc5444_writer_release();
        return -ENOMEM;
    }

    mutex_unlock(&_lock);
    gnrc_rfc5444_writer_release();
    return 0;
}
