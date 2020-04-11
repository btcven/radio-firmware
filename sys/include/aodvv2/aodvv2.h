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
 */

#ifndef AODVV2_AODVV2_H
#define AODVV2_AODVV2_H

#include "net/gnrc/netif.h"
#include "timex.h"

#include "common/netaddr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   AODVv2 metric types. Extend to include alternate metrics.
 */
typedef enum {
    AODVV2_METRIC_HOP_COUNT = 3, /**< see RFC 6551*/
} aodvv2_metric_t;

typedef uint16_t aodvv2_seqnum_t;

#define AODVV2_DEFAULT_METRIC_TYPE AODVV2_METRIC_HOP_COUNT

/**
 * @brief   AODVv2 message types
 */
typedef enum {
    RFC5444_MSGTYPE_RREQ = 10, /**< RREQ message type */
    RFC5444_MSGTYPE_RREP = 11, /**< RREP message type */
    RFC5444_MSGTYPE_RERR = 12, /**< RERR message type */
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
 * @brief   Data about an unreachable node to be embedded in a RERR.
 */
typedef struct unreachable_node {
    struct netaddr addr; /**< IP address */
    aodvv2_seqnum_t seqnum; /**< Sequence Number */
} unreachable_node_t;

/**
 * @brief   Data about an OrigNode or TargNode, typically embedded in an
 *          aodvv2_packet_data_t struct.
 */
typedef struct {
    struct netaddr addr; /**< IP address of the node */
    uint8_t metric; /**< Metric value */
    aodvv2_seqnum_t seqnum; /**< Sequence Number */
} node_data_t;

/**
 * @brief   All data contained in a RREQ or RREP.
 */
typedef struct {
    uint8_t hoplimit; /**< Hop limit */
    struct netaddr sender; /**< IP address of the neighboring router
                                which sent the RREQ/RREP*/
    aodvv2_metric_t metric_type; /**< Metric type */
    node_data_t orig_node; /**< Data about the originating node */
    node_data_t targ_node; /**< Data about the originating node */
    timex_t timestamp; /**< Point at which the packet was (roughly)
                            received. Note that this timestamp
                            will be set after the packet has been
                            successfully parsed. */
} aodvv2_packet_data_t;

/**
 * @brief   This struct contains data which needs to be put into a RREQ or RREP.
 *          It is used to transport this data in a message to the sender_thread.
 *
 * @note    Please note that it is for internal use only. To send a RREQ or
 *          RREP, please use the aodv_send_rreq() and aodv_send_rrep()
 *          functions.
 */
typedef struct {
    aodvv2_packet_data_t *packet_data; /**< Data for the RREQ or RREP */
    struct netaddr *next_hop; /**< Next hop to which the RREQ or RREP should be
                                   sent */
} rreq_rrep_data_t;

/**
 * @brief   This struct contains data which needs to be put into a RERR.
 *          It is used to transport this data in a message to the sender_thread.
 *
 * @note    Please note that it is for internal use only. To send a RERR,
 *          please use the aodv_send_rerr() function.
 */
typedef struct {
    unreachable_node_t *unreachable_nodes; /**< All unreachable nodes. Beware,
                                                this is the start of an array */
    size_t len; /**< Length of the unreachable_nodes array */
    int hoplimit; /**< hoplimit for the RERR */
    struct netaddr *next_hop; /**< Next hop to which the RERR should be sent */
} rerr_data_t;

/**
 * @brief   This struct holds the data for a RREQ, RREP or RERR (contained
 *          in a rreq_rrep_data or rerr_data struct) and the next hop the RREQ,
 *          RREP or RERR should be sent to. It used for message communication
 *          with the sender_thread.
 *
 * @note    Please note that it is for internal use only. To send a RERR,
 *          please use the aodv_send_rerr() function.
 */
typedef struct {
    rfc5444_msg_type_t type; /**< Message type (i.e. one of rfc5444_msg_type) */
    void *data; /**< Pointer to the message data (i.e. rreq_rrep_data or
                     rerr_data) */
} msg_container_t;

/**
 * @brief   Initialize the AODVv2 routing protocol.
 *
 * @param[in] netif The network interface to run the AODVv2 protocol on.
 */
void aodvv2_init(gnrc_netif_t *netif);

/**
 * @brief   Request a route to target_addr, if target_addr is already know, no
 *          RREQ is made.
 *
 * @param[in] target_addr The IPv6 address of the node we want to find a route.
 */
void aodvv2_find_route(ipv6_addr_t *target_addr);

/**
 * @brief   This function creates a wraper for the aodv package to send it to
 *          the thread responsible for handling the packet and use RFC 5444 API
 *
 * @param[in] packet_data The aodv packet with indformation realte with the protocol
 */
void aodvv2_send_rreq(aodvv2_packet_data_t *packet_data);

/**
 * @brief   Send a RREP
 *
 * @pre (packet_data != NULL) && (next_hop != NULL)
 *
 * @param[in] packet_data The AODVv2 packet
 * @param[in] next_hop    Where to send this RREP
 */
void aodvv2_send_rrep(aodvv2_packet_data_t *packet_data,
                      struct netaddr *next_hop);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AODVV2_AODVV2_H */
/** @} */
