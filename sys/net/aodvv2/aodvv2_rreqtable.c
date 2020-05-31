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
 * @brief       AODVv2 routing protocol
 *
 * @author      Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 * @author      Gustavo Grisales <gustavosinbandera1@hotmail.com>
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 * @}
 */

#include "net/aodvv2/aodvv2.h"
#include "net/aodvv2/rreqtable.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

static aodvv2_rreq_entry_t *_get_comparable_rreq(aodvv2_packet_data_t *packet_data);
static void _reset_entry_if_stale(uint8_t i);

static mutex_t rreqt_mutex;

static aodvv2_rreq_entry_t rreq_table[AODVV2_RREQ_BUF];

static timex_t _null_time;
static timex_t now;
static timex_t _max_idletime;

void aodvv2_rreqtable_init(void)
{
    DEBUG("aodvv2_rreqtable_init()\n");
    mutex_lock(&rreqt_mutex);

    _null_time = timex_set(0, 0);
    _max_idletime = timex_set(CONFIG_AODVV2_MAX_IDLETIME, 0);

    memset(&rreq_table, 0, sizeof(rreq_table));
    mutex_unlock(&rreqt_mutex);
}

bool aodvv2_rreqtable_is_redundant(aodvv2_packet_data_t *packet_data)
{
    aodvv2_rreq_entry_t *comparable_rreq;
    timex_t now;
    bool result;

    mutex_lock(&rreqt_mutex);
    comparable_rreq = _get_comparable_rreq(packet_data);

    /* if there is no comparable rreq stored, add one and return false */
    if (comparable_rreq == NULL) {
        aodvv2_rreqtable_add(packet_data);
        result = false;
    }
    else {
        int seqnum_comparison = aodvv2_seqnum_cmp(packet_data->orig_node.seqnum,
                                                  comparable_rreq->seqnum);

        /*
         * If two RREQs have the same
         * metric type and OrigNode and Targnode addresses, the information from
         * the one with the older Sequence Number is not needed in the table
         */
        if (seqnum_comparison < 0) {
            result = true;
        }

        if (seqnum_comparison > 0) {
            /* Update RREQ table entry with new seqnum value */
            comparable_rreq->seqnum = packet_data->orig_node.seqnum;
        }

        /*
         * in case they have the same Sequence Number, the one with the greater
         * Metric value is not needed
         */
        if (seqnum_comparison == 0) {
            if (comparable_rreq->metric <= packet_data->orig_node.metric) {
                result = true;
            }
            /* Update RREQ table entry with new metric value */
            comparable_rreq->metric = packet_data->orig_node.metric;
        }

        /* Since we've changed RREQ info, update the timestamp */
        xtimer_now_timex(&now);
        comparable_rreq->timestamp = now;
        result = true;
    }

    mutex_unlock(&rreqt_mutex);
    return result;
}

/*
 * retrieve pointer to a comparable (according to Section 6.7.)
 * RREQ table entry if it exists and NULL otherwise.
 * Two AODVv2 RREQ messages are comparable if:
 * - they have the same metric type
 * - they have the same OrigNode and TargNode addresses
 */
static aodvv2_rreq_entry_t *_get_comparable_rreq(aodvv2_packet_data_t *packet_data)
{
    for (unsigned i = 0; i < ARRAY_SIZE(rreq_table); i++) {
        _reset_entry_if_stale(i);

        if (ipv6_addr_equal(&rreq_table[i].origNode, &packet_data->orig_node.addr) &&
            ipv6_addr_equal(&rreq_table[i].targNode, &packet_data->targ_node.addr) &&
            rreq_table[i].metricType == packet_data->metric_type) {
            return &rreq_table[i];
        }
    }

    return NULL;
}

void aodvv2_rreqtable_add(aodvv2_packet_data_t *packet_data)
{
    if (_get_comparable_rreq(packet_data)) {
        return;
    }
    /*find empty rreq and fill it with packet_data */

    for (unsigned i = 0; i < ARRAY_SIZE(rreq_table); i++) {
        if (!rreq_table[i].timestamp.seconds &&
            !rreq_table[i].timestamp.microseconds) {
            rreq_table[i].origNode = packet_data->orig_node.addr;
            rreq_table[i].targNode = packet_data->targ_node.addr;
            rreq_table[i].metricType = packet_data->metric_type;
            rreq_table[i].metric = packet_data->orig_node.metric;
            rreq_table[i].seqnum = packet_data->orig_node.seqnum;
            rreq_table[i].timestamp = packet_data->timestamp;
            return;
        }
    }
}

/*
 * Check if entry at index i is stale and clear the struct it fills if it is
 */
static void _reset_entry_if_stale(uint8_t i)
{
    xtimer_now_timex(&now);

    /* Null time means this entry is unused */
    if (timex_cmp(rreq_table[i].timestamp, _null_time) == 0) {
        return;
    }

    timex_t expiration_time = timex_add(rreq_table[i].timestamp, _max_idletime);
    if (timex_cmp(expiration_time, now) < 0) {
        memset(&rreq_table[i], 0, sizeof(rreq_table[i]));
    }
}
