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
 * @brief       AODVv2 routing protocol constants
 *
 * @author      Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 * @author      Gustavo Grisales <gustavosinbandera1@hotmail.com>
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 */

#ifndef AODVV2_CONSTANTS_H
#define AODVV2_CONSTANTS_H

/**
 * @brief TODO: check if it's MAX_METRIC[HopCount] or MAX_HOPCOUNT.
 *
 * @see <a href="https://tools.ietf.org/id/draft-ietf-manet-aodvv2-16.txt">
 *          draft-ietf-manet-aodvv2-16, 11.2 Protocol Constants.
 *      </a>
 */
#define AODVV2_MAX_HOPCOUNT (255)

/**
 * @brief   Maximum number of entries in the routing table.
 */
#define AODVV2_MAX_ROUTING_ENTRIES (8)

/**
 * @brief   Active interval value in seconds
 *
 * @see draft-ietf-manet-aodvv2-16, 11.1 Timers.
 */
#define AODVV2_ACTIVE_INTERVAL (5U)

/**
 * @brief TODO: investigate what's this
 *
 * @see <a href="https://tools.ietf.org/id/draft-ietf-manet-aodvv2-16.txt">
 *          draft-ietf-manet-aodvv2-16, 11.1 Timers.
 *      </a>
 */
#define AODVV2_MAX_IDLETIME (250U)

/**
 * @brief    Lifetime of a sequence number in seconds.
 *
 * @see <a href="https://tools.ietf.org/id/draft-ietf-manet-aodvv2-16.txt">
 *          draft-ietf-manet-aodvv2-16, 11.1 Timers.
 *      </a>
 */
#define AODVV2_MAX_SEQNUM_LIFETIME (300U)

/**
 * @brief TODO: choose value (wisely).
 *        TODO: investigate what's this.
 */
#define AODVV2_MAX_UNREACHABLE_NODES (15U)

/**
 * @brief   TLV type array indices
 */
typedef enum tlv_index {
    TLV_ORIGSEQNUM, /**< Originator Sequence Number */
    TLV_TARGSEQNUM, /**< Target Sequence Number */
    TLV_UNREACHABLE_NODE_SEQNUM, /**< Unreachable Node Sequence Number */
    TLV_METRIC, /** Metric */
} tlv_index_t;

/**
 * @brief   Static initializer for the link-local all MANET-Routers IPv6
 *          address (ff02:0:0:0:0:0:0:6d)
 *
 * @see <a href="https://tools.ietf.org/html/rfc5498#section-6">
 *          RFC 5498, section 6
 *      </a>
 */
#define IPV6_ADDR_ALL_MANET_ROUTERS_LINK_LOCAL {{ 0xff, 0x02, 0x00, 0x00, \
                                                  0x00, 0x00, 0x00, 0x00, \
                                                  0x00, 0x00, 0x00, 0x00, \
                                                  0x00, 0x00, 0x00, 0x6d }}

/**
 * @brief   UDP Port for MANET Protocols 1 (udp/269).
 *
 * @see <a href="https://tools.ietf.org/html/rfc5498#section-6">
 *          RFC 5498, section 6
 *      </a>
 */
#define UDP_MANET_PROTOCOLS_1 (269)

/**
 * @see @ref IPV6_ADDR_ALL_MANET_ROUTERS_LINK_LOCAL
 */
extern ipv6_addr_t ipv6_addr_all_manet_routers_link_local;

#endif /* AODVV2_CONSTANTS_H */
