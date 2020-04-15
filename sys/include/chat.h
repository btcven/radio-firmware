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

#include "shell.h"

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
 * @brief   Initialize chat service
 */
int chat_init(void);

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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CHAT_H */
/** @} */
