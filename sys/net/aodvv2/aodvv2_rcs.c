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
 * @ingroup     aodvv2
 * @{
 *
 * @file
 * @brief       AODVv2 Router Client Set
 *
 * @author      Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 * @author      Gustavo Grisales <gustavosinbandera1@hotmail.com>
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 * @}
 */

#include "net/aodvv2/conf.h"
#include "net/aodvv2/rcs.h"

#include "mutex.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

/**
 * @brief   Memory for the Client Set entries.
 */
typedef struct {
    aodvv2_router_client_t data; /**< Client data */
    bool used; /**< Is this entry used? */
} internal_entry_t;

static internal_entry_t _entries[CONFIG_AODVV2_RCS_MAX_ENTRIES];
static mutex_t _lock = MUTEX_INIT;

void aodvv2_rcs_init(void)
{
    mutex_lock(&_lock);
    memset(_entries, 0, sizeof(_entries));
    mutex_unlock(&_lock);
}

int aodvv2_rcs_add(const ipv6_addr_t *addr, uint8_t pfx_len, const uint8_t cost)
{
    assert(addr != NULL);

    if (pfx_len == 0 || ipv6_addr_is_unspecified(addr)) {
        DEBUG_PUTS("aodvv2: invalid client");
        return -EINVAL;
    }

    if (pfx_len > 128) {
        pfx_len = 128;
    }

    aodvv2_router_client_t entry;
    if (aodvv2_rcs_find(&entry, addr, pfx_len) == 0) {
        DEBUG_PUTS("aodvv2: client exists, not adding it");
        return -EEXIST;
    }

    mutex_lock(&_lock);
    for (unsigned i = 0; i < ARRAY_SIZE(_entries); i++) {
        internal_entry_t *entry = &_entries[i];

        /* Find free spot to place the new entry. */
        if (!entry->used) {
            ipv6_addr_init_prefix(&entry->data.addr, addr, pfx_len);
            entry->data.pfx_len = pfx_len;
            entry->data.cost = cost;

            entry->used = true;

            mutex_unlock(&_lock);
            return 0;
        }
    }

    DEBUG_PUTS("aodvv2: router client set is full");
    mutex_unlock(&_lock);
    return -ENOSPC;
}

int aodvv2_rcs_del(const ipv6_addr_t *addr, uint8_t pfx_len)
{
    assert(addr != NULL);

    if (pfx_len == 0 || ipv6_addr_is_unspecified(addr)) {
        return -EINVAL;
    }

    if (pfx_len > 128) {
        pfx_len = 128;
    }

    mutex_lock(&_lock);
    for (unsigned i = 0; i < ARRAY_SIZE(_entries); i++) {
        internal_entry_t *entry = &_entries[i];

        /* Skip unused entries */
        if (entry->used) {
            /* Compare addresses by prefix */
            if ((entry->data.pfx_len == pfx_len) &&
                (ipv6_addr_match_prefix(&entry->data.addr, addr) >= pfx_len)) {
                memset(&entry->data, 0, sizeof(aodvv2_router_client_t));
                entry->used = false;
                mutex_unlock(&_lock);
                return 0;
            }
        }
    }
    mutex_unlock(&_lock);
    return -ENOENT;
}

int aodvv2_rcs_find(aodvv2_router_client_t *client, const ipv6_addr_t *addr,
                    uint8_t pfx_len)
{
    assert(addr != NULL);
    if (ipv6_addr_is_unspecified(addr) || pfx_len == 0) {
        return -EINVAL;
    }

    if (pfx_len > 128) {
        pfx_len = 128;
    }

    mutex_lock(&_lock);
    for (unsigned i = 0; i < ARRAY_SIZE(_entries); i++) {
        internal_entry_t *entry = &_entries[i];

        /* Skip unused entries */
        if (!entry->used) {
            continue;
        }

        /* Compare addresses by prefix */
        if ((entry->data.pfx_len == pfx_len) &&
            (ipv6_addr_match_prefix(&entry->data.addr,
                                   addr) >= pfx_len)) {
            *client = entry->data;
            mutex_unlock(&_lock);
            return 0;
        }
    }

    /* No entry matches */
    mutex_unlock(&_lock);
    return -ENOENT;
}

int aodvv2_rcs_get(aodvv2_router_client_t *client, const ipv6_addr_t *addr)
{
    assert(addr != NULL);
    if (ipv6_addr_is_unspecified(addr)) {
        return -EINVAL;
    }

    mutex_lock(&_lock);
    internal_entry_t *best_match = NULL;
    for (unsigned i = 0; i < ARRAY_SIZE(_entries); i++) {
        internal_entry_t *entry = &_entries[i];

        /* Skip unused entries */
        if (entry->used) {
            /* Compare addresses by prefix */
            if (ipv6_addr_match_prefix(&entry->data.addr,
                                       addr) >= entry->data.pfx_len) {
                if (best_match == NULL) {
                    best_match = entry;
                }
                else {
                    /* Use the one with the highest matching prefix length */
                    if (best_match->data.pfx_len < entry->data.pfx_len) {
                        best_match = entry;
                    }
                }
            }
        }
    }

    if (best_match == NULL) {
        mutex_unlock(&_lock);
        return -ENOENT;
    }

    *client = best_match->data;

    mutex_unlock(&_lock);
    return 0;
}

void aodvv2_rcs_print_entries(void)
{
    char buf[IPV6_ADDR_MAX_STR_LEN];
    mutex_lock(&_lock);
    for (unsigned i = 0; i < ARRAY_SIZE(_entries); i++) {
        internal_entry_t *entry = &_entries[i];

        /* Skip unused entries */
        if (!entry->used) {
            continue;
        }

        /* prints ipv6/prefix | cost */
        printf("%s/%u | %u\n",
               ipv6_addr_to_str(buf, &entry->data.addr, sizeof(buf)),
               entry->data.pfx_len, entry->data.cost);
    }
    mutex_unlock(&_lock);
}
