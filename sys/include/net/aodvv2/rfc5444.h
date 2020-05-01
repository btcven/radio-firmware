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
 * @brief       RFC5444
 *
 * @author      Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 * @author      Gustavo Grisales <gustavosinbandera1@hotmail.com>
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 */

#ifndef NET_AODVV2_RFC5444_H
#define NET_AODVV2_RFC5444_H

#include "net/aodvv2/seqnum.h"
#include "net/manet/manet.h"
#include "net/metric.h"

#include "timex.h"

#include "common/netaddr.h"
#include "rfc5444/rfc5444_reader.h"
#include "rfc5444/rfc5444_writer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    RFC5444 thread stack size
 * @{
 */
#ifndef CONFIG_AODVV2_RFC5444_STACK_SIZE
#define CONFIG_AODVV2_RFC5444_STACK_SIZE (2048)
#endif
/** @} */

/**
 * @name    RFC5444 thread priority
 * @{
 */
#ifndef CONFIG_AODVV2_RFC5444_PRIO
#define CONFIG_AODVV2_RFC5444_PRIO (6)
#endif
/** @} */

/**
 * @name    RFC5444 message queue size
 * @{
 */
#ifndef CONFIG_AODVV2_RFC5444_MSG_QUEUE_SIZE
#define CONFIG_AODVV2_RFC5444_MSG_QUEUE_SIZE (32)
#endif
/** @} */

/**
 * @name    RFC5444 maximum packet size
 * @{
 */
#ifndef CONFIG_AODVV2_RFC5444_PACKET_SIZE
#define CONFIG_AODVV2_RFC5444_PACKET_SIZE (128)
#endif
/** @} */

/**
 * @name    RFC5444 address TLVs buffer size
 * @{
 */
#ifndef CONFIG_AODVV2_RFC5444_ADDR_TLVS_SIZE
#define CONFIG_AODVV2_RFC5444_ADDR_TLVS_SIZE (1000)
#endif
/** @} */

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
 * @brief   Data about an OrigNode or TargNode.
 */
typedef struct {
    ipv6_addr_t addr; /**< IP address of the node */
    uint8_t metric; /**< Metric value */
    aodvv2_seqnum_t seqnum; /**< Sequence Number */
} node_data_t;

/**
 * @brief   All data contained in a RREQ or RREP.
 */
typedef struct {
    uint8_t hoplimit; /**< Hop limit */
    ipv6_addr_t sender; /**< IP address of the neighboring router which sent the
                             RREQ/RREP*/
    routing_metric_t metric_type; /**< Metric type */
    node_data_t orig_node; /**< Data about the originating node */
    node_data_t targ_node; /**< Data about the originating node */
    timex_t timestamp; /**< Point at which the packet was (roughly)
                            received. Note that this timestamp
                            will be set after the packet has been
                            successfully parsed. */
} aodvv2_packet_data_t;

typedef struct {
    /** Interface for generating RFC5444 packets */
    struct rfc5444_writer_target target;
    /** Address to which the packet should be sent */
    ipv6_addr_t target_addr;
    aodvv2_packet_data_t packet_data; /**< Payload of the AODVv2 Message */
    int type; /**< Type of the AODVv2 Message (i.e. rfc5444_msg_type) */
} aodvv2_writer_target_t;

/**
 * @brief   Register AODVv2 message reader
 *
 * @param[in] reader Pointer to the reader context.
 */
void aodvv2_rfc5444_reader_register(struct rfc5444_reader *reader,
                                    kernel_pid_t netif_pid);

/**
 * @brief   Sets the sender address
 *
 * @notes MUST be called before starting to parse the packet.
 *
 * @param[in] sender The address of the sender.
 */
void aodvv2_rfc5444_handle_packet_prepare(ipv6_addr_t *sender);

/**
 * @brief   Register AODVv2 message writer
 *
 * @param[in] writer Pointer to the writer context.
 * @param[in] target Pointer to the writer target.
 */
void aodvv2_rfc5444_writer_register(struct rfc5444_writer *writer,
                                    aodvv2_writer_target_t *target);

/**
 * @brief   `ipv6_addr_t` to `struct netaddr`.
 *
 * @pre (@p src != NULL) && (@p dst != NULL)
 *
 * @param[in] src  Source.
 * @param[out] dst Destination.
 */
void ipv6_addr_to_netaddr(const ipv6_addr_t *src, struct netaddr *dst);

/**
 * @brief   `struct netaddr` to `ipv6_addr_t`.
 *
 * @pre (@p src != NULL) && (@p dst != NULL)
 *
 * @param[in] src  Source.
 * @param[out] dst Destination.
 */
void netaddr_to_ipv6_addr(struct netaddr *src, ipv6_addr_t *dst);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NET_AODVV2_RFC5444_H */
/** @} */
