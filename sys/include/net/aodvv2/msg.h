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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @{
 * @name RFC 5444 Message Type Allocation
 */
#define AODVV2_MSGTYPE_RREQ     (10) /**< Route Request (RREQ) (TBD) */
#define AODVV2_MSGTYPE_RREP     (11) /**< Route Reply (RREP) (TBD) */
#define AODVV2_MSGTYPE_RERR     (12) /**< Route Error (RERR) (TBD) */
#define AODVV2_MSGTYPE_RREP_ACK (13) /**< Route Reply Acknowledgement (RREP_Ack) (TBD) */
/** @} */

/**
 * @{
 * @name RFC 5444 Message TLV Types
 * @see [draft-perkins-manet-aodvv2-03, section 13.2]
 *      (https://tools.ietf.org/html/draft-perkins-manet-aodvv2-03#section-13.2)
 */
#define AODVV2_MSGTLV_ACKREQ (128) /**< AckReq (TBD) */
/** @} */

/**
 * @{
 * @name RFC 5444 Address Block TLV Type Allocation
 * @see [draft-perkins-manet-aodvv2-03, section 13.3]
 *      (https://tools.ietf.org/html/draft-perkins-manet-aodvv2-03#section-13.3)
 */
#define AODVV2_ADDRTLV_PATH_METRIC  (129) /**< PATH_METRIC (TBD) */
#define AODVV2_ADDRTLV_SEQ_NUM      (130) /**< SEQ_NUM (TBD */
#define AODVV2_ADDRTLV_ADDRESS_TYPE (131) /**< ADDRESS_TYPE (TBD) */
/** @} */

/**
 * @{
 * @name MetricType Allocation
 * @see [draft-perkins-manet-aodvv2-03, section 13.4]
 *      (https://tools.ietf.org/html/draft-perkins-manet-aodvv2-03#section-13.4)
 */
#define AODVV2_METRIC_TYPE_UNASSIGNED   (0) /**< Unassigned */
#define AODVV2_METRIC_TYPE_HOP_COUNT    (1) /**< Hop Count */
#define AODVV2_METRIC_TYPE_RESERVED   (255) /**< Reserved */
/** @} */

/**
 * @{
 * @name ADDRESS_TYPE TLV Values
 * @see [draft-perkins-manet-aodvv2-03, section 13.5]
 *      (https://tools.ietf.org/html/draft-perkins-manet-aodvv2-03#section-13.5)
 */
#define AODVV2_ADDRTYPE_ORIGPREFIX  (0) /**< OrigPrefix */
#define AODVV2_ADDRTYPE_TARGPREFIX  (1) /**< TargPrefix */
#define AODVV2_ADDRTYPE_UNREACHABLE (2) /**< Unreachable address */
#define AODVV2_ADDRTYPE_PKTSOURCE   (3) /**< Packet source */
#define AODVV2_ADDRTYPE_UNSPECIFIED (255) /**< Unspecified */
/** @} */

/**
 * @{
 * @name Codes for destination unreachable messages
 *
 * @anchor net_icmpv6_error_dst_unr_codes
 * @see [draft-perkins-manet-aodvv2-03, section 13.6]
 *      (https://tools.ietf.org/html/draft-perkins-manet-aodvv2-03#section-13.6)
 */
#define ICMPV6_ERROR_DST_UNR_METRIC_TYPE_MISMATCH (8) /**< Metric Type Mismatch */
/** @} */

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
 * @brief   Route Request (RREQ) Message
 */
typedef struct {
    uint8_t msg_hop_limit;       /**< Message hop limit */
    ipv6_addr_t orig_prefix;     /**< OrigPrefix */
    ipv6_addr_t targ_prefix;     /**< TargPrefix */
    ipv6_addr_t seqnortr;        /**< SeqNoRtr */
    uint8_t orig_pfx_len;        /**< OrigPfxLen */
    aodvv2_seqnum_t orig_seqnum; /**< OrigSeqNum */
    aodvv2_seqnum_t targ_seqnum; /**< TargSeqNum */
    uint8_t metric_type;         /**< MetricType */
    uint8_t orig_metric;         /**< OrigMetric */
} aodvv2_msg_rreq_t;

/**
 * @brief   Route Reply (RREP) Message
 */
typedef struct {
    uint8_t msg_hop_limit;       /**< Message hop limit */
    ipv6_addr_t orig_prefix;     /**< OrigPrefix */
    ipv6_addr_t targ_prefix;     /**< TargPrefix */
    ipv6_addr_t seqnortr;        /**< SeqNoRtr */
    uint8_t targ_pfx_len;        /**< TargPfxLen */
    aodvv2_seqnum_t targ_seqnum; /**< TargSeqNum */
    uint8_t metric_type;         /**< MetricType */
    uint8_t targ_metric;         /**< TargMetric */
} aodvv2_msg_rrep_t;

/**
 * @brief   Route Reply Acknowledgement (RREP_Ack) Message
 *
 * @see [draft-perkins-manet-aodvv2-03, section 7.3]
 *      (https://tools.ietf.org/html/draft-perkins-manet-aodvv2-03#section-7.3)
 */
typedef struct {
    uint8_t ackreq;              /**< AckReq */
    aodvv2_seqnum_t timestamp;   /**< TIMESTAMP TLV */
} aodvv2_msg_rrep_ack_t;

/**
 * @brief   AODVv2 Message Type
 */
typedef struct {
    uint8_t type;                 /**< Message type */
    union {
        aodvv2_msg_rreq_t rreq;   /**< Route Request */
        aodvv2_msg_rrep_t rrep;   /**< Route Reply */
        aodvv2_msg_rrep_ack_t rrep_ack; /**< Route Reply Acknowledgement */
    };
} aodvv2_message_t;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AODVV2_MSG_H */
/** @} */
