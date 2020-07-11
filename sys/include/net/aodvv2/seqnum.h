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
 * @brief       AODVv2 Node Sequence Number maintenance
 *
 * @author      Locha Mesh Developers <contact@locha.io>
 */

#ifndef NET_AODVV2_SEQNUM_H
#define NET_AODVV2_SEQNUM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Sequence Number
 *
 * Sequence Numbers are used to determine temporal order of AODVv2 messages that
 * originate from AODVv2 routers. Used to identify routing stale information so
 * that it can be discarded.
 *
 * @see [draft-perkins-manet-aodvv2-03, section 4.4]
 *      (https://tools.ietf.org/html/draft-perkins-manet-aodvv2-03#section-4.4)
 */
typedef uint16_t aodvv2_seqnum_t;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NET_AODVV2_SEQNUM_H */
