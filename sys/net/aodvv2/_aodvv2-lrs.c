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
 * @brief       AODVv2 Local Route Set
 *
 * @author      Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 */

#include <errno.h>

#include "_aodvv2-lrs.h"

#include "net/aodvv2/conf.h"
#include "net/gnrc/ipv6/nib/ft.h"

#include "rmutex.h"
#include "xtimer.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

static int _lrs_update_set(aodvv2_message_t *advrte, const ipv6_addr_t *sender,
                           const uint16_t iface, _aodvv2_local_route_t **matches,
                           unsigned match_count);
static void _lrs_update(_aodvv2_local_route_t *lr, aodvv2_seqnum_t seqnum,
                        const ipv6_addr_t *next_hop,
                        const uint16_t iface, const uint8_t metric,
                        const ipv6_addr_t *seqnortr);

static _aodvv2_local_route_t _lr[CONFIG_AODVV2_LRS_MAX_ENTRIES];
static rmutex_t _lock = RMUTEX_INIT;

static char addr_str[IPV6_ADDR_MAX_STR_LEN];

void _aodvv2_lrs_init(void)
{
    DEBUG("aodvv2: initializing local route set\n");

    _aodvv2_lrs_acquire();
    memset(&_lr, 0, sizeof(_lr));
    _aodvv2_lrs_release();
}

void _aodvv2_lrs_acquire(void)
{
    rmutex_lock(&_lock);
}

void _aodvv2_lrs_release(void)
{
    rmutex_unlock(&_lock);
}

int _aodvv2_lrs_process(aodvv2_message_t *rtemsg, const ipv6_addr_t *sender,
                        const uint16_t iface)
{
    DEBUG("aodvv2: processing RteMsg %p\n", (void *)rtemsg);
    DEBUG("  sender = %s, iface = %d\n",
          (sender == NULL ) ? "NULL": ipv6_addr_to_str(addr_str, sender,
                                                       sizeof(addr_str)),
          iface);

    if (rtemsg == NULL || sender == NULL) {
        DEBUG("  invalid RteMsg or sender\n");
        return -EINVAL;
    }

    unsigned match_count = 0;
    _aodvv2_local_route_t *matches[CONFIG_AODVV2_LRS_MAX_ENTRIES];

    DEBUG("  searching for matching Local Routes\n");

    _aodvv2_lrs_acquire();
    for (unsigned i = 0; i < ARRAY_SIZE(_lr); i++) {
        _aodvv2_local_route_t *tmp = &_lr[i];

        if (!tmp->used) {
            if (_aodvv2_lrs_match(tmp, rtemsg)) {
                DEBUG("  Local Route matched %i (%p)\n", i, (void *)tmp);
                matches[match_count] = tmp;
                match_count += 1;
            }
        }
    }

    if (match_count == 0) {
        DEBUG("  no matching Local Routes found\n");
        _lrs_update_set(rtemsg, sender, iface, matches, match_count);
        _aodvv2_lrs_release();
        return 0;
    }

    DEBUG("  found %d match Local Routes\n", match_count);

    _aodvv2_lrs_release();
    return 0;
}

_aodvv2_local_route_t *_aodvv2_lrs_find(const ipv6_addr_t *dst)
{
    DEBUG("aodvv2: finding route to %s\n",
          (dst == NULL) ? "NULL" : ipv6_addr_to_str(addr_str, dst,
                                                    sizeof(addr_str)));

    if (dst == NULL || ipv6_addr_is_unspecified(dst)) {
        DEBUG("  invalid address");
        return NULL;
    }

    /* Find the entry with the highest matching prefix */
    _aodvv2_local_route_t *lr = NULL;
    for (unsigned i = 0; i < ARRAY_SIZE(_lr); i++) {
        _aodvv2_local_route_t *tmp = &_lr[i];

        if (!tmp->used) {
            continue;
        }

        if ((ipv6_addr_match_prefix(&tmp->addr, dst) >=tmp->pfx_len) &&
            (tmp->state != AODVV2_ROUTE_STATE_INVALID)) {
            if (lr == NULL) {
                DEBUG("  found route match %s/%d\n",
                      ipv6_addr_to_str(addr_str, &tmp->addr, sizeof(addr_str)),
                      tmp->pfx_len);
                lr = tmp;
            }
            else if (lr->pfx_len < tmp->pfx_len) {
                DEBUG("  longest route match %s/%d\n",
                      ipv6_addr_to_str(addr_str, &tmp->addr, sizeof(addr_str)),
                      tmp->pfx_len);
                lr = tmp;
            }
        }
    }

#if ENABLE_DEBUG
    if (lr == NULL) {
        DEBUG("  no matching route found\n");
    }
#endif

    return lr;
}

static int _lrs_update_set(aodvv2_message_t *advrte, const ipv6_addr_t *sender,
                           const uint16_t iface, _aodvv2_local_route_t **matches,
                           unsigned match_count)
{
    assert(advrte != NULL);
    assert(matches != NULL);
    assert(match_count < ARRAY_SIZE(_lr));

    if (match_count == 0) {
        const ipv6_addr_t *addr = NULL;
        uint8_t pfx_len = 0;
        uint8_t metric_type = 0;
        uint8_t metric = 0;
        _advrte_get_addr(advrte, &addr, &pfx_len);
        _advrte_get_metric(advrte, &metric_type, &metric);

        _aodvv2_local_route_t *entry = NULL;
        if ((entry = _aodvv2_lrs_alloc(addr, pfx_len, metric_type)) == NULL) {
            return -ENOSPC;
        }

        aodvv2_seqnum_t seqnum = 0;
        const ipv6_addr_t *seqnortr = NULL;

        _advrte_get_seqnum(advrte, &seqnum);
        _advrte_get_seqnortr(advrte, &seqnortr);

        _lrs_update(entry, seqnum, sender, iface,
                    metric, seqnortr);
    }

    return 0;
}

_aodvv2_local_route_t *_aodvv2_lrs_alloc(const ipv6_addr_t *addr,
                                         uint8_t pfx_len,
                                         uint8_t metric_type)
{
    DEBUG("aodvv2: allocating route (addr = %s/%d, metric_type = %d)\n",
          (addr == NULL) ? "NULL" : ipv6_addr_to_str(addr_str, addr,
                                                     sizeof(addr_str)), pfx_len,
          metric_type);

    if (pfx_len == 0 || addr == NULL || ipv6_addr_is_unspecified(addr)) {
        DEBUG("  invalid parameters\n");
        return NULL;
    }

    if (pfx_len > 128) {
        pfx_len = 128;
    }

    _aodvv2_local_route_t *lr = NULL;
    for (unsigned i = 0; i < ARRAY_SIZE(_lr); i++) {
        _aodvv2_local_route_t *tmp= &_lr[i];

        if (!tmp->used) {
            lr = tmp;
            break;
        }
    }

    if (lr != NULL) {
        memcpy(&lr->addr, addr, sizeof(ipv6_addr_t));
        lr->pfx_len = pfx_len;
        lr->metric_type = metric_type;

        lr->used = true;
    }
#if ENABLE_DEBUG
    else {
        DEBUG("  LRS FULL!\n");
    }
#endif

    return lr;
}

static void _lrs_update(_aodvv2_local_route_t *lr, aodvv2_seqnum_t seqnum,
                        const ipv6_addr_t *next_hop,
                        const uint16_t iface, const uint8_t metric,
                        const ipv6_addr_t *seqnortr)
{
    assert(lr != NULL);
    assert(next_hop != NULL);
    assert(seqnortr != NULL);

    lr->seqnum = seqnum;
    lr->next_hop = *next_hop;
    lr->iface = iface;
    lr->metric = metric;

    timex_t now;
    xtimer_now_timex(&now);
    lr->last_used = now;
    lr->last_seqnum_update = now;

    lr->seqnortr = *seqnortr;
}

bool _aodvv2_lrs_match(_aodvv2_local_route_t *lr, aodvv2_message_t *advrte)
{
    assert(lr != NULL);
    assert(advrte != NULL);
    assert(advrte->type == AODVV2_MSGTYPE_RREQ ||
           advrte->type == AODVV2_MSGTYPE_RREP);

    uint8_t metric_type = 0;
    uint8_t metric = 0;
    uint8_t pfx_len = 0;
    const ipv6_addr_t *addr = NULL;
    const ipv6_addr_t *seqnortr = NULL;

    _advrte_get_metric(advrte, &metric_type, &metric);
    _advrte_get_addr(advrte, &addr, &pfx_len);
    _advrte_get_seqnortr(advrte, &seqnortr);

    if ((ipv6_addr_match_prefix(&lr->addr, addr) >= lr->pfx_len) &&
        lr->pfx_len == pfx_len &&
        lr->metric_type == metric_type &&
        ipv6_addr_equal(&lr->seqnortr, seqnortr)) {
        return true;
    }

    return false;
}

int aodvv2_lrs_set_active(_aodvv2_local_route_t *lr)
{
    assert(lr != NULL);

    DEBUG("aodvv2: setting LocalRoute[%s/%d] to ACTIVE\n",
          ipv6_addr_to_str(addr_str, &lr->addr, sizeof(addr_str)), lr->iface);

    timex_t now;
    xtimer_now_timex(&now);

    lr->state = AODVV2_ROUTE_STATE_ACTIVE;
    lr->last_used = now;

    return gnrc_ipv6_nib_ft_add(&lr->addr, lr->pfx_len, &lr->next_hop,
                                lr->iface, 0);
}

void aodvv2_lrs_set_invalid(_aodvv2_local_route_t *lr)
{
    assert(lr != NULL);

    DEBUG("aodvv2: setting LocalRoute[%s/%d] to INVALID\n",
          ipv6_addr_to_str(addr_str, &lr->addr, sizeof(addr_str)), lr->iface);

    lr->state = AODVV2_ROUTE_STATE_INVALID;

    gnrc_ipv6_nib_ft_del(&lr->addr, lr->pfx_len);
}
