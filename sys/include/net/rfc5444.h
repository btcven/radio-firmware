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

#ifndef NET_RFC5444_H
#define NET_RFC5444_H

#include "rmutex.h"
#include "evtimer_msg.h"
#include "timex.h"

#include "net/manet.h"

#include "common/netaddr.h"
#include "rfc5444/rfc5444_reader.h"
#include "rfc5444/rfc5444_writer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   RFC5444 thread stack size
 */
#ifndef CONFIG_RFC5444_STACK_SIZE
#define CONFIG_RFC5444_STACK_SIZE (THREAD_STACKSIZE_MAIN)
#endif

/**
 * @brief   RFC5444 thread priority
 */
#ifndef CONFIG_RFC5444_PRIORITY
#define CONFIG_RFC5444_PRIORITY (THREAD_PRIORITY_MAIN - 2)
#endif

/**
 * @brief   Message queue size for RFC 5444 thread
 */
#ifndef CONFIG_RFC5444_MSG_QUEUE_SIZE
#define CONFIG_RFC5444_MSG_QUEUE_SIZE (16)
#endif

#ifndef CONFIG_RFC5444_PACKET_SIZE
#define CONFIG_RFC5444_PACKET_SIZE (1024)
#endif

/**
 * @brief   RFC5444 address TLVs buffer size
 */
#ifndef CONFIG_RFC5444_ADDR_TLVS_SIZE
#define CONFIG_RFC5444_ADDR_TLVS_SIZE (1024)
#endif

/**
 * @brief   Maximum number of targets for RFC 5444
 */
#ifndef CONFIG_RFC5444_MAX_TARGETS
#define CONFIG_RFC5444_MAX_TARGETS (16)
#endif

#ifndef CONFIG_RFC5444_MSG_AGGREGATION_TIME
#define CONFIG_RFC5444_MSG_AGGREGATION_TIME (100)
#endif

#define RFC5444_MSG_TYPE_AGGREGATE (0x9340)

typedef struct {
    struct rfc5444_writer_target target; /**< RFC5444 writer target */
    uint8_t pkt_buffer[CONFIG_RFC5444_PACKET_SIZE];
    ipv6_addr_t destination;             /**< Address where the packet will be sent */
    uint16_t netif;                      /**< Network interface to send the packet */
    timex_t lifetime;                    /**< Lifetime of this target */
} rfc5444_writer_target_t;

typedef struct {
    rmutex_t rd_lock;
    rmutex_t wr_lock;
    struct rfc5444_reader reader;
    struct rfc5444_writer writer;
    uint8_t writer_msg_buffer[CONFIG_RFC5444_PACKET_SIZE];
    uint8_t writer_msg_addrtlvs[CONFIG_RFC5444_ADDR_TLVS_SIZE];
    uint8_t writer_pkt_buffer[CONFIG_RFC5444_PACKET_SIZE];
    ipv6_addr_t sender;
    uint16_t netif;
} rfc5444_protocol_t;

extern rfc5444_protocol_t rfc5444_protocol;

#if IS_USED(MODULE_AUTO_INIT_RFC5444) || defined(DOXYGEN)
/**
 * @brief   Auto-initialize RFC 5444 protocol
 */
void rfc5444_auto_init(void);
#endif

/**
 * @brief   Initialize RFC 5444 protocol
 */
void rfc5444_init(void);

/**
 * @brief   Register a target
 *
 * @param[in] dst      Destination address,
 * @param[in] netif    Network interface ID.
 * @param[in] lifetime Duration of this target in seconds.
 *
 * @return Pointer to RFC 5444 target on success, otherwise NULL on failure.
 */
rfc5444_writer_target_t *rfc5444_register_target(const ipv6_addr_t *dst,
                                                 uint16_t netif,
                                                 uint32_t lifetime);

/**
 * @brief   Acquire the RFC 5444 reader
 */
static inline void rfc5444_reader_acquire(void)
{
    rmutex_lock(&rfc5444_protocol.rd_lock);
}

/**
 * @brief   Release the RFC 5444 reader
 */
static inline void rfc5444_reader_release(void)
{
    rmutex_unlock(&rfc5444_protocol.rd_lock);
}

/**
 * @brief   Acquire the RFC 5444 writer
 */
static inline void rfc5444_writer_acquire(void)
{
    rmutex_lock(&rfc5444_protocol.wr_lock);
}

/**
 * @brief   Release the RFC 5444 writer
 */
static inline void rfc5444_writer_release(void)
{
    rmutex_unlock(&rfc5444_protocol.wr_lock);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NET_RFC5444_H */
/** @} */
