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
 * @}
 */

#include "net/aodvv2/conf.h"
#include "net/aodvv2/mcmsg.h"

#include "xtimer.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

typedef struct {
    aodvv2_mcmsg_t data; /**< McMsg data */
    bool used;           /**< Is this entry used? */
} internal_entry_t;

static void _reset_entry_if_stale(internal_entry_t *entry);
static internal_entry_t *_find_comparable_entry(aodvv2_mcmsg_t *mcmsg);
static internal_entry_t *_add(aodvv2_mcmsg_t *mcmsg);

static internal_entry_t _entries[CONFIG_AODVV2_MCMSG_MAX_ENTRIES];
static mutex_t _lock = MUTEX_INIT;

static timex_t _max_seqnum_lifetime;

void aodvv2_mcmsg_init(void)
{
    DEBUG_PUTS("aodvv2: init McMset set");
    mutex_lock(&_lock);

    _max_seqnum_lifetime = timex_set(CONFIG_AODVV2_MAX_SEQNUM_LIFETIME, 0);

    memset(&_entries, 0, sizeof(_entries));
    mutex_unlock(&_lock);
}

int aodvv2_mcmsg_process(aodvv2_mcmsg_t *mcmsg)
{
    mutex_lock(&_lock);

    internal_entry_t *comparable = _find_comparable_entry(mcmsg);
    if (comparable == NULL) {
        DEBUG_PUTS("aodvv2: adding new McMsg");
        if (_add(mcmsg) == NULL) {
            DEBUG_PUTS("aodvv2: McMsg set is full");
        }
        mutex_unlock(&_lock);
        return AODVV2_MCMSG_OK;
    }

    DEBUG_PUTS("aodvv2: comparable McMsg found");

    /* There's a comparable entry, update it's timing information */
    timex_t current_time;
    xtimer_now_timex(&current_time);

    comparable->data.timestamp = current_time;
    comparable->data.removal_time = timex_add(current_time, _max_seqnum_lifetime);

    int seqcmp = aodvv2_seqnum_cmp(comparable->data.orig_seqnum, mcmsg->orig_seqnum);
    if (seqcmp < 0) {
        DEBUG_PUTS("aodvv2: stored McMsg is newer");
        mutex_unlock(&_lock);
        return AODVV2_MCMSG_REDUNDANT;
    }

    if (seqcmp == 0) {
        if (comparable->data.metric <= mcmsg->metric) {
            DEBUG_PUTS("aodvv2: stored McMsg is no worse than received");
            mutex_unlock(&_lock);
            return AODVV2_MCMSG_REDUNDANT;
        }
    }

    if (seqcmp > 0) {
        DEBUG_PUTS("aodvv2: received McMsg is newer than stored");
    }

    comparable->data.orig_seqnum = mcmsg->orig_seqnum;
    comparable->data.metric = mcmsg->metric;

    /* Search for compatible entries and compare their metrics */
    for (unsigned i = 0; i < ARRAY_SIZE(_entries); i++) {
        internal_entry_t *entry = &_entries[i];
        if (entry == comparable) {
            continue;
        }

        _reset_entry_if_stale(entry);

        if (entry->used && entry != comparable) {
            if (aodvv2_mcmsg_is_compatible(&comparable->data, &entry->data)) {
                if (entry->data.metric <= comparable->data.metric) {
                    DEBUG_PUTS("aodvv2: received McMsg is worse than stored");
                    mutex_unlock(&_lock);
                    return AODVV2_MCMSG_REDUNDANT;
                }
            }
        }
    }

    mutex_unlock(&_lock);
    return AODVV2_MCMSG_OK;
}

bool aodvv2_mcmsg_is_compatible(aodvv2_mcmsg_t *a, aodvv2_mcmsg_t *b)
{
    /* A RREQ is considered compatible if they both contain the same OrigPrefix,
     * OrigPrefixLength, TargPrefix and MetricType */
    if (ipv6_addr_equal(&a->orig_prefix, &b->orig_prefix) &&
        (a->orig_pfx_len == b->orig_pfx_len) &&
        ipv6_addr_equal(&a->targ_prefix, &b->targ_prefix) &&
        (a->metric_type == b->metric_type)) {
        return true;
    }

    return false;
}

bool aodvv2_mcmsg_is_comparable(aodvv2_mcmsg_t *a, aodvv2_mcmsg_t *b)
{
    /* At least one of the McMsg provided a SeqNoRtr address, so it needs to be
     * checked if they're the same in order for the McMsgs to be comparable */
    if (aodvv2_mcmsg_is_compatible(a, b) &&
        ipv6_addr_equal(&a->seqnortr, &b->seqnortr)) {
        return true;
    }

    return false;
}

bool aodvv2_mcmsg_is_stale(aodvv2_mcmsg_t *mcmsg)
{
    timex_t current_time;
    xtimer_now_timex(&current_time);

    return timex_cmp(current_time, mcmsg->removal_time) >= 0;
}

static void _reset_entry_if_stale(internal_entry_t *entry)
{
    if (!entry->used) {
        return;
    }

    if (aodvv2_mcmsg_is_stale(&entry->data)) {
        DEBUG_PUTS("aodvv2: McMsg is stale");
        memset(&entry->data, 0, sizeof(entry->data));
        entry->used = false;
    }
}

static internal_entry_t *_find_comparable_entry(aodvv2_mcmsg_t *mcmsg)
{
    for (unsigned i = 0; i < ARRAY_SIZE(_entries); i++) {
        internal_entry_t *entry = &_entries[i];
        _reset_entry_if_stale(entry);

        if (entry->used) {
            if (aodvv2_mcmsg_is_comparable(&entry->data, mcmsg)) {
                return entry;
            }
        }
    }

    return NULL;
}

static internal_entry_t *_add(aodvv2_mcmsg_t *mcmsg)
{
    /* Find empty McMsg and fill it */
    for (unsigned i = 0; i < ARRAY_SIZE(_entries); i++) {
        internal_entry_t *entry = &_entries[i];

        if (!entry->used) {
            timex_t current_time;
            xtimer_now_timex(&current_time);

            entry->used = true;
            entry->data.orig_prefix = mcmsg->orig_prefix;
            entry->data.orig_pfx_len = mcmsg->orig_pfx_len;
            entry->data.targ_prefix = mcmsg->targ_prefix;
            entry->data.orig_seqnum = mcmsg->orig_seqnum;
            entry->data.targ_seqnum = mcmsg->targ_seqnum;
            entry->data.metric_type = mcmsg->metric_type;
            entry->data.metric = mcmsg->metric;

            entry->data.timestamp = current_time;
            entry->data.removal_time = timex_add(current_time, _max_seqnum_lifetime);

            entry->data.iface = mcmsg->iface;
            return entry;
        }
    }

    return NULL;
}
