/*
 * Copyright (C) 2020 Locha Inc
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     chat
 * @{
 *
 * @file
 * @brief       Chat serial communication
 *
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 */

#ifndef CHAT_H
#define CHAT_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "net/gnrc/ipv6.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_CHAT_RX_BUF_SIZE
/**
 * @brief   RX buffer size for the chat serial
 */
#define CONFIG_CHAT_RX_BUF_SIZE (256)
#endif

#ifndef CONFIG_CHAT_UART_DEV
/**
 * @brief   UART device to use
 */
#define CONFIG_CHAT_UART_DEV 1
#endif

#ifndef CONFIG_CHAT_BAUDRATE
/**
 * @brief   UART baudrate
 */
#define CONFIG_CHAT_BAUDRATE (115200U)
#endif

#ifndef CONFIG_CHAT_UDP_PORT
/**
 * @breif   Chat UDP port
 */
#define CONFIG_CHAT_UDP_PORT (8080)
#endif

/**
 * @brief   Chat ID
 */
typedef union {
    uint8_t u8[32];
} chat_id_t;

/**
 * @brief   Chat message content
 */
typedef struct {
    uint8_t buf[128]; /**< Buffer with contents */
    size_t len; /**< Bytes used in buf */
} chat_msg_content_t;

/**
 * @brief   Chat message
 */
typedef struct {
    chat_id_t from_uid; /**< Where the message comes from */
    chat_id_t to_uid; /**< Where the message is directed to */
    chat_id_t msg_id; /**< Message ID */
    chat_msg_content_t msg; /**< Message content */
    uint64_t timestamp; /**< Message timestamp */
    uint64_t type; /**< Message type */
} chat_msg_t;

/**
 * @brief   Unspecified chat ID.
 */
extern chat_id_t chat_id_unspecified;

/**
 * @brief   Is ID unspecified?
 *
 * @return true it's unspecified.
 * @return false otherwise.
 */
static inline bool chat_id_is_unspecified(chat_id_t *id)
{
    return memcmp(&chat_id_unspecified, id, sizeof(chat_id_t)) == 0;
}

/**
 * @brief   Convert ID.
 */
static inline void chat_id_to_ipv6(ipv6_addr_t *addr, chat_id_t *id)
{
    assert(addr != NULL && id != NULL);

    /* Set global prefix */
    addr->u8[0] = 0x20;
    addr->u8[1] = 0x01;

    /* Take the first 14-bytes of the ID */
    memcpy(&addr->u8[2], id, (sizeof(chat_id_t) - sizeof(ipv6_addr_t) - 2));
}

/**
 * @brief   Initialize chat service
 *
 * @pre @p netif != NULL
 *
 * @param[in] netif The network interface to use.
 */
int chat_init(gnrc_netif_t *netif);

/**
 * @brief   Parse a chat message encoded in CBOR.
 *
 * @param[out] msg    Result.
 * @param[in]  buffer Input buffer.
 * @param[in]  len    Length of buffer.
 *
 * @return Negative value on failure.
 * @return 0 Otherwise.
 */
int chat_parse_msg(chat_msg_t *msg, uint8_t *buffer, size_t len);

/**
 * @brief   Encode a message
 *
 * @param[in]  msg    Message to encode.
 * @param[out] buffer Output buffer.
 * @param[in]  len    Length of output buffer.
 *
 * @return The number of bytes written into the buffer.
 */
size_t chat_encode_msg(chat_msg_t *msg, uint8_t *buffer, size_t len);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CHAT_H */
/** @} */
