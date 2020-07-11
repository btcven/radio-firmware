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

#include "net/aodvv2/conf.h"
#include "net/aodvv2.h"
#include "net/aodvv2/metric.h"
#include "net/aodvv2/rcs.h"
#include "net/manet.h"

#include "mutex.h"

#include "_aodvv2.h"
#include "_aodvv2-buffer.h"
#include "_aodvv2-lrs.h"
#include "_aodvv2-mcmsg.h"
#include "_aodvv2-neigh.h"
#include "_aodvv2-reader.h"
#include "_aodvv2-seqnum.h"
#include "_aodvv2-writer.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

static void _route_info(unsigned type, const ipv6_addr_t *ctx_addr,
                        const void *ctx);
static void _route_request(gnrc_pktsnip_t *pkt, const ipv6_addr_t *dst);

static void *_thread(void *arg);

static _priority_msgqueue_node_t *_alloc_priority_msgqueue_node(void);
static void _priority_msgqueue_push(msg_t *msg);
static uint32_t _priority_msgqueue_length(void);
static _priority_msgqueue_node_t* _priority_msgqueue_head(void);
static void _priority_msgqueue_remove_head(void);

#if ENABLE_DEBUG
static char _thread_stack[CONFIG_AODVV2_STACK_SIZE + THREAD_EXTRA_STACKSIZE_PRINTF];
#else
static char _thread_stack[CONFIG_AODVV2_STACK_SIZE];
#endif

static kernel_pid_t _thread_pid;

static priority_queue_t _queue = PRIORITY_QUEUE_INIT;
static _priority_msgqueue_node_t _nodes[CONFIG_AODVV2_CONTROL_TRAFFIC_LIMIT];

static timex_t _last_sent_timestamp;
static timex_t _rate_limit;

static char addr_str[IPV6_ADDR_MAX_STR_LEN];

int aodvv2_init(void)
{
    DEBUG("aodvv2: initializing\n");

    memset(&_nodes, 0, sizeof(_nodes));
    _last_sent_timestamp = timex_set(0, 0);
    _rate_limit = timex_set(0, (1 * US_PER_SEC) / CONFIG_AODVV2_CONTROL_TRAFFIC_LIMIT);
    timex_normalize(&_rate_limit);

    _aodvv2_buffer_init();
    _aodvv2_lrs_init();
    _aodvv2_mcmsg_init();
    _aodvv2_neigh_init();

    /* Initialize reader */
    gnrc_rfc5444_reader_acquire();
    _aodvv2_reader_init();
    gnrc_rfc5444_reader_release();

    /* Initialize SeqNum */
    _aodvv2_seqnum_init();

    gnrc_rfc5444_writer_acquire();
    if (_aodvv2_writer_init() < 0) {
        DEBUG("  couldn't initialize writer\n");
        gnrc_rfc5444_writer_release();
        return -ENOMEM;
    }
    gnrc_rfc5444_writer_release();

    /* Initialize Router Client Set */
    aodvv2_rcs_init();

    if (_thread_pid == KERNEL_PID_UNDEF) {
        _thread_pid = thread_create(_thread_stack, sizeof(_thread_stack),
                                    CONFIG_AODVV2_PRIO, THREAD_CREATE_STACKTEST,
                                    _thread, NULL, "aodvv2");
        if (_thread_pid < 0) {
            return _thread_pid;
        }
    }

    return 0;
}

int aodvv2_gnrc_netif_join(gnrc_netif_t *netif)
{
    DEBUG("aodvv2: joining netif %d to AODVv2\n", netif_get_id(&netif->netif));

    /* Install route info callback, this is called from the NIB when a route is
     * needed */
    netif->ipv6.route_info_cb = _route_info;

    /* Joint LL-MANET-Routers IPv6 multicast group, this is, to listen to
     * RFC 5444 multicast packets. */
    if (manet_netif_ipv6_group_join(netif) < 0) {
        DEBUG("  couldn't join LL-MANET-Routers group\n");
        return -1;
    }

    /* Add target to LL-MANET-Routers multicast address on this interface */
    if (gnrc_rfc5444_add_writer_target(&ipv6_addr_all_manet_routers_link_local,
                                       netif_get_id(&netif->netif)) < 0) {
        DEBUG("  couldn't add RFC 5444 target\n");
        return -1;
    }

    return 0;
}

int _aodvv2_send_message(uint16_t prio, aodvv2_message_t *message,
                         ipv6_addr_t *dst, uint16_t iface)
{
    DEBUG("aodvv2: sending message %d (prio = %d dst = %s, iface = %d)\n",
          prio, message->type,
          (dst == NULL) ? "NULL" : ipv6_addr_to_str(addr_str, dst,
                                                    sizeof(addr_str)), iface);

    _aodvv2_ipc_msg_t *ipc = malloc(sizeof(_aodvv2_ipc_msg_t));
    memset(ipc, 0, sizeof(_aodvv2_ipc_msg_t));

    ipc->prio = prio;
    memcpy(&ipc->msg, message, sizeof(aodvv2_message_t));
    if (dst != NULL) {
        memcpy(&ipc->dst, dst, sizeof(ipv6_addr_t));
    }
    if (iface != KERNEL_PID_UNDEF) {
        ipc->iface = iface;
    }

    msg_t msg;
    msg.type = _AODVV2_MSG_TYPE_SND;
    msg.content.ptr = ipc;
    return msg_send(&msg, _thread_pid);
}

static void _route_info(unsigned type, const ipv6_addr_t *ctx_addr,
                        const void *ctx)
{
    DEBUG("aodvv2: route info (type = %d)\n", type);

    switch (type) {
        case GNRC_IPV6_NIB_ROUTE_INFO_TYPE_UNDEF:
            DEBUG("  GNRC_IPV6_NIB_ROUTE_INFO_TYPE_UNDEF\n");
            break;

        case GNRC_IPV6_NIB_ROUTE_INFO_TYPE_RRQ:
            DEBUG("  GNRC_IPV6_NIB_ROUTE_INFO_TYPE_RRQ\n");
            _route_request((gnrc_pktsnip_t *)ctx, ctx_addr);
            break;

        case GNRC_IPV6_NIB_ROUTE_INFO_TYPE_RN:
            DEBUG("  GNRC_IPV6_NIB_ROUTE_INFO_TYPE_RN\n");
            break;

        case GNRC_IPV6_NIB_ROUTE_INFO_TYPE_NSC:
            DEBUG("  GNRC_IPV6_NIB_ROUTE_INFO_TYPE_NSC\n");
            break;

        default:
            DEBUG("  unknown route info");
            break;
    }
}

static void _route_request(gnrc_pktsnip_t *pkt, const ipv6_addr_t *dst)
{
    assert(pkt != NULL);
    assert(dst != NULL);

    DEBUG("aodvv2: route request (pkt = %p, dst = %s)\n", (void *)pkt,
          ipv6_addr_to_str(addr_str, dst, sizeof(addr_str)));

    if (ipv6_addr_is_unspecified(dst) || !ipv6_addr_is_global(dst)) {
        DEBUG("  tried to request route to invalid address\n");
        return;
    }

    ipv6_hdr_t *ipv6_hdr = gnrc_ipv6_get_header(pkt);
    if (ipv6_hdr == NULL) {
        DEBUG("  IPv6 header not found\n");
        return;
    }

    aodvv2_router_client_t client;
    if (aodvv2_rcs_get(&client, &ipv6_hdr->src) < 0) {
        DEBUG("  no matching client for %s\n",
              ipv6_addr_to_str(addr_str, &ipv6_hdr->src, sizeof(addr_str)));
        return;
    }

    /* Buffer this packet so the LRS can check for it */
    if (_aodvv2_buffer_pkt_add(pkt) < 0) {
        DEBUG("  packet buffer is full\n");
        return;
    }

    /* Tell the LRS to lookup a route for the given buffered packet, if an
     * "Idle" route exists, it will be marked as "Active" and the buffered
     * packet(s) will be dispatched, if "Unconfirmed" routes exist, a RREP_Ack
     * request may be triggered in order to confirm bidirectionality, however
     * it's not a guarantee that the the request will get acknowledged, so we
     * do not wait here and when we get the answer, the LRS alone will dispatch
     * the packets if a route isn't already found.
     */
    _aodvv2_lrs_acquire();
    _aodvv2_local_route_t *lr;
    if ((lr = _aodvv2_lrs_find(dst)) == NULL) {
        DEBUG("  route doesn't exists\n");
    }

    aodvv2_message_t msg;
    memset(&msg, 0, sizeof(aodvv2_msg_rreq_t));

    msg.type = AODVV2_MSGTYPE_RREQ;
    msg.rreq.msg_hop_limit = CONFIG_AODVV2_MAX_HOPCOUNT;
    memcpy(&msg.rreq.orig_prefix, &ipv6_hdr->src, sizeof(ipv6_addr_t));
    memcpy(&msg.rreq.targ_prefix, dst, sizeof(ipv6_addr_t));
    msg.rreq.orig_pfx_len = client.pfx_len;
    msg.rreq.orig_seqnum = _aodvv2_seqnum_new();
    msg.rreq.targ_seqnum = lr == NULL ? 0 : lr->seqnum;
    msg.rreq.orig_metric = client.cost;
    msg.rreq.metric_type = AODVV2_METRIC_TYPE_HOP_COUNT;
    _aodvv2_lrs_release();

    if (_aodvv2_send_message(_AODVV2_MSG_PRIO_RREQ, &msg, NULL, 0) < 1) {
        DEBUG("  could not send AOVVv2 message\n");
    }
}

static void *_thread(void *arg)
{
    (void)arg;

    msg_t msg;
    timex_t now;
    msg_t msg_queue[CONFIG_AODVV2_CONTROL_TRAFFIC_LIMIT];

    msg_init_queue(msg_queue, CONFIG_AODVV2_CONTROL_TRAFFIC_LIMIT);

    while (1) {
        /* If we have available messages or the queue is empty, wait for new
         * messages */
        while (msg_avail() || _priority_msgqueue_length() == 0) {
            msg_receive(&msg);
            if (msg.type == _AODVV2_MSG_TYPE_SND) {
                _priority_msgqueue_push(&msg);
            }
            else {
                DEBUG("  unknown message type %04x\n", msg.type);
            }
        }

        xtimer_now_timex(&now);
        if (timex_cmp(now, timex_add(_last_sent_timestamp, _rate_limit)) >= 0) {
            _priority_msgqueue_node_t *node = _priority_msgqueue_head();

            switch (node->msg.type) {
                case AODVV2_MSGTYPE_RREQ:
                    _aodvv2_writer_send_rreq(&node->msg.rreq);
                    break;

                case AODVV2_MSGTYPE_RREP:
                    _aodvv2_writer_send_rrep(&node->msg.rrep, &node->addr,
                                             node->iface);
                    break;

                /* TODO(jeandudey): not implemented */
                case AODVV2_MSGTYPE_RERR:
                    assert(false);
                    break;

                case AODVV2_MSGTYPE_RREP_ACK:
                    _aodvv2_writer_send_rrep_ack(&node->msg.rrep_ack,
                                                 &node->addr, node->iface);
                    break;

                default:
                    assert(false);
            }
            xtimer_now_timex(&_last_sent_timestamp);

            _priority_msgqueue_remove_head();
        }
        else {
            /* Wait until we can process a new message */
            timex_t diff = timex_sub(now, timex_add(_last_sent_timestamp, _rate_limit));
            xtimer_usleep(timex_uint64(diff));
        }
    }

    /* Not reachable */
    return NULL;
}

static _priority_msgqueue_node_t *_alloc_priority_msgqueue_node(void)
{
    DEBUG("aodvv2: allocating node\n");
    for (unsigned i = 0; i < ARRAY_SIZE(_nodes); i++) {
        if (_nodes[i].node.data == 0) {
            _nodes[i].node.data = 1;
            return &_nodes[i];
        }
    }

    DEBUG("  TRAFFIC QUEUE FULL!\n");
    return NULL;
}

static void _priority_msgqueue_push(msg_t *msg)
{
    assert(msg != NULL);

    DEBUG("aodvv2: inserting newly received message (msg->type = %d)\n",
          msg->type);

    _priority_msgqueue_node_t *node = _alloc_priority_msgqueue_node();
    if (node == NULL) {
        DEBUG("  control limit reached\n");
        /* TODO(jeandudey): search for lowest priority node */
        return;
    }

    _aodvv2_ipc_msg_t *tmp = (_aodvv2_ipc_msg_t *)msg->content.ptr;
    assert(tmp != NULL);

    _priority_msgqueue_node_init(node, tmp->prio, &tmp->msg, &tmp->dst, tmp->iface);
    free(tmp);

    priority_queue_add(&_queue, &node->node);
}


static uint32_t _priority_msgqueue_length(void)
{
    uint32_t length = 0;
    priority_queue_node_t *node = _queue.first;
    if (!node) {
        return length;
    }

    length++;
    while (node->next != NULL) {
        length++;
        node = node->next;
    }
    return length;
}

static _priority_msgqueue_node_t* _priority_msgqueue_head(void)
{
    if (_priority_msgqueue_length() == 0) {
        return NULL;
    }
    return container_of(_queue.first, _priority_msgqueue_node_t, node);
}

static void _priority_msgqueue_remove_head(void)
{
    if (_priority_msgqueue_length() == 0) {
        return;
    }
    priority_queue_node_t *tmp = priority_queue_remove_head(&_queue);
    tmp->next = NULL;
    tmp->data = 0;
}
