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
 * @brief       Packet buffering
 *
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 */

#ifndef PRIV_AODVV2_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

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

#endif /* PRIV_AODVV2_BUFFER_H */
/** @} */
