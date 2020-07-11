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
#include "_aodvv2-mcmsg.h"

#include "xtimer.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

static void _reset_entry_if_stale(_aodvv2_mcmsg_t *entry);
static _aodvv2_mcmsg_t *_find_comparable_entry(_aodvv2_mcmsg_t *mcmsg);

static _aodvv2_mcmsg_t _mcmsg[CONFIG_AODVV2_MCMSG_MAX_ENTRIES];
static mutex_t _lock = MUTEX_INIT;

static timex_t _max_seqnum_lifetime;

void _aodvv2_mcmsg_init(void)
{
    DEBUG("aodvv2: initializing multicast message set\n");
    mutex_lock(&_lock);

    _max_seqnum_lifetime = timex_set(CONFIG_AODVV2_MAX_SEQNUM_LIFETIME, 0);

    memset(&_mcmsg, 0, sizeof(_mcmsg));
    mutex_unlock(&_lock);
}

int _aodvv2_mcmsg_process(_aodvv2_mcmsg_t *mcmsg)
{
    DEBUG("aodvv2: process McMsg %p\n", (void *)mcmsg);
    mutex_lock(&_lock);

    _aodvv2_mcmsg_t *comparable = _find_comparable_entry(mcmsg);
    if (comparable == NULL) {
        DEBUG("  adding new McMsg\n");
        if (_aodvv2_mcmsg_alloc(mcmsg) == NULL) {
            DEBUG("  McMsg set is full\n");
        }
        mutex_unlock(&_lock);
        return AODVV2_MCMSG_OK;
    }

    DEBUG("  comparable McMsg found\n");

    /* There's a comparable entry, update it's timing information */
    timex_t current_time;
    xtimer_now_timex(&current_time);

    comparable->timestamp = current_time;
    comparable->removal_time = timex_add(current_time, _max_seqnum_lifetime);

    int seqcmp = _aodvv2_seqnum_cmp(comparable->orig_seqnum, mcmsg->orig_seqnum);
    if (seqcmp < 0) {
        DEBUG("  stored McMsg is newer\n");
        mutex_unlock(&_lock);
        return AODVV2_MCMSG_REDUNDANT;
    }

    if (seqcmp == 0) {
        if (comparable->metric <= mcmsg->metric) {
            DEBUG("  stored McMsg is no worse than received\n");
            mutex_unlock(&_lock);
            return AODVV2_MCMSG_REDUNDANT;
        }
    }

    if (seqcmp > 0) {
        DEBUG("  received McMsg is newer than stored\n");
    }

    comparable->orig_seqnum = mcmsg->orig_seqnum;
    comparable->metric = mcmsg->metric;

    /* Search for compatible entries and compare their metrics */
    for (unsigned i = 0; i < ARRAY_SIZE(_mcmsg); i++) {
        _aodvv2_mcmsg_t *tmp = &_mcmsg[i];
        _reset_entry_if_stale(tmp);

        if ((tmp == comparable) || (!tmp->used)) {
            continue;
        }

        if (_aodvv2_mcmsg_is_compatible(comparable, tmp)) {
            if (tmp->metric <= comparable->metric) {
                DEBUG("  received McMsg is worse than stored\n");
                mutex_unlock(&_lock);
                return AODVV2_MCMSG_REDUNDANT;
            }
        }
    }

    mutex_unlock(&_lock);
    return AODVV2_MCMSG_OK;
}

_aodvv2_mcmsg_t *_aodvv2_mcmsg_alloc(_aodvv2_mcmsg_t *entry)
{
    DEBUG("aodvv2: allocating McMsg entry\n");

    _aodvv2_mcmsg_t *mcmsg = NULL;
    for (unsigned i = 0; i < ARRAY_SIZE(_mcmsg); i++) {
        _aodvv2_mcmsg_t *tmp = &_mcmsg[i];

        if (!tmp->used) {
            mcmsg = tmp;
            break;
        }
    }

    if (mcmsg != NULL) {
        memcpy(mcmsg, entry, sizeof(_aodvv2_mcmsg_t));

        timex_t current_time;
        xtimer_now_timex(&current_time);

        mcmsg->timestamp = current_time;
        mcmsg->removal_time = timex_add(current_time, _max_seqnum_lifetime);
        mcmsg->used = true;
    }
#if ENABLE_DEBUG
    else {
        DEBUG("  MCMSG FULL!\n");
    }
#endif

    return mcmsg;
}

bool _aodvv2_mcmsg_is_compatible(_aodvv2_mcmsg_t *a, _aodvv2_mcmsg_t *b)
{
    /* A RREQ is considered compatible if they both contain the same OrigPrefix,
     * OrigPrefixLength, TargPrefix and MetricType */
    return ipv6_addr_equal(&a->orig_prefix, &b->orig_prefix) &&
           ipv6_addr_equal(&a->targ_prefix, &b->targ_prefix) &&
           (a->orig_pfx_len == b->orig_pfx_len) &&
           (a->metric_type == b->metric_type);
}

bool _aodvv2_mcmsg_is_comparable(_aodvv2_mcmsg_t *a, _aodvv2_mcmsg_t *b)
{
    /* At least one of the McMsg provided a SeqNoRtr address, so it needs to be
     * checked if they're the same in order for the McMsgs to be comparable */
    return _aodvv2_mcmsg_is_compatible(a, b) &&
           ipv6_addr_equal(&a->seqnortr, &b->seqnortr);
}

bool _aodvv2_mcmsg_is_stale(_aodvv2_mcmsg_t *mcmsg)
{
    timex_t current_time;
    xtimer_now_timex(&current_time);

    return timex_cmp(current_time, mcmsg->removal_time) >= 0;
}

static void _reset_entry_if_stale(_aodvv2_mcmsg_t *entry)
{
    if (!entry->used) {
        return;
    }

    if (_aodvv2_mcmsg_is_stale(entry)) {
        DEBUG("aodvv2: resetting stale entry %p\n", (void *)entry);
        memset(&entry, 0, sizeof(_aodvv2_mcmsg_t));
        entry->used = false;
    }
}

static _aodvv2_mcmsg_t *_find_comparable_entry(_aodvv2_mcmsg_t *mcmsg)
{
    for (unsigned i = 0; i < ARRAY_SIZE(_mcmsg); i++) {
        _aodvv2_mcmsg_t *tmp = &_mcmsg[i];
        _reset_entry_if_stale(tmp);

        if (_aodvv2_mcmsg_is_comparable(tmp, mcmsg) && tmp->used) {
            return tmp;
        }
    }

    return NULL;
}
