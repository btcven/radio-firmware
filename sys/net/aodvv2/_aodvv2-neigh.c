/*
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
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 * @}
 */

#include "net/aodvv2/conf.h"

#include "_aodvv2-neigh.h"
#include "_aodvv2-writer.h"

#include "xtimer.h"
#include "random.h"
#include "rmutex.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

rmutex_t _aodvv2_lock = RMUTEX_INIT;

static _aodvv2_neigh_t _neigh[CONFIG_AODVV2_NEIGH_MAX_ENTRIES];

static char addr_str[IPV6_ADDR_MAX_STR_LEN];

void _aodvv2_neigh_init(void)
{
    DEBUG("aodvv2: initializing neighbor set\n");

    _aodvv2_neigh_acquire();
    memset(_neigh, 0, sizeof(_neigh));
    _aodvv2_neigh_release();
}

void _aodvv2_neigh_acquire(void)
{
    rmutex_lock(&_aodvv2_lock);
}

void _aodvv2_neigh_release(void)
{
    rmutex_unlock(&_aodvv2_lock);
}

static inline bool _addr_equals(const ipv6_addr_t *addr,
                                const _aodvv2_neigh_t *neigh)
{
    return (addr == NULL) || ipv6_addr_is_unspecified(&neigh->addr) ||
           (ipv6_addr_equal(addr, &neigh->addr));
}

_aodvv2_neigh_t *_aodvv2_neigh_alloc(const ipv6_addr_t *addr, uint16_t iface)
{
    DEBUG("aodvv2: allocation neighbor entry (addr = %s, iface = %d)\n",
           (addr == NULL) ? "NULL" : ipv6_addr_to_str(addr_str, addr,
                                                      sizeof(addr_str)),iface);
    _aodvv2_neigh_t *neigh = NULL;
    for (unsigned i = 0; i < ARRAY_SIZE(_neigh); i++) {
        _aodvv2_neigh_t *tmp = &_neigh[i];
        _aodvv2_neigh_upd_state(tmp);
        if (tmp->iface == iface && _addr_equals(addr, tmp)) {
            DEBUG("  %p is an exact match", (void *)tmp);
            neigh = tmp;
            break;
        }
        if ((neigh == NULL) && (!tmp->used)) {
            DEBUG("using %p", (void *)tmp);
            neigh = tmp;
        }
    }

    if (neigh != NULL && !neigh->used) {
        if (addr != NULL) {
            memcpy(&neigh->addr, addr, sizeof(ipv6_addr_t));
        }
        neigh->iface = iface;
        neigh->used = true;

        if (gnrc_rfc5444_add_writer_target(&neigh->addr, neigh->iface) < 0) {
            DEBUG(" couldn't register writer target\n");
        }
    }
#if ENABLE_DEBUG
    else {
        DEBUG("   NEIGH full!\n");
    }
#endif

    return neigh;
}

_aodvv2_neigh_t *_aodvv2_neigh_get(const ipv6_addr_t *addr, uint16_t iface)
{
    DEBUG("aodvv2: processing neighbor information (addr = %s, iface = %d)\n",
          (addr == NULL) ? "NULL" : ipv6_addr_to_str(addr_str, addr,
                                                     sizeof(addr_str)),
          iface);

    _aodvv2_neigh_t *neigh = NULL;
    for (unsigned i = 0; i < ARRAY_SIZE(_neigh); i++) {
        _aodvv2_neigh_t *tmp = &_neigh[i];

        if (tmp->used && _addr_equals(addr, tmp) && tmp->iface == iface) {
            neigh = tmp;
            break;
        }
    }

    if (neigh == NULL) {
        DEBUG("  no matching neighbor found, creating new one\n");
        neigh = _aodvv2_neigh_alloc(addr, iface);
        _aodvv2_neigh_set_heard(neigh, false);
        neigh->ackseqnum = random_uint32_range(0, UINT16_MAX);
        neigh->heard_rerr_seqnum = 0;
        return neigh;
    }

    return neigh;
}

void _aodvv2_neigh_upd_state(_aodvv2_neigh_t *neigh)
{
    if (!neigh->used) {
        return;
    }

    timex_t now;
    xtimer_now_timex(&now);
    switch (neigh->state) {
        case AODVV2_NEIGH_STATE_BLACKLISTED:
            if (!_timex_is_zero(neigh->timeout) &&
                timex_cmp(now, neigh->timeout) > 0) {
                DEBUG("aodvv2: blacklisted neighbor coming back to heard (addr = %s, iface = %d)\n",
                      ipv6_addr_to_str(addr_str, &neigh->addr, sizeof(addr_str)),
                      neigh->iface);
                _aodvv2_neigh_set_heard(neigh, false);
            }
            break;

        case AODVV2_NEIGH_STATE_HEARD:
            /* If timeout is not zero, then a RREP_Ack has been requested, check
             * if it timed out */
            if (!_timex_is_zero(neigh->timeout) &&
                timex_cmp(now, neigh->timeout)) {
                DEBUG("aodvv2: blacklistig heard neighbor (addr = %s, iface = %d)\n",
                      ipv6_addr_to_str(addr_str, &neigh->addr, sizeof(addr_str)),
                      neigh->iface);
                DEBUG("  RREP_Ack timed out\n");
                neigh->state = AODVV2_NEIGH_STATE_BLACKLISTED;
                xtimer_now_timex(&now);
                /* TODO(jeandudey): blacklist time */
                neigh->timeout = timex_add(now, timex_set(300, 0));
            }
            break;

        default:
            break;
    }
}

void _aodvv2_neigh_set_heard(_aodvv2_neigh_t *neigh, bool reqack)
{
    DEBUG("aodvv2: setting neighbor to \"heard\" (addr = %s, iface = %d)",
          ipv6_addr_to_str(addr_str, &neigh->addr, sizeof(addr_str)),
          neigh->iface);

    neigh->timeout = timex_set(0, 0);
    neigh->state = AODVV2_NEIGH_STATE_HEARD;

    if (reqack) {
        _aodvv2_req_ack(neigh);
    }
}

void _aodvv2_req_ack(_aodvv2_neigh_t *neigh)
{
    DEBUG("aodvv2: sending RREP_Ack request (addr = %s, iface = %d)",
          ipv6_addr_to_str(addr_str, &neigh->addr, sizeof(addr_str)),
          neigh->iface);

    timex_t now;
    xtimer_now_timex(&now);
    neigh->timeout = timex_add(now, timex_set(CONFIG_AODVV2_RREP_ACK_SENT_TIMEOUT, 0));

    aodvv2_msg_rrep_ack_t rrep_ack = {
        .ackreq = 1,
        .timestamp = neigh->ackseqnum,
    };
    _aodvv2_writer_send_rrep_ack(&rrep_ack, &neigh->addr, neigh->iface);
}
