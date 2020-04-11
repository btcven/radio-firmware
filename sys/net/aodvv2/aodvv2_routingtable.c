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
 * @brief       Cobbled-together routing table.
 *
 * @author      Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 */

#include "net/aodvv2/aodvv2.h"
#include "net/aodvv2/routingtable.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

static void _reset_entry_if_stale(uint8_t i);

static struct netaddr_str nbuf;

/**
 * @brief   Memory for the Routing Entries Set
 */
static aodvv2_routing_entry_t routing_table[CONFIG_AODVV2_MAX_ROUTING_ENTRIES];

static timex_t null_time, max_seqnum_lifetime, active_interval, max_idletime, validity_t;
timex_t now;

#if ENABLE_DEBUG == 1
static struct netaddr_str nbuf;
#endif

void aodvv2_routingtable_init(void)
{
    null_time = timex_set(0, 0);
    max_seqnum_lifetime = timex_set(CONFIG_AODVV2_MAX_SEQNUM_LIFETIME, 0);
    active_interval = timex_set(CONFIG_AODVV2_ACTIVE_INTERVAL, 0);
    max_idletime = timex_set(CONFIG_AODVV2_MAX_IDLETIME, 0);
    validity_t = timex_set(CONFIG_AODVV2_ACTIVE_INTERVAL +
                           CONFIG_AODVV2_MAX_IDLETIME, 0);

    memset(&routing_table, 0, sizeof(routing_table));
    DEBUG("routing table initialized.\n");
}

struct netaddr *aodvv2_routingtable_get_next_hop(struct netaddr *dest, routing_metric_t metricType)
{
    aodvv2_routing_entry_t *entry = aodvv2_routingtable_get_entry(dest, metricType);
    if (!entry) {
        return NULL;
    }
    return (&entry->nextHopAddr);
}

void aodvv2_routingtable_add_entry(aodvv2_routing_entry_t *entry)
{
    /* only add if we don't already know the address */
    if (aodvv2_routingtable_get_entry(&(entry->addr), entry->metricType)) {
        return;
    }
    /*find free spot in RT and place rt_entry there */
    for (unsigned i = 0; i < ARRAY_SIZE(routing_table); i++) {
        if (routing_table[i].addr._type == AF_UNSPEC) {
            memcpy(&routing_table[i], entry, sizeof(aodvv2_routing_entry_t));
            return;
        }
    }
}

aodvv2_routing_entry_t *aodvv2_routingtable_get_entry(struct netaddr *addr,
                                                      routing_metric_t metricType)
{
    for (unsigned i = 0; i < ARRAY_SIZE(routing_table); i++) {
        _reset_entry_if_stale(i);

        if (!netaddr_cmp(&routing_table[i].addr, addr)
            && routing_table[i].metricType == metricType) {
            DEBUG("[routing] found entry for %s :", netaddr_to_string(&nbuf, addr));
            return &routing_table[i];
        }
    }
    return NULL;
}

void aodvv2_routingtable_delete_entry(struct netaddr *addr, routing_metric_t metricType)
{
    for (unsigned i = 0; i < ARRAY_SIZE(routing_table); i++) {
        _reset_entry_if_stale(i);

        if (!netaddr_cmp(&routing_table[i].addr, addr)
                && routing_table[i].metricType == metricType) {
            memset(&routing_table[i], 0, sizeof(routing_table[i]));
            return;
        }
    }
}


/*
 * Check if entry at index i is stale as described in Section 6.3.
 * and clear the struct it fills if it is
 */
static void _reset_entry_if_stale(uint8_t i)
{
    xtimer_now_timex(&now);
    timex_t lastUsed, expirationTime;

    if (timex_cmp(routing_table[i].expirationTime, null_time) == 0) {
        return;
    }

    int state = routing_table[i].state;
    lastUsed = routing_table[i].lastUsed;
    expirationTime = routing_table[i].expirationTime;

    /* an Active route is considered to remain Active as long as it is used at least once
     * during every ACTIVE_INTERVAL. When a route is no longer Active, it becomes an Idle route. */

    /* if the node is younger than the active interval, don't bother */
    if (timex_cmp(now, active_interval) < 0) {
        return;
    }

    if ((state == ROUTE_STATE_ACTIVE) &&
        (timex_cmp(timex_sub(now, active_interval), lastUsed) == 1)) {
        DEBUG("\t[routing] route towards %s Idle\n",
              netaddr_to_string(&nbuf, &routing_table[i].addr));
        routing_table[i].state = ROUTE_STATE_IDLE;
        routing_table[i].lastUsed = now; /* mark the time entry was set to Idle */
    }

    /* After an idle route remains Idle for MAX_IDLETIME, it becomes an Expired route.
       A route MUST be considered Expired if Current_Time >= Route.ExpirationTime
    */

    /* if the node is younger than the expiration time, don't bother */
    if (timex_cmp(now, expirationTime) < 0) {
        return;
    }

    if ((state == ROUTE_STATE_IDLE) &&
        (timex_cmp(expirationTime, now) < 1)) {
        DEBUG("\t[routing] route towards %s Expired\n",
              netaddr_to_string(&nbuf, &routing_table[i].addr));
        DEBUG("\t expirationTime: %"PRIu32":%"PRIu32" , now: %"PRIu32":%"PRIu32"\n",
              expirationTime.seconds, expirationTime.microseconds,
              now.seconds, now.microseconds);
        routing_table[i].state = ROUTE_STATE_EXPIRED;
        routing_table[i].lastUsed = now; /* mark the time entry was set to Expired */
    }

    /* After that time, old sequence number information is considered no longer
     * valuable and the Expired route MUST BE expunged */
    if (timex_cmp(timex_sub(now, lastUsed), max_seqnum_lifetime) >= 0) {
        DEBUG("\t[routing] reset routing table entry for %s at %i\n",
              netaddr_to_string(&nbuf, &routing_table[i].addr), i);
        memset(&routing_table[i], 0, sizeof(routing_table[i]));
    }
}

bool aodvv2_routingtable_offers_improvement(aodvv2_routing_entry_t *rt_entry,
                                            node_data_t *node_data)
{
    /* Check if new info is stale */
    if (aodvv2_seqnum_cmp(node_data->seqnum, rt_entry->seqnum) == -1) {
        return false;
    }
    /* Check if new info is more costly */
    if ((node_data->metric >= rt_entry->metric)
        && !(rt_entry->state != ROUTE_STATE_BROKEN)) {
        return false;
    }
    /* Check if new info repairs a broken route */
    if (!(rt_entry->state != ROUTE_STATE_BROKEN)) {
        return false;
    }
    return true;
}

void aodvv2_routingtable_fill_routing_entry_rreq(aodvv2_packet_data_t *packet_data,
                                                 aodvv2_routing_entry_t *rt_entry,
                                                 uint8_t link_cost)
{
    rt_entry->addr = packet_data->orig_node.addr;
    rt_entry->seqnum = packet_data->orig_node.seqnum;
    rt_entry->nextHopAddr = packet_data->sender;
    rt_entry->lastUsed = packet_data->timestamp;
    rt_entry->expirationTime = timex_add(packet_data->timestamp, validity_t);
    rt_entry->metricType = packet_data->metric_type;
    rt_entry->metric = packet_data->orig_node.metric + link_cost;
    rt_entry->state = ROUTE_STATE_ACTIVE;
}

void aodvv2_routingtable_fill_routing_entry_rrep(aodvv2_packet_data_t *packet_data,
                                                 aodvv2_routing_entry_t *rt_entry,
                                                 uint8_t link_cost)
{
    rt_entry->addr = packet_data->targ_node.addr;
    rt_entry->seqnum = packet_data->targ_node.seqnum;
    rt_entry->nextHopAddr = packet_data->sender;
    rt_entry->lastUsed = packet_data->timestamp;
    rt_entry->expirationTime = timex_add(packet_data->timestamp, validity_t);
    rt_entry->metricType = packet_data->metric_type;
    rt_entry->metric = packet_data->targ_node.metric + link_cost;
    rt_entry->state = ROUTE_STATE_ACTIVE;
}
