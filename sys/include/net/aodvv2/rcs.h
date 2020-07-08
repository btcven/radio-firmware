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
 */

#ifndef AODVV2_RCS_H
#define AODVV2_RCS_H

#include <errno.h>

#include "net/ipv6/addr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Router Client Set entry
 *
 * @see <a href="https://tools.ietf.org/html/draft-perkins-manet-aodvv2-03#section-4.2">
 *          draft-perkins-manet-aodvv2-03, Section 4.2 Router Client Set
 *      </a>
 */
typedef struct {
    /**
     * @brief An IP address or the start of an address range that requires route
     *        discovery services from the AODVv2 router.
     */
    ipv6_addr_t addr;
    /**
     * @brief The length, in bits, of the routing prefix associated with the
     * IP address.
     *
     * If the prefix length is not equal to the address length of IP address,
     * the AODVv2 router MUST participate in route discovery on behalf of all
     * addresses within that prefix.
     */
    uint8_t pfx_len;
    /**
     * @brief The cost associated with reaching the client's address or address
     *        range.
     */
    uint8_t cost;
} aodvv2_router_client_t;

/**
 * @brief   Initialize Router Client Set
 */
void aodvv2_rcs_init(void);

/**
 * @brief   Add a client to the Router Client Set.
 *
 * @pre @p addr != NULL
 *
 * @param[in]  addr    Client address.
 * @param[in]  pfx_len Prefix length.
 * @param[in]  cost    Cost associated with the client address.
 *
 * @return 0 client added.
 * @return -EEXISTS if client exists.
 * @return -ENOSPC if the set is full.
 * @return -EINVAL if the client data is invalid.
 */
int aodvv2_rcs_add(const ipv6_addr_t *addr, uint8_t pfx_len, const uint8_t cost);

/**
 * @brief   Delete a client from the Router Client Set
 *
 * @pre @p addr != NULL
 *
 * @param[in]  addr    IPv6 address of the client to remove.
 * @param[in]  pfx_len `addr` prefix length.
 *
 * @return 0 on success.
 * @return -EINVAL client address/prefix length is invalid.
 * @return -ENOENT client not found.
 */
int aodvv2_rcs_del(const ipv6_addr_t *addr, uint8_t pfx_len);

/**
 * @brief   Find a client in the set.
 *
 * @pre @p addr != NULL
 *
 * @param[out] client  The client return value.
 * @param[in]  addr    The client address to be found.
 * @param[in]  pfx_len `addr` prefix length.
 *
 * @return 0 on success
 * @return -EINVAL parameter errors.
 * @return -ENOENT entry doesn't exists.
 */
int aodvv2_rcs_find(aodvv2_router_client_t *client, const ipv6_addr_t *addr,
                    uint8_t pfx_len);

/**
 * @brief   Verifies if the given IP address matches a client entry.
 *
 * @pre @p addr != NULL
 *
 * @param[out] client The client return value.
 * @param[in]  addr   The IPv6 address.
 *
 * @return 0 if found.
 * @return -ENOENT if not found.
 * @return -EINVAL on invalid address (unspecified).
 */
int aodvv2_rcs_get(aodvv2_router_client_t *client, const ipv6_addr_t *addr);

/**
 * @brief   Print RCS entries.
 */
void aodvv2_rcs_print_entries(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AODVV2_RCS_H */
/** @} */
