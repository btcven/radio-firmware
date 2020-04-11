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

#ifndef AODVV2_CLIENT_H
#define AODVV2_CLIENT_H

#include "net/ipv6/addr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    Configurable number of maximum entries in the Router Client Set
 * @{
 */
#ifndef CONFIG_AODVV2_CLIENT_SET_ENTRIES
#define CONFIG_AODVV2_CLIENT_SET_ENTRIES (2)
#endif
/** @} */

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
    ipv6_addr_t ip_address;
    /**
     * @brief The length, in bits, of the routing prefix associated with the
     * IP address.
     *
     * If the prefix length is not equal to the address length of IP address,
     * the AODVv2 router MUST participate in route discovery on behalf of all
     * addresses within that prefix.
     */
    uint8_t prefix_length;
    /**
     * @brief The cost associated with reaching the client's address or address
     *        range.
     */
    uint8_t cost;
    bool _used; /**< **Not meant to be changed by user!** */
} aodvv2_client_entry_t;

/**
 * @brief   Initialize Router Client Set
 */
void aodvv2_client_init(void);

/**
 * @brief   Add a client to the Router Client Set.
 *
 * @pre @p addr != NULL
 *
 * @param[in] addr          Client IP address.
 * @param[in] prefix_length Length of the routing prefix associated with the
 * address.
 * @param[in] cost          Cost associated with the client.
 *
 * @return NULL The Set is full.
 * @return aodvv2_client_entry_t * Pointer to the entry in the client set.
 */
aodvv2_client_entry_t *aodvv2_client_add(const ipv6_addr_t *addr,
                                         const uint8_t prefix_length,
                                         const uint8_t cost);
/**
 * @brief   Delete a client from the Router Client Set
 *
 * @pre @p addr != NULL
 *
 * @param[in] addr IP address of the client to remove.
 *
 * @return 0  Deletion succeed.
 * @return -1 Client doesn't exist in the set.
 */
int aodvv2_client_delete(const ipv6_addr_t *addr);

/**
 * @brief   Find a client in the set.
 *
 * @pre @p != NULL
 *
 * @param[in] addr The address of the client to be found.
 *
 * @return aodvv2_client_entry_t * Pointer to entry if found.
 * @return NULL Not found.
 */
aodvv2_client_entry_t *aodvv2_client_find(const ipv6_addr_t *addr);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AODVV2_CLIENT_H */
/** @} */
