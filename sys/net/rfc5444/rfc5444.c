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

#include <assert.h>

#include "net/rfc5444.h"
#include "net/manet.h"

#include "net/gnrc/ipv6.h"
#include "net/gnrc/udp.h"
#include "net/gnrc/netif/hdr.h"

#include "evtimer_msg.h"
#include "rmutex.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

#if ENABLE_DEBUG
#include "rfc5444/rfc5444_print.h"
#endif

typedef struct {
    struct rfc5444_writer_target target;
    uint8_t pkt_buffer[CONFIG_RFC5444_PACKET_SIZE];
    ipv6_addr_t dst;
    uint16_t iface;
    evtimer_msg_event_t aggregation_timeout;
    bool used;
} _gnrc_rfc5444_target_t;

static kernel_pid_t _thread_pid = KERNEL_PID_UNDEF;

#if ENABLE_DEBUG
static char _thread_stack[CONFIG_RFC5444_STACK_SIZE + THREAD_EXTRA_STACKSIZE_PRINTF];
#else
static char _thread_stack[CONFIG_RFC5444_STACK_SIZE];
#endif

static struct rfc5444_reader _reader;
static struct rfc5444_writer _writer;
static rmutex_t _writer_lock = RMUTEX_INIT;
static rmutex_t _reader_lock = RMUTEX_INIT;

static uint8_t _writer_msg_buffer[CONFIG_RFC5444_MSG_SIZE];
static uint8_t _writer_msg_addrtlvs[CONFIG_RFC5444_ADDR_TLVS_SIZE];

static gnrc_rfc5444_packet_data_t _packet_data;

static _gnrc_rfc5444_target_t _target[CONFIG_RFC5444_TARGET_NUMOF];

static gnrc_netreg_entry_t netreg = GNRC_NETREG_ENTRY_INIT_PID(GNRC_NETREG_DEMUX_CTX_ALL,
                                                               KERNEL_PID_UNDEF);

static evtimer_msg_t _gnrc_rfc5444_evtimer;

static char addr_str[IPV6_ADDR_MAX_STR_LEN];

static void _message_generation_notifier(struct rfc5444_writer_target *target);
static void *_thread(void *arg);
static void _aggregate(struct rfc5444_writer_target *iface);
static void _receive(gnrc_pktsnip_t *pkt);
static void _send_packet(struct rfc5444_writer *writer,
                         struct rfc5444_writer_target *iface, void *buffer,
                         size_t length);
static void _debug_packet(void *data, size_t size);

int gnrc_rfc5444_init(void)
{
    if (_thread_pid > 0) {
        DEBUG("gncr_rfc5444: trying to reinitialize\n");
        return 0;
    }

    memset(&_target, 0, sizeof(_target));

    evtimer_init_msg(&_gnrc_rfc5444_evtimer);

    rfc5444_reader_init(&_reader);

    _writer.msg_buffer = _writer_msg_buffer;
    _writer.msg_size = sizeof(_writer_msg_buffer);
    _writer.addrtlv_buffer = _writer_msg_addrtlvs;
    _writer.addrtlv_size = sizeof(_writer_msg_addrtlvs);
    _writer.message_generation_notifier = _message_generation_notifier;
    rfc5444_writer_init(&_writer);

    _thread_pid = thread_create(_thread_stack, sizeof(_thread_stack),
                                CONFIG_RFC5444_PRIO, THREAD_CREATE_STACKTEST,
                                _thread, NULL, "rfc5444");
    if (_thread_pid < 0) {
        DEBUG("gnrc_rfc5444: failed to create thread (errno = %d)\n",
              _thread_pid);
        return _thread_pid;
    }

    gnrc_netreg_entry_init_pid(&netreg, UDP_MANET_PORT, _thread_pid);
    gnrc_netreg_register(GNRC_NETTYPE_UDP, &netreg);

    return 0;
}

void gnrc_rfc5444_reader_acquire(void)
{
    rmutex_lock(&_reader_lock);
}

void gnrc_rfc5444_reader_release(void)
{
    rmutex_unlock(&_reader_lock);
}

void gnrc_rfc5444_writer_acquire(void)
{
    rmutex_lock(&_writer_lock);
}

void gnrc_rfc5444_writer_release(void)
{
    rmutex_unlock(&_writer_lock);
}

struct rfc5444_reader *gnrc_rfc5444_reader(void)
{
    return &_reader;
}

struct rfc5444_writer *gnrc_rfc5444_writer(void)
{
    return &_writer;
}

static inline bool _addr_equals(const ipv6_addr_t *addr,
                                const _gnrc_rfc5444_target_t *tmp)
{
    return (addr == NULL) || ipv6_addr_is_unspecified(&tmp->dst) ||
           ipv6_addr_equal(addr, &tmp->dst);
}

int gnrc_rfc5444_add_writer_target(const ipv6_addr_t *dst, uint16_t iface)
{
    DEBUG("gnrc_rfc5444: allocating target (dst = %s, iface = %d)\n",
          (dst == NULL) ? "NULL" : ipv6_addr_to_str(addr_str, dst,
                                                    sizeof(addr_str)), iface);

    gnrc_rfc5444_reader_acquire();
    _gnrc_rfc5444_target_t *target = NULL;
    for (unsigned i = 0; i < ARRAY_SIZE(_target); i++) {
        _gnrc_rfc5444_target_t *tmp = &_target[i];
        if (tmp->iface == iface && _addr_equals(dst, tmp)) {
            target = tmp;
            break;
        }
        if (target == NULL && !tmp->used) {
            target = tmp;
        }
    }

    if (target == NULL) {
        DEBUG("  couldn't allocate RFC 544 target\n");
        gnrc_rfc5444_reader_release();
        return -ENOMEM;
    }

    if (!target->used) {
        memset(target->pkt_buffer, 0, CONFIG_RFC5444_PACKET_SIZE);

        target->target.packet_buffer = target->pkt_buffer;
        target->target.packet_size = CONFIG_RFC5444_PACKET_SIZE;
        target->target.sendPacket = _send_packet;

        rfc5444_writer_register_target(&_writer, &target->target);

        memcpy(&target->dst, dst, sizeof(ipv6_addr_t));
        target->iface = iface;

        target->used = true;
    }

    gnrc_rfc5444_reader_release();
    return 0;
}

void gnrc_rfc5444_del_writer_target(const ipv6_addr_t *dst, uint16_t iface)
{
    DEBUG("gnrc_rfc5444: deleting target (dst = %s, iface = %d)\n",
          (dst == NULL) ? "NULL" : ipv6_addr_to_str(addr_str, dst,
                                                    sizeof(addr_str)), iface);

    gnrc_rfc5444_writer_acquire();
    for (unsigned i = 0; i < ARRAY_SIZE(_target); i++) {
        _gnrc_rfc5444_target_t *tmp = &_target[i];
        if (tmp->iface == iface && _addr_equals(dst, tmp) && tmp->used) {
            rfc5444_writer_unregister_target(&_writer, &tmp->target);
            tmp->used = false;
            break;
        }
    }
    gnrc_rfc5444_writer_release();
}

struct rfc5444_writer_target *gnrc_rfc5444_get_writer_target(const ipv6_addr_t *dst, uint16_t iface)
{
    DEBUG("gnrc_rfc5444: searching target (dst = %s, iface = %d)\n",
          (dst == NULL) ? "NULL" : ipv6_addr_to_str(addr_str, dst,
                                                    sizeof(addr_str)), iface);

    gnrc_rfc5444_writer_acquire();
    _gnrc_rfc5444_target_t *target = NULL;
    for (unsigned i = 0; i < ARRAY_SIZE(_target); i++) {
        _gnrc_rfc5444_target_t *tmp = &_target[i];

        if (tmp->used && _addr_equals(dst, tmp) && tmp->iface == iface) {
            target = tmp;
            break;
        }
    }

#if ENABLE_DEBUG
    if (target == NULL) {
        DEBUG("  not found\n");
    }
#endif

    gnrc_rfc5444_writer_release();
    return (target == NULL) ? NULL : &target->target;
}

const gnrc_rfc5444_packet_data_t *gnrc_rfc5444_get_packet_data(void)
{
    return &_packet_data;
}

void ipv6_addr_to_netaddr(const ipv6_addr_t *src, uint8_t pfx_len,
                          struct netaddr *dst)
{
    assert(src != NULL && dst != NULL);

    /* Guard against invalid prefix lengths */
    if (pfx_len > 128) {
        pfx_len = 128;
    }

    dst->_type = AF_INET6;
    dst->_prefix_len = pfx_len;

    memcpy(dst->_addr, src, sizeof(dst->_addr));
}

void netaddr_to_ipv6_addr(struct netaddr *src, ipv6_addr_t *dst,
                          uint8_t *pfx_len)
{
    assert(src != NULL && dst != NULL);

    /* Address is unspecified */
    if (src->_type == AF_UNSPEC) {
        memset(dst, 0, sizeof(ipv6_addr_t));
        *pfx_len = 0;
        return;
    }

    /* Guard against invalid prefix lengths */
    if (src->_prefix_len > 128) {
        *pfx_len = 128;
    }
    else {
        *pfx_len = src->_prefix_len;
    }

    /* Initialize dst with only the needed bits found in the prefix length */
    ipv6_addr_t pfx;
    memcpy(&pfx, src->_addr, sizeof(ipv6_addr_t));

    ipv6_addr_init_prefix(dst, &pfx, *pfx_len);
}

static void _message_generation_notifier(struct rfc5444_writer_target *iface)
{
    _gnrc_rfc5444_target_t *target  = container_of(iface, _gnrc_rfc5444_target_t,
                                                   target);

    DEBUG("gnrc_rfc5444: message generated for target %p (dst = %s, iface = %d)\n",
          (void *)target,
          (target == NULL ) ? "NULL" : ipv6_addr_to_str(addr_str, &target->dst,
                                                        sizeof(addr_str)),
          (target == NULL) ? 0 : target->iface);

    if (target == NULL) {
        DEBUG("  invalid target\n");
        return;
    }

    target->aggregation_timeout.event.next = NULL;
    target->aggregation_timeout.event.offset = CONFIG_RFC5444_AGGREGATION_TIME;
    target->aggregation_timeout.msg.type = GNRC_RFC5444_MSG_TYPE_AGGREGATE;
    target->aggregation_timeout.msg.content.ptr = iface;

    evtimer_add_msg(&_gnrc_rfc5444_evtimer,
                    &target->aggregation_timeout, _thread_pid);
}

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
            case GNRC_RFC5444_MSG_TYPE_AGGREGATE:
                DEBUG("gnrc_rfc5444: GNRC_RFC5444_MSG_TYPE_AGGREGATE\n");
                _aggregate((struct rfc5444_writer_target *)msg.content.ptr);
                break;

            case GNRC_NETAPI_MSG_TYPE_RCV:
                DEBUG("gnrc_rfc5444: GNRC_NETAPI_MSG_TYPE_RCV\n");
                _receive((gnrc_pktsnip_t *)msg.content.ptr);
                break;

            case GNRC_NETAPI_MSG_TYPE_GET:
            case GNRC_NETAPI_MSG_TYPE_SET:
                msg_reply(&msg, &reply);
                break;

            default:
                DEBUG("gnrc_rfc5444: received unidentified message\n");
                break;
        }
    }

    /* Never reached */
    return NULL;
}

static void _aggregate(struct rfc5444_writer_target *iface)
{
    _gnrc_rfc5444_target_t *target  = container_of(iface, _gnrc_rfc5444_target_t,
                                                   target);

    DEBUG("gnrc_rfc5444: aggregation of packets for target (dst = %s, iface = %d)\n",
          ipv6_addr_to_str(addr_str, &target->dst, sizeof(addr_str)),
          target->iface);

    gnrc_rfc5444_writer_acquire();
    rfc5444_writer_flush(&_writer, iface, false);
    gnrc_rfc5444_writer_release();
}

static void _receive(gnrc_pktsnip_t *pkt)
{
    assert(pkt != NULL);
    assert(pkt->data != NULL);

    DEBUG("gnrc_rfc5444: received packet (pkt = %p)\n", (void *)pkt);

    gnrc_rfc5444_reader_acquire();
    memset(&_packet_data, 0, sizeof(gnrc_rfc5444_packet_data_t));

    ipv6_hdr_t *ipv6_hdr = gnrc_ipv6_get_header(pkt);
    gnrc_pktsnip_t *netif_hdr = gnrc_pktsnip_search_type(pkt,
                                                         GNRC_NETTYPE_NETIF);

    if (ipv6_hdr == NULL || netif_hdr == NULL) {
        DEBUG("  invalid headers (ipv6 = %p, netif = %p)\n",
              (void *)ipv6_hdr, (void *)netif_hdr);
        gnrc_rfc5444_reader_release();
        return;
    }
    gnrc_netif_t *iface = gnrc_netif_hdr_get_netif(netif_hdr->data);

    memcpy(&_packet_data.src, &ipv6_hdr->src, sizeof(ipv6_addr_t));
    _packet_data.iface = (iface == NULL) ? KERNEL_PID_UNDEF :
                                           netif_get_id(&iface->netif);
    _packet_data.pkt = pkt;

    DEBUG("  src = %s, iface = %d\n",
          ipv6_addr_to_str(addr_str, &_packet_data.src, sizeof(addr_str)),
          _packet_data.iface);

    _debug_packet(pkt->data, pkt->size);

    /* Save packet data */
    ipv6_addr_t sender;
    assert(ipv6_hdr != NULL);
    memcpy(&sender, &ipv6_hdr->src, sizeof(ipv6_addr_t));

    if (rfc5444_reader_handle_packet(&_reader, pkt->data,
                                     pkt->size) != RFC5444_OKAY) {
        DEBUG("  couldn't handle packet\n");
    }

    gnrc_rfc5444_reader_release();
    gnrc_pktbuf_release(pkt);
}

static void _send_packet(struct rfc5444_writer *writer,
                         struct rfc5444_writer_target *iface, void *buffer,
                         size_t length)
{
    (void)writer;

    assert(iface != NULL);
    assert(buffer != NULL);
    assert(length != 0);

    _gnrc_rfc5444_target_t *target  = container_of(iface, _gnrc_rfc5444_target_t,
                                                   target);


    DEBUG("gnrc_rfc5444: sending packet (dst = %s, iface = %d)\n",
          ipv6_addr_to_str(addr_str, &target->dst, sizeof(addr_str)),
          target->iface);
    DEBUG("  length = %d, buffer = %p\n", length, (void *)buffer);

    _debug_packet(buffer, length);

    gnrc_pktsnip_t *payload;
    gnrc_pktsnip_t *udp;
    gnrc_pktsnip_t *ip;

    /* Generate our pktsnip with our RFC5444 message */
    payload = gnrc_pktbuf_add(NULL, buffer, length, GNRC_NETTYPE_UNDEF);
    if (payload == NULL) {
        DEBUG("gnrc_rfc5444: couldn't allocate payload\n");
        return;
    }

    /* Build UDP packet */
    uint16_t port = UDP_MANET_PORT;
    udp = gnrc_udp_hdr_build(payload, port, port);
    if (udp == NULL) {
        DEBUG("gnrc_rfc5444: unable to allocate UDP header\n");
        gnrc_pktbuf_release(payload);
        return;
    }

    /* Build IPv6 header */
    ip = gnrc_ipv6_hdr_build(udp, NULL, &target->dst);
    if (ip == NULL) {
        DEBUG("gnrc_rfc5444: unable to allocate IPv6 header\n");
        gnrc_pktbuf_release(udp);
        return;
    }

    /* Build netif header */
    gnrc_netif_t *netif = gnrc_netif_get_by_pid(target->iface);
    if (netif == NULL) {
        DEBUG("gnrc_rfc5444: couldn't find interface %d\n", target->iface);
        gnrc_pktbuf_release(ip);
        return;
    }

    gnrc_pktsnip_t *netif_hdr = gnrc_netif_hdr_build(NULL, 0, NULL, 0);
    gnrc_netif_hdr_set_netif(netif_hdr->data, netif);
    LL_PREPEND(ip, netif_hdr);

    /* Send packet */
    int res = gnrc_netapi_dispatch_send(GNRC_NETTYPE_UDP,
                                        GNRC_NETREG_DEMUX_CTX_ALL, ip);
    if (res < 1) {
        DEBUG("gnrc_rfc5444: unable to locate UDP thread\n");
        gnrc_pktbuf_release(ip);
        return;
    }
}


static void _debug_packet(void *data, size_t size)
{
#if ENABLE_DEBUG
    static struct autobuf hexbuf;

    /* Generate hexdump of packet */
    abuf_hexdump(&hexbuf, "\t", data, size);
    rfc5444_print_direct(&hexbuf, data, size);

    /* Print hexdump to console */
    DEBUG("%s", abuf_getptr(&hexbuf));

    abuf_free(&hexbuf);
#else
    (void)data;
    (void)size;
#endif
}
