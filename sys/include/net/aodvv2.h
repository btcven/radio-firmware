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

#include "net/gnrc/netif.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Initialize and start RFC5444
 */
int aodvv2_init(void);

/**
 * @brief   Join this network interface to the AODVv2 routing protocol.
 *
 * @param[in]  netif Network interface.
 *
 * @return 0 on success, otherwise less than 0.
 */
int aodvv2_gnrc_netif_join(gnrc_netif_t *netif);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NET_AODVV2_AODVV2_H */
/** @} */
