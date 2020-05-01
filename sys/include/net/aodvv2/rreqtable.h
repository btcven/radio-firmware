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
 */

#ifndef NET_AODVV2_RREQTABLE_H
#define NET_AODVV2_RREQTABLE_H

#include "net/metric.h"
#include "net/aodvv2/seqnum.h"
#include "net/aodvv2/rfc5444.h"

#include "timex.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   should be enough for now...
 */
#define AODVV2_RREQ_BUF (16)

/**
 * @brief   RREQ wait time in seconds
 */
#define AODVV2_RREQ_WAIT_TIME (2)

/**
 * @brief   RREQ Table entry which stores all information about a RREQ that was
 *          received in order to avoid duplicates.
 */
typedef struct {
    ipv6_addr_t origNode; /**< Node which originated the RREQ*/
    ipv6_addr_t targNode; /**< Target (destination) of the RREQ */
    routing_metric_t metricType; /**< Metric type of the RREQ */
    uint8_t metric; /**< Metric of the RREQ */
    aodvv2_seqnum_t seqnum; /**< Sequence number of the RREQ */
    timex_t timestamp; /**< Last time this entry was updated */
} aodvv2_rreq_entry_t;

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

/**
 * @brief   Add a RREQ to the RREQ table
 *
 * @param[in] packet_data The packet to add.
 */
void aodvv2_rreqtable_add(aodvv2_packet_data_t *packet_data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NET_AODVV2_RREQTABLE_H */
