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
 * @}
 */

#include "chat.h"

#include "periph/uart.h"
#include "isrpipe.h"

#include "cbor.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#if ENABLE_DEBUG == 1
static char _stack[THREAD_STACKSIZE_DEFAULT + THREAD_EXTRA_STACKSIZE_PRINTF];
#else
static char _stack[THREAD_STACKSIZE_DEFAULT];
#endif

static uint8_t _isrpipe_buf_mem[CONFIG_CHAT_RX_BUF_SIZE];
isrpipe_t chat_serial_isrpipe = ISRPIPE_INIT(_isrpipe_buf_mem);

chat_id_t chat_id_unspecified = {{ 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00 }};

static void _serial_init(void)
{
    uart_rx_cb_t cb;
    cb = (uart_rx_cb_t) isrpipe_write_one;
    void *arg = &chat_serial_isrpipe;

    uart_init(CONFIG_CHAT_UART_DEV, CONFIG_CHAT_BAUDRATE, cb, arg);
}

static ssize_t _serial_read(void *buffer, size_t count)
{
    return (ssize_t)isrpipe_read(&chat_serial_isrpipe, (uint8_t *)buffer,
                                 count);
}

/*
static ssize_t _serial_write(const void *buffer, size_t len)
{
    uart_write(CONFIG_CHAT_UART_DEV, (const uint8_t *)buffer, len);
    return (ssize_t)len;
}
*/

static void *_serial_read_loop(void *arg)
{
    (void)arg;

    uint8_t chat_buf[256];
    chat_msg_t chat_msg;
    while (1) {
        /* Read length */
        uint8_t len = 0;
        _serial_read(&len, sizeof(len));

        DEBUG("chat: len = %d", len);

        /* Rest of the payload */
        _serial_read(chat_buf, (size_t)len);

        /* Parse message */
        if (chat_parse_msg(&chat_msg, chat_buf, (size_t)len) < 0) {
            DEBUG("chat: invalid message!\n");
        }

        if (memcmp(&chat_msg.to_uid, &chat_id_unspecified, sizeof(chat_id_t)) == 0) {
            DEBUG("chat: to a public channel, sending as multicast\n");
        }

        /* Reset buffer & message */
        memset(chat_buf, 0, sizeof(chat_buf));
        memset(&chat_msg, 0, sizeof(chat_msg));
    }

    /* Never reached */
    return NULL;
}

int chat_init(void)
{
    _serial_init();

    thread_create(_stack, sizeof(_stack), THREAD_PRIORITY_MAIN + 1, 0,
                  _serial_read_loop, NULL, "chat_serial_read");

    return 0;
}
