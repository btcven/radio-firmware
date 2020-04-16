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

#include "net/gnrc/udp.h"
#include "net/gnrc/netif/hdr.h"

#include "periph/uart.h"
#include "isrpipe.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#if ENABLE_DEBUG == 1
static char _serial_stack[THREAD_STACKSIZE_DEFAULT + THREAD_EXTRA_STACKSIZE_PRINTF];
static char _udp_stack[THREAD_STACKSIZE_DEFAULT + THREAD_EXTRA_STACKSIZE_PRINTF];
#else
static char _serial_stack[THREAD_STACKSIZE_DEFAULT];
static char _udp_stack[THREAD_STACKSIZE_DEFAULT];
#endif

/**
 * @brief   Netif
 */
static gnrc_netif_t *_netif;

/**
 * @brief   Netreg
 */
static gnrc_netreg_entry_t netreg = GNRC_NETREG_ENTRY_INIT_PID(GNRC_NETREG_DEMUX_CTX_ALL,
                                                               KERNEL_PID_UNDEF);

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

static ssize_t _serial_write(const void *buffer, size_t len)
{
    uart_write(CONFIG_CHAT_UART_DEV, (const uint8_t *)buffer, len);
    return (ssize_t)len;
}

void chat_send_msg(chat_msg_t *msg)
{
    DEBUG("chat: sending message\n");

    ipv6_addr_t target_addr;
    if (chat_id_is_unspecified(&msg->to_uid)) {
        DEBUG("chat: sending message as multicast\n");
        /* Send as multicast to neighbor nodes */
        target_addr = ipv6_addr_all_nodes_link_local;
    }
    else {
        DEBUG("chat: sending message as unicast\n");
        /* Send a unicast message to our node with the generated global IPv6
         * address */
        chat_id_to_ipv6(&target_addr, &msg->to_uid);
    }

    uint8_t buffer[256];
    size_t length = chat_encode_msg(msg, buffer, sizeof(buffer));

    gnrc_pktsnip_t *payload;
    gnrc_pktsnip_t *udp;
    gnrc_pktsnip_t *ip;

    /* Generate our pktsnip with our RFC 5444 message */
    payload = gnrc_pktbuf_add(NULL, buffer, length, GNRC_NETTYPE_UNDEF);
    if (payload == NULL) {
        DEBUG("chat: couldn't allocate payload\n");
        return;
    }

    /* Build UDP packet */
    uint16_t port = CONFIG_CHAT_UDP_PORT;
    udp = gnrc_udp_hdr_build(payload, port, port);
    if (udp == NULL) {
        DEBUG("chat: unable to allocate UDP header\n");
        gnrc_pktbuf_release(payload);
        return;
    }

    /* Build IPv6 header */
    ip = gnrc_ipv6_hdr_build(udp, NULL, &target_addr);
    if (ip == NULL) {
        DEBUG("chat: unable to allocate IPv6 header\n");
        gnrc_pktbuf_release(udp);
        return;
    }

    /* Build netif header */
    gnrc_pktsnip_t *netif_hdr = gnrc_netif_hdr_build(NULL, 0, NULL, 0);
    gnrc_netif_hdr_set_netif(netif_hdr->data, _netif);
    LL_PREPEND(ip, netif_hdr);

    /* Send packet */
    int res = gnrc_netapi_dispatch_send(GNRC_NETTYPE_UDP,
                                        GNRC_NETREG_DEMUX_CTX_ALL, ip);
    if (res < 1) {
        DEBUG("aodvv2: unable to locate UDP thread\n");
        gnrc_pktbuf_release(ip);
        return;
    }
}

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

        chat_send_msg(&chat_msg);

        /* Reset buffer & message */
        memset(chat_buf, 0, sizeof(chat_buf));
        memset(&chat_msg, 0, sizeof(chat_msg));
    }

    /* Never reached */
    return NULL;
}

static void *_udp_event_loop(void *arg)
{
    (void)arg;
    msg_t msg;
    msg_t reply;
    msg_t msg_queue[32];

    /* Initialize message queue */
    msg_init_queue(msg_queue, 32);

    reply.content.value = (uint32_t)(-ENOTSUP);
    reply.type = GNRC_NETAPI_MSG_TYPE_ACK;

    while (1) {
        msg_receive(&msg);

        switch (msg.type) {
            case GNRC_NETAPI_MSG_TYPE_RCV:
                DEBUG("GNRC_NETAPI_MSG_TYPE_RCV\n");
                {
                    /* Parse incoming message */
                    gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t *)msg.content.ptr;
                    chat_msg_t msg;
                    if (chat_parse_msg(&msg, pkt->data, pkt->size) == 0) {
                        /* TODO: verify it's for us */

                        /* Encode message again */
                        uint8_t buffer[257];
                        size_t length = chat_encode_msg(&msg, buffer + 1,
                                                        sizeof(buffer) - 1);
                        assert(length <= 256);
                        buffer[0] = (uint8_t)length;

                        /* Send over serial */
                        _serial_write(buffer, length + 1);
                    }
                }
                break;

            case GNRC_NETAPI_MSG_TYPE_GET:
            case GNRC_NETAPI_MSG_TYPE_SET:
                msg_reply(&msg, &reply);
                break;

            default:
                DEBUG("chat: received unidentified message\n");
                break;
        }
    }
}

int chat_init(gnrc_netif_t *netif)
{
    assert(netif != NULL);

    _netif = netif;
    _serial_init();

    /* Initialize serial */
    thread_create(_serial_stack, sizeof(_serial_stack),
                  THREAD_PRIORITY_MAIN + 2, THREAD_CREATE_STACKTEST,
                  _serial_read_loop, NULL, "chat_serial_read");

    kernel_pid_t udp_pid = thread_create(_udp_stack, sizeof(_udp_stack),
                                         THREAD_PRIORITY_MAIN + 1,
                                         THREAD_CREATE_STACKTEST,
                                         _udp_event_loop, NULL, "chat_udp");
    if (udp_pid < 0) {
        DEBUG("chat: couldn't create chat_udp\n");
        return -1;
    }

    /* Register netreg */
    gnrc_netreg_entry_init_pid(&netreg, CONFIG_CHAT_UDP_PORT, udp_pid);
    gnrc_netreg_register(GNRC_NETTYPE_UDP, &netreg);

    return 0;
}
