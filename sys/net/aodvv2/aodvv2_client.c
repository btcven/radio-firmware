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

#include "net/aodvv2/client.h"

#include "mutex.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

static aodvv2_client_entry_t _client_set[CONFIG_AODVV2_CLIENT_SET_ENTRIES];
static mutex_t _set_mutex = MUTEX_INIT;
static char ipbuf[IPV6_ADDR_MAX_STR_LEN];

void aodvv2_client_init(void)
{
    DEBUG("aodvv2_client_init()\n");

    mutex_lock(&_set_mutex);
    memset(&_client_set, 0, sizeof(_client_set));
    mutex_unlock(&_set_mutex);
}

aodvv2_client_entry_t *aodvv2_client_add(const ipv6_addr_t *addr,
                                         const uint8_t prefix_length,
                                         const uint8_t cost)
{
    DEBUG("aodvv2_client_add_client(%s)\n",
          ipv6_addr_to_str(ipbuf, addr, sizeof(ipbuf)));

    aodvv2_client_entry_t *entry = aodvv2_client_find(addr);
    if (entry) {
        /* Update entry if it exists */
        mutex_lock(&_set_mutex);
        entry->prefix_length = prefix_length;
        entry->cost = cost;
        entry->_used = true;
        mutex_unlock(&_set_mutex);
        DEBUG("aodvv2_client_add_client: client is already stored\n");
        return entry;
    }

    /* Find free spot in client set and place the entry there */
    mutex_lock(&_set_mutex);
    for (unsigned i = 0; i < ARRAY_SIZE(_client_set); i++) {
        if (!_client_set[i]._used) {
            memcpy(&_client_set[i].ip_address, addr, sizeof(ipv6_addr_t));
            _client_set[i].prefix_length = prefix_length;
            _client_set[i].cost = cost;
            _client_set[i]._used = true;
            DEBUG("aodvv2_client_add_client: client added\n");
            mutex_unlock(&_set_mutex);
            return &_client_set[i];
        }
    }
    DEBUG("aodvv2_client_add_client: client table is full.\n");
    mutex_unlock(&_set_mutex);

    return NULL;
}

int aodvv2_client_delete(const ipv6_addr_t *addr)
{
    DEBUG("aodvv2_client_delete(%s)\n",
          ipv6_addr_to_str(ipbuf, addr, sizeof(ipbuf)));

    aodvv2_client_entry_t *entry = aodvv2_client_find(addr);
    if (entry) {
        mutex_lock(&_set_mutex);
        memset(&entry->ip_address, 0, sizeof(entry->ip_address));
        entry->_used = false;
        mutex_unlock(&_set_mutex);
        return 0;
    }

    DEBUG("aodvv2_client_delete: client not found\n");

    return -1;
}

aodvv2_client_entry_t *aodvv2_client_find(const ipv6_addr_t *addr)
{
    DEBUG("aodvv2_client_find(%s)\n",
          ipv6_addr_to_str(ipbuf, addr, sizeof(ipbuf)));

    mutex_lock(&_set_mutex);
    for (unsigned i = 0; i < ARRAY_SIZE(_client_set); i++) {
        if (ipv6_addr_equal(&_client_set[i].ip_address, addr)) {
            mutex_unlock(&_set_mutex);
            return &_client_set[i];
        }
    }
    mutex_unlock(&_set_mutex);

    return NULL;
}
