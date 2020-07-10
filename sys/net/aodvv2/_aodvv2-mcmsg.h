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
 *
 * @{
 * @file
 * @brief       AODVv2 Multicast Message Set
 *
 * @author      Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 * @author      Gustavo Grisales <gustavosinbandera1@hotmail.com>
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 */

#ifndef PRIV_AODVV2_MCMSG_H
#define PRIV_AODVV2_MCMSG_H

#include <stdbool.h>
#include <stdint.h>

#include "net/aodvv2/seqnum.h"
#include "net/ipv6/addr.h"

#include "timex.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   A Multicast Message
 */
typedef struct {
    ipv6_addr_t orig_prefix;      /**< OrigPrefix */
    uint8_t orig_pfx_len;         /**< OrigPrefix length */
    ipv6_addr_t targ_prefix;      /**< TargPrefix */
    aodvv2_seqnum_t orig_seqnum;  /**< SeqNum associated with OrigPrefix */
    aodvv2_seqnum_t targ_seqnum;  /**< SeqNum associated with TargPrefix */
    uint8_t metric_type;          /**< Metric type of the RREQ */
    uint8_t metric;               /**< Metric of the RREQ */
    timex_t timestamp;            /**< Last time this entry was updated */
    timex_t removal_time;         /**< Time at which this entry should be removed */
    uint16_t iface;               /**< Interface where this McMsg was received */
    ipv6_addr_t seqnortr;         /**< SeqNoRtr */
    bool used;                    /**< Is this entry used? */
} _aodvv2_mcmsg_t;

enum {
    AODVV2_MCMSG_REDUNDANT = -1,  /**< Processed McMsg is redundant */
    AODVV2_MCMSG_OK = 0           /**< McMsg is new (ok) */
};

/**
 * @brief   Initialize RREQ table.
 */
void _aodvv2_mcmsg_init(void);

/**
 * @brief   Process an RREQ
 *
 * @param[in] msg RREQ message
 *
 * @return AODVV2_MCMSG_OK processing went fine.
 * @return AODVV2_MCMSG_REDUNDANT message is redundant.
 */
int _aodvv2_mcmsg_process(_aodvv2_mcmsg_t *msg);

/**
 * @brief   Allocate a new Multicast Message entry.
 *
 * @param[in]  mcmsg Multicast Message entry.
 *
 * @return Pointer to @ref _aodvv2_mcmsg_t entry on success.
 * @return NULL if no space available.
 */
_aodvv2_mcmsg_t *_aodvv2_mcmsg_alloc(_aodvv2_mcmsg_t *entry);

/**
 * @brief   Are both Multicast Messages compatible?
 *
 * A RREQ is considered compatible if they both contain the same OrigPrefix,
 * OrigPrefixLength, TargPrefix and MetricType.
 *
 * @see [draft-perkins-manet-aodvv2-03, Section 6.8]
 *      (https://tools.ietf.org/html/draft-perkins-manet-aodvv2-03#section-6.8)
 *
 * @param[in]  a Mutlicast Message
 * @param[in]  b Multicast Message
 *
 * @return true both McMsg's are compatible
 * @return false both McMsg's aren't compatible
 */
bool _aodvv2_mcmsg_is_compatible(_aodvv2_mcmsg_t *a, _aodvv2_mcmsg_t *b);

/**
 * @brief   Are both Multicast Messages comparable?
 *
 * A RREQ is considered comparable if they both are compatible and SeqNoRtr are
 * equal.
 *
 * @see [draft-perkins-manet-aodvv2-03, Section 6.8]
 *      (https://tools.ietf.org/html/draft-perkins-manet-aodvv2-03#section-6.8)
 *
 * @param[in]  a Mutlicast Message
 * @param[in]  b Multicast Message
 *
 * @return true both McMsg's are comparable
 * @return false both McMsg's aren't comparable
 */
bool _aodvv2_mcmsg_is_comparable(_aodvv2_mcmsg_t *a, _aodvv2_mcmsg_t *b);

/**
 * @brief   Is this message stale?
 *
 * Compares the removal_time of the McMsg to the current time.
 *
 * @param[in]  mcmsg Mutlicast Message
 *
 * @return true stale
 * @return false not stale
 */
bool _aodvv2_mcmsg_is_stale(_aodvv2_mcmsg_t *mcmsg);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PRIV_AODVV2_MCMSG_H */
/** @} */
