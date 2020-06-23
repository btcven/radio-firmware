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
 * @brief       RFC5444 server implementation
 *
 * @author      Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 * @author      Gustavo Grisales <gustavosinbandera1@hotmail.com>
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 * @}
 */

#include "net/rfc5444.h"

#include "net/gnrc.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/udp.h"

#include "timex.h"
#include "xtimer.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#if ENABLE_DEBUG
#include "rfc5444/rfc5444_print.h"
#endif

typedef struct {
    rfc5444_writer_target_t data;
    bool used;
} target_entry_t;

rfc5444_protocol_t rfc5444_protocol;

static void _send_packet(struct rfc5444_writer *writer, struct rfc5444_writer_target *target, void *ptr, size_t len);
static void _receive(gnrc_pktsnip_t *pkt);
static void message_generation_notifier(struct rfc5444_writer_target *target);
static void _debug_packet(void *data, size_t len);
static void _reset_target_if_stale(target_entry_t *entry);

#if ENABLE_DEBUG == 1
static char _thread_stack[CONFIG_RFC5444_STACK_SIZE + THREAD_EXTRA_STACKSIZE_PRINTF];
#else
static char _thread_stack[CONFIG_RFC5444_STACK_SIZE];
#endif

static kernel_pid_t _thread_pid = KERNEL_PID_UNDEF;

static gnrc_netreg_entry_t netreg = GNRC_NETREG_ENTRY_INIT_PID(GNRC_NETREG_DEMUX_CTX_ALL,
                                                               KERNEL_PID_UNDEF);

static target_entry_t _targets[CONFIG_RFC5444_MAX_TARGETS];

static evtimer_msg_t _rfc5444_evtimer;

#if IS_USED(MODULE_AUTO_INIT_RFC5444)
void rfc5444_auto_init(void)
{
    rfc5444_init();
}
#endif

static void *_thread(void *arg)
{
    (void)arg;
    msg_t msg;
    msg_t reply;
    msg_t msg_queue[CONFIG_RFC5444_MSG_QUEUE_SIZE];

    /* Initialize message queue */
    msg_init_queue(msg_queue, CONFIG_RFC5444_MSG_QUEUE_SIZE);

    reply.content.value = (uint32_t)(-ENOTSUP);
    reply.type = GNRC_NETAPI_MSG_TYPE_ACK;

    while (1) {
        msg_receive(&msg);

        switch (msg.type) {
            case RFC5444_MSG_TYPE_AGGREGATE:
                DEBUG_PUTS("RFC5444_MSG_TYPE_AGGREGATE");
                rfc5444_writer_acquire();
                rfc5444_writer_flush(&rfc5444_protocol.writer,
                                     (struct rfc5444_writer_target *)msg.content.ptr,
                                    false);
                rfc5444_writer_release();
                break;

            case GNRC_NETAPI_MSG_TYPE_RCV:
                DEBUG_PUTS("GNRC_NETAPI_MSG_TYPE_RCV");
                _receive((gnrc_pktsnip_t *)msg.content.ptr);
                break;

            case GNRC_NETAPI_MSG_TYPE_GET:
            case GNRC_NETAPI_MSG_TYPE_SET:
                msg_reply(&msg, &reply);
                break;

            default:
                DEBUG_PUTS("rfc5444: received unidentified message");
                break;
        }
    }

    /* Never reached */
    return NULL;
}

void rfc5444_init(void)
{
    if (_thread_pid == KERNEL_PID_UNDEF) {
        rmutex_init(&rfc5444_protocol.wr_lock);
        rmutex_init(&rfc5444_protocol.rd_lock);

        evtimer_init_msg(&_rfc5444_evtimer);

        /* Initialize RFC5444 reader */
        rmutex_lock(&rfc5444_protocol.rd_lock);
        rfc5444_reader_init(&rfc5444_protocol.reader);
        rmutex_unlock(&rfc5444_protocol.rd_lock);

        /* Initialize RFC5444 writer */
        rmutex_lock(&rfc5444_protocol.wr_lock);
        memset(_targets, 0, sizeof(_targets));

        rfc5444_protocol.writer.msg_buffer = rfc5444_protocol.writer_msg_buffer;
        rfc5444_protocol.writer.msg_size = sizeof(rfc5444_protocol.writer_msg_buffer);
        rfc5444_protocol.writer.addrtlv_buffer = rfc5444_protocol.writer_msg_addrtlvs;
        rfc5444_protocol.writer.addrtlv_size = sizeof(rfc5444_protocol.writer_msg_addrtlvs);

        rfc5444_protocol.writer.message_generation_notifier = message_generation_notifier;

        rfc5444_writer_init(&rfc5444_protocol.writer);
        rmutex_unlock(&rfc5444_protocol.wr_lock);

        _thread_pid = thread_create(_thread_stack, sizeof(_thread_stack),
                                    CONFIG_RFC5444_PRIORITY,
                                    THREAD_CREATE_STACKTEST, _thread, NULL,
                                    "rfc5444");

        gnrc_netreg_entry_init_pid(&netreg, UDP_MANET_PORT, _thread_pid);
        gnrc_netreg_register(GNRC_NETTYPE_UDP, &netreg);
    }
}

rfc5444_writer_target_t *rfc5444_register_target(const ipv6_addr_t *dst,
                                                 uint16_t netif,
                                                 uint32_t lifetime)
{
    assert(dst != NULL);
    assert(netif != 0);
    (void)lifetime;

    rfc5444_writer_acquire();

    /* Find if this target already exists */
    for (unsigned i = 0; i < ARRAY_SIZE(_targets); i++) {
        target_entry_t *entry = &_targets[i];

        _reset_target_if_stale(entry);

        if (entry->used) {
            if (entry->data.netif == netif &&
                ipv6_addr_equal(&entry->data.destination, dst)) {
                timex_t now;
                xtimer_now_timex(&now);

                entry->data.lifetime = timex_add(now, timex_set(lifetime, 0));
                rfc5444_writer_release();
                return &entry->data;
            }
        }
    }

    for (unsigned i = 0; i < ARRAY_SIZE(_targets); i++) {
        target_entry_t *entry = &_targets[i];

        _reset_target_if_stale(entry);

        if (!entry->used) {
            rfc5444_writer_target_t *tgt = &entry->data;

            tgt->target.packet_buffer = tgt->pkt_buffer;
            tgt->target.packet_size = CONFIG_RFC5444_PACKET_SIZE;
            tgt->target.sendPacket = _send_packet;

            tgt->destination = *dst;
            tgt->netif = netif;

            entry->used = true;
            rfc5444_writer_register_target(&rfc5444_protocol.writer,
                                           &tgt->target);
            rfc5444_writer_release();
            return tgt;
        }
    }

    rfc5444_writer_release();
    return NULL;
}

static void _send_packet(struct rfc5444_writer *writer, struct rfc5444_writer_target *target, void *ptr, size_t len)
{
    assert(target != NULL);
    assert(ptr != NULL);
    assert(len != 0);

    (void)writer;

    _debug_packet(ptr, len);

    rfc5444_writer_target_t *tgt = container_of(target, rfc5444_writer_target_t,
                                                target);

    gnrc_pktsnip_t *payload;
    gnrc_pktsnip_t *udp;
    gnrc_pktsnip_t *ip;

    /* Generate our pktsnip with our RFC5444 message */
    payload = gnrc_pktbuf_add(NULL, ptr, len, GNRC_NETTYPE_UNDEF);
    if (payload == NULL) {
        DEBUG_PUTS("rfc5444: couldn't allocate payload");
        return;
    }

    /* Build UDP packet */
    uint16_t port = UDP_MANET_PORT;
    udp = gnrc_udp_hdr_build(payload, port, port);
    if (udp == NULL) {
        DEBUG_PUTS("rfc5444: unable to allocate UDP header");
        gnrc_pktbuf_release(payload);
        return;
    }

    /* Build IPv6 header */
    ip = gnrc_ipv6_hdr_build(udp, NULL, &tgt->destination);
    if (ip == NULL) {
        DEBUG_PUTS("rfc5444: unable to allocate IPv6 header");
        gnrc_pktbuf_release(udp);
        return;
    }

    /* Build netif header */
    gnrc_netif_t *netif = gnrc_netif_get_by_pid(tgt->netif);
    gnrc_pktsnip_t *netif_hdr = gnrc_netif_hdr_build(NULL, 0, NULL, 0);
    gnrc_netif_hdr_set_netif(netif_hdr->data, netif);
    LL_PREPEND(ip, netif_hdr);

    /* Send packet */
    int res = gnrc_netapi_dispatch_send(GNRC_NETTYPE_UDP,
                                        GNRC_NETREG_DEMUX_CTX_ALL, ip);
    if (res < 1) {
        DEBUG_PUTS("rfc5444: unable to locate UDP thread");
        gnrc_pktbuf_release(ip);
        return;
    }
}

static void _receive(gnrc_pktsnip_t *pkt)
{
    assert(pkt != NULL);
    assert(pkt->data != NULL);
    assert(pkt->size > 0);

    _debug_packet(pkt->data, pkt->size);

    ipv6_hdr_t *ipv6_hdr = gnrc_ipv6_get_header(pkt);
    gnrc_pktsnip_t *netif_hdr = gnrc_pktsnip_search_type(pkt,
                                                         GNRC_NETTYPE_NETIF);

    assert(ipv6_hdr != NULL);
    assert(netif_hdr != NULL);

    rfc5444_reader_acquire();
    gnrc_netif_t *iface = gnrc_netif_hdr_get_netif(netif_hdr->data);
    rfc5444_protocol.sender = ipv6_hdr->src;
    rfc5444_protocol.netif = netif_get_id(&iface->netif);

    if (rfc5444_reader_handle_packet(&rfc5444_protocol.reader, pkt->data,
                                     pkt->size) != RFC5444_OKAY) {
        DEBUG_PUTS("rfc5444: couldn't handle packet");
    }
    rfc5444_reader_release();

    gnrc_pktbuf_release(pkt);
}

static void message_generation_notifier(struct rfc5444_writer_target *target)
{
    assert(target != NULL);

    evtimer_msg_event_t event;
    event.event.next = NULL;
    event.event.offset = CONFIG_RFC5444_MSG_AGGREGATION_TIME;
    event.msg.type = RFC5444_MSG_TYPE_AGGREGATE;
    event.msg.content.ptr = target;

    evtimer_add_msg(&_rfc5444_evtimer, &event, _thread_pid);
}

static void _debug_packet(void *data, size_t len)
{
#if ENABLE_DEBUG
    static struct autobuf hexbuf;

    /* Generate hexdump of packet and pretty print */
    abuf_hexdump(&hexbuf, "\t", data, len);
    rfc5444_print_direct(&hexbuf, data, len);

    /* Print hexdump to console */
    DEBUG("%s", abuf_getptr(&hexbuf));

    abuf_free(&hexbuf);
#else
    (void)data;
    (void)len;
#endif
}

static void _reset_target_if_stale(target_entry_t *entry)
{
    timex_t null = timex_set(0, 0);
    if (timex_cmp(entry->data.lifetime, null) == 0) {
        return;
    }

    timex_t now;
    xtimer_now_timex(&now);
    if (timex_cmp(entry->data.lifetime, now) > 0) {
        DEBUG_PUTS("rfc5444: stale target, removing");
        entry->used = false;
    }
}
