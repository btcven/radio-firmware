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
 * @brief       AODVV2
 *
 * @author      Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 * @author      Gustavo Grisales <gustavosinbandera1@hotmail.com>
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 */

#ifndef NET_AODVV2_AODVV2_H
#define NET_AODVV2_AODVV2_H

#include "net/aodvv2/conf.h"
#include "net/aodvv2/msg.h"
#include "net/ipv6/addr.h"
#include "net/gnrc.h"

#ifdef __cplusplus
extern "C" {
#endif

#if IS_USED(MODULE_AUTO_INIT_AODVV2) || defined(DOXYGEN)
/**
 * @brief   Auto-initialize AODVv2
 */
void aodvv2_auto_init(void);
#endif

/**
 * @brief   Initialize AODVv2
 */
void aodvv2_init(void);

/**
 * @brief   Register a network interface as an AODVv2 router
 *
 * @pre @p netif != NULL
 *
 * @param[in]  netif The network interface to register.
 */
void aodvv2_netif_register(gnrc_netif_t *netif);

/**
 * @brief   Initialize the AODVv2 packer buffering code.
 */
void aodvv2_buffer_init(void);

/**
 * @brief   Add a packet to the packet buffer
 *
 * @pre @p dst != NULL && @p pkt != NULL
 *
 * @brief[in] dst Packet destination address.
 * @brief[in] pkt Packet.
 */
int aodvv2_buffer_pkt_add(const ipv6_addr_t *dst, gnrc_pktsnip_t *pkt);

/**
 * @brief   Dispatch buffered packets to `targ_addr`
 *
 * @notes Only call this when a route to `targ_addr` is on the NIB
 *
 * @param[in] targ_addr Target address to dispatch packets.
 */
void aodvv2_buffer_dispatch(const ipv6_addr_t *targ_addr);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NET_AODVV2_AODVV2_H */
/** @} */
