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
 * @brief       AODVv2 Protocol Messages
 *
 * @author      Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 * @author      Gustavo Grisales <gustavosinbandera1@hotmail.com>
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 */

#ifndef NET_AODVV2_MSG_H
#define NET_AODVV2_MSG_H

#include <stdint.h>

#include "net/ipv6.h"
#include "net/metric.h"
#include "net/aodvv2/seqnum.h"
#include "timex.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   AODVv2 message types
 */
typedef enum {
    RFC5444_MSGTYPE_RREQ     = 10, /**< RREQ message type */
    RFC5444_MSGTYPE_RREP     = 11, /**< RREP message type */
    RFC5444_MSGTYPE_RERR     = 12, /**< RERR message type */
    RFC5444_MSGTYPE_RREP_ACK = 13, /**< RREP_Ack message type */
} rfc5444_msg_type_t;

/**
 * @brief   AODVv2 TLV types
 */
typedef enum {
    RFC5444_MSGTLV_ORIGSEQNUM,
    RFC5444_MSGTLV_TARGSEQNUM,
    RFC5444_MSGTLV_UNREACHABLE_NODE_SEQNUM,
    RFC5444_MSGTLV_METRIC,
} rfc5444_tlv_type_t;

/**
 * @brief   Data about an OrigNode or TargNode.
 */
typedef struct {
    ipv6_addr_t addr;             /**< IPv6 address of the node */
    uint8_t pfx_len;              /**< IPv6 address length */
    uint8_t metric;               /**< Metric value */
    aodvv2_seqnum_t seqnum;       /**< Sequence Number */
} node_data_t;

/**
 * @brief   All data contained in a RREQ or RREP.
 */
typedef struct {
    uint8_t msg_hop_limit;        /**< Hop limit */
    ipv6_addr_t sender;           /**< IP address of the neighboring router */
    routing_metric_t metric_type; /**< Metric type */
    node_data_t orig_node;        /**< OrigNode data */
    node_data_t targ_node;        /**< TargNode data */
    ipv6_addr_t seqnortr;         /**< SeqNoRtr */
    timex_t timestamp;            /**< Time at which the message was received */
} aodvv2_message_t;


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AODVV2_MSG_H */
/** @} */
