/*
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
 * @brief       AODVv2 packet buffering
 *
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 * @}
 */

#include "_aodvv2-buffer.h"

#include "net/aodvv2/conf.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

static _aodvv2_buffered_pkt_t _pkt[CONFIG_AODVV2_BUFFER_MAX_ENTRIES];
static mutex_t _lock = MUTEX_INIT;
static char addr_str[IPV6_ADDR_MAX_STR_LEN];

void _aodvv2_buffer_init(void)
{
    DEBUG("aodvv2: initializing packet buffer\n");
    mutex_lock(&_lock);
    memset(_pkt, 0, sizeof(_pkt));
    mutex_unlock(&_lock);
}

int _aodvv2_buffer_pkt_add(gnrc_pktsnip_t *pkt)
{
    DEBUG("aodvv2: adding pkt = %p to packet buffer\n", (void *)pkt);

    mutex_lock(&_lock);
    _aodvv2_buffered_pkt_t *entry = NULL;
    for (unsigned i = 0; i < ARRAY_SIZE(_pkt); i++) {
        _aodvv2_buffered_pkt_t *tmp = &_pkt[i];
        if (!tmp->used) {
            entry = tmp;
            break;
        }
    }

    if (entry == NULL) {
        DEBUG("  packet buffer is full\n");
        mutex_unlock(&_lock);
        return -ENOMEM;
    }

    entry->used = true;
    entry->pkt = pkt;
    /* Increase reference count for this packet as we'll l store it until we
     * find a route to send it (or not, and release the packet) */
    gnrc_pktbuf_hold(entry->pkt, 1);
    mutex_unlock(&_lock);
    return 0;
}

void _aodvv2_buffer_dispatch(const ipv6_addr_t *targ_prefix, uint8_t pfx_len)
{
    DEBUG("aodvv2: dispatching packets for %s/%d\n",
          (targ_prefix == NULL) ? "NULL" : ipv6_addr_to_str(addr_str,
                                                            targ_prefix,
                                                            sizeof(addr_str)),
          pfx_len);

    if (pfx_len > 128 || pfx_len == 0) {
        DEBUG("  invalid prefix len %d\n", pfx_len);
        pfx_len = 128;
    }

    mutex_lock(&_lock);
    ipv6_hdr_t *hdr = NULL;
    for (unsigned i = 0; i < ARRAY_SIZE(_pkt); i++) {
        _aodvv2_buffered_pkt_t *tmp = &_pkt[i];

        if (tmp->used) {
            hdr = gnrc_ipv6_get_header(tmp->pkt);
            if (hdr == NULL) {
                DEBUG("  IPv6 header not found\n");
                continue;
            }

            if (ipv6_addr_match_prefix(&hdr->dst,
                                       targ_prefix) >= pfx_len) {
                DEBUG(" match for %s\n",
                      ipv6_addr_to_str(addr_str, targ_prefix,
                                       sizeof(addr_str)));

                if (gnrc_netapi_dispatch_send(GNRC_NETTYPE_IPV6,
                                              GNRC_NETREG_DEMUX_CTX_ALL,
                                              tmp->pkt) < 1) {
                    DEBUG("  failed to dispatch packet\n");
                }
                tmp->used = false;
                tmp->pkt = NULL;
            }
        }
    }
    mutex_unlock(&_lock);
}

