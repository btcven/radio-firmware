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
 * @ingroup     net
 * @{
 *
 * @file
 * @brief       RFC 5444 server demultiplexer
 *
 * This is an implementation of the RFC 5444 protocol on top of the GNRC
 * network stack.
 *
 * @see [RFC 5444]
 *      (https://tools.ietf.org/html/rfc5444)
 *
 * @author      Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 * @author      Gustavo Grisales <gustavosinbandera1@hotmail.com>
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 */

#ifndef NET_RFC5444_H
#define NET_RFC5444_H

#include "net/ipv6/addr.h"
#include "net/gnrc/pktbuf.h"

#include "common/netaddr.h"
#include "rfc5444/rfc5444_writer.h"
#include "rfc5444/rfc5444_reader.h"
#include "rfc5444/rfc5444_iana.h"

#include "thread.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   RFC 5444 thread stack size
 */
#ifndef CONFIG_RFC5444_STACK_SZIE
#define CONFIG_RFC5444_STACK_SIZE (THREAD_STACKSIZE_DEFAULT)
#endif

/**
 * @brief   RFC 5444 thread priority
 */
#ifndef CONFIG_RFC5444_PRIO
#define CONFIG_RFC5444_PRIO (THREAD_PRIORITY_MAIN - 1)
#endif

/**
 * @brief   RFC 5444 thread message queue size
 */
#ifndef CONFIG_RFC5444_MSG_QUEUE_SIZE
#define CONFIG_RFC5444_MSG_QUEUE_SIZE (16)
#endif

/**
 * @brief   Maximum message size
 */
#ifndef CONFIG_RFC5444_MSG_SIZE
#define CONFIG_RFC5444_MSG_SIZE (64)
#endif

/**
 * @brief   Maximum packet size
 */
#ifndef CONFIG_RFC5444_PACKET_SIZE
#define CONFIG_RFC5444_PACKET_SIZE (128)
#endif

/**
 * @brief   Address/TLVs buffer size
 */
#ifndef CONFIG_RFC5444_ADDR_TLVS_SIZE
#define CONFIG_RFC5444_ADDR_TLVS_SIZE (1024)
#endif

/**
 * @brief   Maximum available write targets
 */
#ifndef CONFIG_RFC5444_TARGET_NUMOF
#define CONFIG_RFC5444_TARGET_NUMOF (16)
#endif

/**
 * @brief   Message aggregationt time
 *
 * This is the time of aggregation after an RFC 5444 message has been created,
 * if other messages for the same target are created within the aggregation,
 * they will be sent in the same packet.
 *
 * Increasing this values increases the possibility of more messages fitting in
 * one packet, although will slow down the time it takes to send a single
 * packet.
 */
#ifndef CONFIG_RFC5444_AGGREGATION_TIME
#define CONFIG_RFC5444_AGGREGATION_TIME (100)
#endif

/**
 * @{
 * @name IPC message types.
 */
#define GNRC_RFC5444_MSG_TYPE_AGGREGATE (0x9120) /**< RFC 5444 message aggregation */
/** @} */

/**
 * @brief   Packet data.
 */
typedef struct {
    ipv6_addr_t src; /**< Source IPv6 address */
    uint16_t iface; /**< Network interface where this packet was received */
    gnrc_pktsnip_t *pkt; /**< Pointer to packet */
} gnrc_rfc5444_packet_data_t;

/**
 * @brief   Initialize GNRC RFC 5444 server/demultiplexer.
 *
 * @return 0 on success.
 * @return less than 0 on failure.
 */
int gnrc_rfc5444_init(void);

/**
 * @brief   Acquire lock on the RFC 5444 reader.
 *
 * @note All read/writes when using the RFC 5444 reader MUST be locked.
 *
 * @note Holding a lock on the RFC 5444 reader prevents other threads for
 *       registering message types. It also stops the main thread from
 *       processing received packets.
 *
 * @see @ref gnrc_rfc5444_reader to get a pointer to the writer.
 * @see @ref gnrc_rfc5444_reader_release to release the lock.
 */
void gnrc_rfc5444_reader_acquire(void);

/**
 * @brief   Release lock on the RFC 5444 reader.
 */
void gnrc_rfc5444_reader_release(void);

/**
 * @brief   Acquire lock on the RFC 5444 writer.
 *
 * @note All read/writes when using the RFC 5444 writer must be locked.
 *
 * @note Holding a lock on the writer blocks other threads that may want to
 *       write messages.
 *
 * @see @ref gnrc_rfc5444_writer to get a pointer to the writer.
 * @see @ref gnrc_rfc5444_writer_release to release the lock.
 */
void gnrc_rfc5444_writer_acquire(void);

/**
 * @brief   Release the lock on the RFC 5444 writer.
 */
void gnrc_rfc5444_writer_release(void);

/**
 * @brief   The RFC 5444 reader pointer
 *
 * @note You _MUST_ first [acquire the reader lock](@ref gnrc_rfc5444_reader_acquire).
 */
struct rfc5444_reader *gnrc_rfc5444_reader(void);

/**
 * @brief   The RFC 5444 writer pointer.
 *
 * @note You _MUST_ first [acquire the writer lock](@ref gnrc_rfc5444_writer_acquire).
 */
struct rfc5444_writer *gnrc_rfc5444_writer(void);

/**
 * @brief   Add a RFC 5444 writer target
 *
 * @note It's not necessary to unlock/lock the writer (it's locked internally).
 *
 * @see @ref gnrc_rfc5444_del_writer_target.
 *
 * @param[in]  dst   IPv6 link local destination address.
 * @param[in]  iface Network interface.
 *
 * @return 0 if succeed.
 * @return -ENOMEM on failed allocation of the target.
 */
int gnrc_rfc5444_add_writer_target(const ipv6_addr_t *dst, uint16_t iface);

/**
 * @brief   Delete a RFC 5444 writer target
 *
 * @note It's not necessary to unlock/lock the writer (it's locked internally).
 *
 * @see @ref gnrc_rfc5444_add_writer_target
 *
 * @param[in]  dst   IPv6 link local destination address.
 * @param[in]  iface Network interface.
 */
void gnrc_rfc5444_del_writer_target(const ipv6_addr_t *dst, uint16_t iface);

/**
 * @brief   Get a RFC 5444 target pointer.
 *
 * @note The target MUST have been previously
 *       [added](@ref gnrc_rfc5444_add_writer_target).
 *
 * @param[in]  dst   IPv6 link local destination address.
 * @param[in]  iface Network interface.
 *
 * @return Pointer to target if found.
 * @return NULL if target doesn't exists.
 */
struct rfc5444_writer_target *gnrc_rfc5444_get_writer_target(const ipv6_addr_t *dst, uint16_t iface);

/**
 * @brief   Get packet data
 *
 * Returns a pointer to the packet data, which contains information such as
 * the source IPv6 address and the network interface where it was received.
 *
 * @note This MUST be only called inside RFC 5444 reader callbacks. Usage in
 * other contexts is prone to undefined behaviour and it might ate your dog!
 *
 * @return Pointer to packet data.
 */
const gnrc_rfc5444_packet_data_t *gnrc_rfc5444_get_packet_data(void);

/**
 * @brief   `ipv6_addr_t` to `struct netaddr`.
 *
 * @pre (@p src != NULL) && (@p dst != NULL)
 *
 * @param[in]  src     Source.
 * @param[in]  pfx_len Prefix length.
 * @param[out] dst     Destination.
 */
void ipv6_addr_to_netaddr(const ipv6_addr_t *src, uint8_t pfx_len, struct netaddr *dst);

/**
 * @brief   `struct netaddr` to `ipv6_addr_t`.
 *
 * @pre (@p src != NULL) && (@p dst != NULL)
 *
 * @param[in]  src     Source.
 * @param[out] dst     Destination.
 * @param[out] pfx_len Prefix length.
 */
void netaddr_to_ipv6_addr(struct netaddr *src, ipv6_addr_t *dst, uint8_t *pfx_len);

#ifdef __cpluslpus
} /* extern "C" */
#endif

#endif /* NET_RFC5444_H */
