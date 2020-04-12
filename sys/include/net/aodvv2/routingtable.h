/*
 * Copyright (C) 2014 Freie Universität Berlin
 * Copyright (C) 2014 Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     aodvv2
 * @{
 *
 * @file        routingtable.h
 * @brief       Cobbled-together routing table.
 *
 * @author      Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 */

#ifndef AODVV2_ROUTINGTABLE_H
#define AODVV2_ROUTINGTABLE_H

#include <string.h>

#include "common/netaddr.h"

#include "net/aodvv2/aodvv2.h"
#include "net/aodvv2/rfc5444.h"
#include "net/aodvv2/seqnum.h"
#include "net/metric.h"

#include "timex.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Maximum number of routing entries
 * @{
 */
#ifndef CONFIG_AODVV2_MAX_ROUTING_ENTRIES
#define CONFIG_AODVV2_MAX_ROUTING_ENTRIES (16)
#endif
/** @} */

/**
 * A route table entry (i.e., a route) may be in one of the following states:
 */
enum aodvv2_routing_state {
    ROUTE_STATE_ACTIVE,
    ROUTE_STATE_IDLE,
    ROUTE_STATE_EXPIRED,
    ROUTE_STATE_BROKEN,
    ROUTE_STATE_TIMED
};

/**
 * @brief   All fields of a routing table entry
 */
typedef struct {
    struct netaddr addr; /**< IP address of this route's destination */
    aodvv2_seqnum_t seqnum; /**< The Sequence Number obtained from the
                                 last packet that updated the entry */
    struct netaddr next_hop; /**< IP address of the the next hop towards the
                                     destination */
    timex_t last_used; /**< IP address of this route's destination */
    timex_t expiration_time; /**< Time at which this route expires */
    routing_metric_t metricType; /**< Metric type of this route */
    uint8_t metric; /**< Metric value of this route*/
    uint8_t state; /**< State of this route (i.e. one of
                        aodvv2_routing_states) */
} aodvv2_routing_entry_t;

/**
 * @brief     Initialize routing table.
 */
void aodvv2_routingtable_init(void);

/**
 * @brief     Get next hop towards dest.
 *            Returns NULL if dest is not in routing table.
 *
 * @param[in] dest        Destination of the packet
 * @param[in] metricType  Metric Type of the desired route
 * @return                next hop towards dest if it exists, NULL otherwise
 */
struct netaddr *aodvv2_routingtable_get_next_hop(struct netaddr *dest,
                                                 routing_metric_t metricType);

/**
 * @brief     Add new entry to routing table, if there is no other entry
 *            to the same destination.
 *
 * @param[in] entry        The routing table entry to add
 */
void aodvv2_routingtable_add_entry(aodvv2_routing_entry_t *entry);

/**
 * @brief     Retrieve pointer to a routing table entry.
 *            To edit, simply follow the pointer.
 *            Returns NULL if addr is not in routing table.
 *
 * @param[in] addr          The address towards which the route should point
 * @param[in] metricType    Metric Type of the desired route
 * @return                  Routing table entry if it exists, NULL otherwise
 */
aodvv2_routing_entry_t *aodvv2_routingtable_get_entry(struct netaddr *addr,
                                                      routing_metric_t metricType);

/**
 * @brief     Delete routing table entry towards addr with metric type MetricType,
 *            if it exists.
 *
 * @param[in] addr          The address towards which the route should point
 * @param[in] metricType    Metric Type of the desired route
 */
void aodvv2_routingtable_delete_entry(struct netaddr *addr, routing_metric_t metricType);

/**
 * Check if the data of a RREQ or RREP offers improvement for an existing routing
 * table entry.
 * @param rt_entry            the routing table entry to check
 * @param node_data           The data to check against. When handling a RREQ,
 *                            the OrigNode's information (i.e. packet_data.origNode)
 *                            must be passed. When handling a RREP, the
 *                            TargNode's data (i.e. packet_data.targNode) must
 *                            be passed.
 */
bool aodvv2_routingtable_offers_improvement(aodvv2_routing_entry_t *rt_entry,
                                            node_data_t *node_data);

/**
 * Fills a routing table entry with the data of a RREQ.
 * @param packet_data         the RREQ's data
 * @param rt_entry            the routing table entry to fill
 * @param link_cost           the link cost for this RREQ
 */
void aodvv2_routingtable_fill_routing_entry_rreq(aodvv2_packet_data_t *packet_data,
                                                 aodvv2_routing_entry_t *rt_entry,
                                                 uint8_t link_cost);

/**
 * Fills a routing table entry with the data of a RREP.
 * @param packet_data         the RREP's data
 * @param rt_entry            the routing table entry to fill
 * @param link_cost           the link cost for this RREP
 */
void aodvv2_routingtable_fill_routing_entry_rrep(aodvv2_packet_data_t *packet_data,
                                                 aodvv2_routing_entry_t *rt_entry,
                                                 uint8_t link_cost);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AODVV2_ROUTINGTABLE_H  */
