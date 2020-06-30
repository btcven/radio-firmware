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
 * @brief       RFC5444
 *
 * @author      Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 * @author      Gustavo Grisales <gustavosinbandera1@hotmail.com>
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 */

#ifndef NET_AODVV2_RFC5444_H
#define NET_AODVV2_RFC5444_H

#include "net/aodvv2/msg.h"
#include "net/manet.h"

#include "common/netaddr.h"
#include "rfc5444/rfc5444_reader.h"
#include "rfc5444/rfc5444_writer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    RFC5444 thread stack size
 */
#ifndef CONFIG_AODVV2_RFC5444_STACK_SIZE
#define CONFIG_AODVV2_RFC5444_STACK_SIZE     (2048)
#endif

/**
 * @name    RFC5444 thread priority
 */
#ifndef CONFIG_AODVV2_RFC5444_PRIO
#define CONFIG_AODVV2_RFC5444_PRIO           (6)
#endif

/**
 * @name    RFC5444 message queue size
 */
#ifndef CONFIG_AODVV2_RFC5444_MSG_QUEUE_SIZE
#define CONFIG_AODVV2_RFC5444_MSG_QUEUE_SIZE (32)
#endif

/**
 * @name    RFC5444 maximum packet size
 */
#ifndef CONFIG_AODVV2_RFC5444_PACKET_SIZE
#define CONFIG_AODVV2_RFC5444_PACKET_SIZE    (128)
#endif

/**
 * @name    RFC5444 address TLVs buffer size
 */
#ifndef CONFIG_AODVV2_RFC5444_ADDR_TLVS_SIZE
#define CONFIG_AODVV2_RFC5444_ADDR_TLVS_SIZE (1000)
#endif

typedef struct {
    struct rfc5444_writer_target target; /**< RFC5444 writer target */
    ipv6_addr_t target_addr;             /**< Address where the packet will be sent */
} aodvv2_writer_target_t;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NET_AODVV2_RFC5444_H */
/** @} */
