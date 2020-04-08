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

#ifndef AODVV2_UTILS_H
#define AODVV2_UTILS_H

#include "common/netaddr.h"
#include "net/ipv6.h"

#include "aodvv2/aodvv2.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Multiple clients are currently not supported.
 */
#define AODVV2_MAX_CLIENTS (1)

/**
 * @brief   should be enough for now...
 */
#define AODVV2_RREQ_BUF (128)

/**
 * @brief   RREQ wait time in seconds
 */
#define AODVV2_RREQ_WAIT_TIME (2)

/**
 * @brief   Prefix length of the IPv6 addresses used in the network served by
 *          AODVv2
 */
#define AODVV2_RIOT_PREFIXLEN (128)

/**
 * @brief   RREQ Table entry which stores all information about a RREQ that was
 *          received in order to avoid duplicates.
 */
typedef struct {
    struct netaddr origNode; /**< Node which originated the RREQ*/
    struct netaddr targNode; /**< Target (destination) of the RREQ */
    aodvv2_metric_t metricType; /**< Metric type of the RREQ */
    uint8_t metric; /**< Metric of the RREQ */
    aodvv2_seqnum_t seqnum; /**< Sequence number of the RREQ */
    timex_t timestamp; /**< Last time this entry was updated */
} aodvv2_rreq_entry_t;

/**
 * @brief   Convert an IP stored as an ipv6_addr_t to a netaddr
 *
 * @param[in]  src ipv6_addr_t to convert
 * @param[out] dst netaddr to convert into
 */
void ipv6_addr_to_netaddr(ipv6_addr_t *src, struct netaddr *dst);

/**
 * @brief   Convert an IP stored as a netaddr to an ipv6_addr_t
 *
 * @param[in]  src netaddr to convert into
 * @param[out] dst ipv6_addr_t to convert
 */
void netaddr_to_ipv6_addr(struct netaddr *src, ipv6_addr_t *dst);

/**
 * @brief   Initialize table of clients that the router currently serves.
 */
void aodvv2_clienttable_init(void);

/**
 * @brief   Add client to the list of clients that the router currently serves.
 *
 * @param[in] addr Address of the client (Since the current version doesn't
 *                 offer support for Client Networks, the prefixlen is
 *                 currently ignored).
 */
void aodvv2_clienttable_add_client(struct netaddr *addr);

/**
 * @brief   Find out if a client is in the list of clients that the router
 *          currently serves.
 *
 * @param[in] addr Address of the client in question (Since the current version
 *                 doesn't offer support for Client Networks, the prefixlen is
 *                 currently ignored.)
 */
bool aodvv2_clienttable_is_client(struct netaddr *addr);

/**
 * @brief   Delete a client from the list of clients that the router currently
 *          serves.
 *
 * @param[in] addr Address of the client to delete (Since the current version
 *                 doesn't offer support for Client Networks, the prefixlen is
 *                 currently ignored).
 */
void aodvv2_clienttable_delete_client(struct netaddr *addr);

/**
 * @brief   Initialize RREQ table.
 */
void aodvv2_rreqtable_init(void);

/**
 * @brief   Check if a RREQ is redundant, i.e. was received from another node
 *          already.
 *
 * @see <a href="https://tools.ietf.org/id/draft-ietf-manet-aodvv2-16.txt">
 *          draft-ietf-manet-aodvv2-16, Section 5.6
 *      </a>
 *
 * @param[in] packet_data Data of the RREQ in question
 */
bool aodvv2_rreqtable_is_redundant(aodvv2_packet_data_t *packet_data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AODVV2_UTILS_H */
