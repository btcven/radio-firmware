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
 * @ingroup     aodvv2
 * @{
 *
 * @file
 * @brief       AODVv2 routing protocol
 *
 * @author      Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 * @author      Gustavo Grisales <gustavosinbandera1@hotmail.com>
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 * @}
 */

#include "aodvv2/aodvv2.h"

#include "net/gnrc/udp.h"
#include "net/sock/udp.h"

#include "reader.h"
#include "routingtable.h"
#include "seqnum.h"
#include "utils.h"
#include "writer.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

#define RCV_MSG_Q_SIZE (32)
#define UDP_BUFFER_SIZE (128) /** with respect to IEEE 802.15.4's MTU */

ipv6_addr_t ipv6_addr_all_manet_routers_link_local =
    IPV6_ADDR_ALL_MANET_ROUTERS_LINK_LOCAL;

ipv6_addr_t ipv6_addr_aodvv2_prefix =
    IPV6_ADDR_AODVV2_PREFIX;

kernel_pid_t aodvv2_if_pid = KERNEL_PID_UNDEF;

static struct netaddr na_all_manet_routers_link_local;

static char _addr_str[IPV6_ADDR_MAX_STR_LEN];

static char _send_stack[THREAD_STACKSIZE_LARGE];
static char _recv_stack[THREAD_STACKSIZE_LARGE];

static kernel_pid_t sender_pid;

static gnrc_netif_t *_netif = NULL;

static sock_udp_t _udp_sock;

/**
 * @brief   Originator (our) address, as a netaddr.
 *
 *          This is to avoid calls to ipv6_addr_to_netaddr.
 */
static struct netaddr na_orig;

/*static aodvv2_writer_target_t *wt;*/


/**
 * @brief Build RREQs, RREPs and RERRs from the information contained in the thread's message queue and send them
 *
 * @param arg this variabe is not used in this section
 * @return void*
 */
static void *_sender_thread(void *arg);
static void *_receiver_thread(void *arg);

/**
 * @brief   Handle the output of the RFC 5444 packet creation process. This
 *          callback is called by every writer_send_* function.
 *
 * @param[in] wr
 * @param[in] iface
 * @param[in] buffer
 * @param[in] length
 */
static void _write_packet(struct rfc5444_writer *wr,
                          struct rfc5444_writer_target *iface,
                          void *buffer, size_t length);

/**
 * @brief   Frees the msg_container_t memory
 */
static void _clean_msg_container(msg_container_t *mc);

void aodvv2_init(gnrc_netif_t *netif) {
    DEBUG("[aodvv2]: init\n");
    _netif = netif;
    aodvv2_if_pid = _netif->pid;

    aodvv2_seqnum_init();
    aodvv2_routingtable_init();
    aodvv2_clienttable_init();
    aodvv2_rreqtable_init();

    aodvv2_packet_writer_init(_write_packet);
    aodvv2_packet_reader_init();

    /* Initialize na_orig address */
    ipv6_addr_t orig_addr;
    if (gnrc_netapi_get(_netif->pid,
                        NETOPT_IPV6_ADDR,
                        0,
                        &orig_addr,
                        sizeof(orig_addr)) < 0) {
        DEBUG("[aodvv2]: can't get iface IPv6 address");
        return;
    }
    ipv6_addr_to_netaddr(&orig_addr, &na_orig);

    /* Every node is its own client */
    aodvv2_clienttable_add_client(&na_orig);

    /* Initialize na_all_manet_routers_link_local */
    ipv6_addr_to_netaddr(&ipv6_addr_all_manet_routers_link_local,
                         &na_all_manet_routers_link_local);

    /* Join LL-MANET-Routers multicast group */
    gnrc_netapi_opt_t opt = {
        .opt = NETOPT_IPV6_GROUP,
        .context = 0,
        .data = &ipv6_addr_all_manet_routers_link_local,
        .data_len = sizeof(ipv6_addr_t),
    };
    gnrc_netif_set_from_netdev(_netif, &opt);

    /* Create UDP socket */
    sock_udp_ep_t udp_local = SOCK_IPV6_EP_ANY;
    udp_local.port = UDP_MANET_PROTOCOLS_1;
    if (sock_udp_create(&_udp_sock, &udp_local, NULL, 0) < 0) {
        DEBUG("[aodvv2]: couldn't create UDP socket\n");
        return;
    }

    sender_pid = KERNEL_PID_UNDEF;
    sender_pid = thread_create(_send_stack,
                               sizeof(_send_stack),
                               THREAD_PRIORITY_MAIN - 1,
                               THREAD_CREATE_STACKTEST,
                               _sender_thread,
                               NULL,
                               "aodvv2_sender_thread");

    /* Start listening & enable sending */
    thread_create(_recv_stack,
                  sizeof(_recv_stack),
                  THREAD_PRIORITY_MAIN - 1,
                  THREAD_CREATE_STACKTEST,
                  _receiver_thread,
                  NULL,
                  "_receiver_thread");
}

void aodvv2_find_route(ipv6_addr_t *target_addr)
{
    ipv6_addr_to_str(_addr_str, target_addr, IPV6_ADDR_MAX_STR_LEN);
    DEBUG("[aodvv2]: finding route to %s\n", _addr_str);

    struct netaddr na_target;
    ipv6_addr_to_netaddr(target_addr, &na_target);

    aodvv2_seqnum_t seqnum = aodvv2_seqnum_get();
    aodvv2_seqnum_inc();

    aodvv2_packet_data_t rreq_data = {
        .hoplimit = AODVV2_MAX_HOPCOUNT,
        .metricType = AODVV2_DEFAULT_METRIC_TYPE,
        .origNode = {
            .addr = na_orig,
            .metric = 0,
            .seqnum = seqnum,
        },
        .targNode = {
            .addr = na_target,
        },
        .timestamp = {0},
    };
    aodvv2_send_rreq(&rreq_data);
}

void aodvv2_send_rreq(aodvv2_packet_data_t *packet_data)
{
    /* Move the AODVv2 packet data to allocated memory in order to be able
     * to pass it to the sender thread.
     */
    aodvv2_packet_data_t *pd = malloc(sizeof(aodvv2_packet_data_t));
    memcpy(pd, packet_data, sizeof(aodvv2_packet_data_t));

    /* Alloc RREQ data container */
    rreq_rrep_data_t *rd = malloc(sizeof(rreq_rrep_data_t));
    rd->next_hop = &na_all_manet_routers_link_local;
    rd->packet_data = pd;

    /* Alloc a message to be handled in sender thread, it's filled with the
     * RREQ data pointer and message type */
    msg_container_t *mc = malloc(sizeof(msg_container_t));
    mc->type = RFC5444_MSGTYPE_RREQ;
    mc->data = rd;

    /* Create the IPC message and send it */
    msg_t msg;
    msg.content.ptr = mc;

    if (msg_send(&msg, sender_pid) < 1) {
        DEBUG("[aodvv2]: sender thread can't receive messages!\n");
    }
}

void aodvv2_send_rrep(aodvv2_packet_data_t *packet_data,
                      struct netaddr *next_hop)
{
    /* Move the AODVv2 packet data to allocated memory in order to be able
     * to pass it to the sender thread.
     */
    aodvv2_packet_data_t *pd = malloc(sizeof(aodvv2_packet_data_t));
    memcpy(pd, packet_data, sizeof(aodvv2_packet_data_t));

    /* Allocate next hop netaddr */
    struct netaddr *nh = malloc(sizeof(struct netaddr));
    memcpy(nh, next_hop, sizeof(struct netaddr));

    /* Allocate RREP data container */
    rreq_rrep_data_t *rd = malloc(sizeof(rreq_rrep_data_t));
    rd->next_hop = nh;
    rd->packet_data = pd;

    /* Alloc a message to be handled in sender thread, it's filled with the
     * RREP data pointer and message type */
    msg_container_t *mc = malloc(sizeof(msg_container_t));
    mc->type = RFC5444_MSGTYPE_RREP;
    mc->data = rd;

    /* Create the IPC message and send it */
    msg_t msg;
    msg.content.ptr = mc;

    if (msg_send(&msg, sender_pid) < 1) {
        DEBUG("[aodvv2]: sender thread can't receive messages!\n");
    }
}

static void *_sender_thread(void *arg)
{
    (void)arg;

    msg_t msgq[RCV_MSG_Q_SIZE];
    msg_init_queue(msgq, sizeof msgq);

    while (true) {
        msg_t msg;
        msg_receive(&msg);

        DEBUG("[aodvv2]: sending AODV message, %lx\n", (uint32_t)msg.content.ptr);
        msg_container_t *mc = (msg_container_t *)msg.content.ptr;

        switch (mc->type) {
            case RFC5444_MSGTYPE_RREQ:
                {
                    DEBUG("[aodvv2]: msg = RREQ\n");
                    rreq_rrep_data_t *rreq_data = (rreq_rrep_data_t *)mc->data;
                    aodvv2_packet_writer_send_rreq(rreq_data->packet_data,
                                                   rreq_data->next_hop);
                }
                break;

            case RFC5444_MSGTYPE_RREP:
                {
                    DEBUG("[aodvv2]: msg = RREP\n");
                    rreq_rrep_data_t *rrep_data = (rreq_rrep_data_t *)mc->data;
                    aodvv2_packet_writer_send_rrep(rrep_data->packet_data,
                                                   rrep_data->next_hop);
                }
                break;

            default:
                DEBUG("[aodvv2]: couldn't identify msg type");
                break;
        }

        /* Free the memory allocated by send RREQ/RREP functions */
        _clean_msg_container(mc);
    }

    /* Should never be reached */
    return NULL;
}

static void _write_packet(struct rfc5444_writer *wr,
                          struct rfc5444_writer_target *iface,
                          void *buffer, size_t length)
{
    (void)wr;

    DEBUG("[aodvv2]: write packet\n");

    /* Get the aodvv2_writer_target_t from the rfc5444_writer_targe
     * pointer */
    aodvv2_writer_target_t *wt =
        container_of(iface, aodvv2_writer_target_t, interface);

    sock_udp_ep_t remote = {
        .family = AF_INET6,
        .port = UDP_MANET_PROTOCOLS_1,
        .addr = {},
    };

    switch (wt->type) {
        case RFC5444_MSGTYPE_RREQ:
            memcpy(&remote.addr, &ipv6_addr_all_manet_routers_link_local, sizeof(ipv6_addr_t));
            break;

        default:
            {
                ipv6_addr_t tmp;
                netaddr_to_ipv6_addr(&wt->target_addr, &tmp);
                memcpy(&remote.addr, &tmp, sizeof(ipv6_addr_t));
            }
            break;
    }

    DEBUG("[aodvv2]: sending packet\n");
    if (sock_udp_send(&_udp_sock, buffer, length, &remote) < 0) {
        DEBUG("[aodvv2]: error sending UDP packet\n");
        return;
    }
}

static void *_receiver_thread(void *arg)
{
    (void)arg;

    char recv_buf[UDP_BUFFER_SIZE];
    memset(recv_buf, 0, sizeof(recv_buf));

    while (1) {
        sock_udp_ep_t remote;
        ssize_t res;

        if ((res = sock_udp_recv(&_udp_sock, recv_buf, sizeof(recv_buf),
                                 SOCK_NO_TIMEOUT, &remote)) >= 0) {
            DEBUG("[aodvv2]: received remote packet\n");

            /* Convert to struct netaddr */
            struct netaddr na_sender;
            na_sender._type = AF_INET6;
            na_sender._prefix_len = AODVV2_RIOT_PREFIXLEN;
            memcpy(na_sender._addr, &remote.addr, sizeof(na_sender._addr));

            if (aodvv2_packet_reader_handle_packet(recv_buf, res, &na_sender) < 0) {
                DEBUG("[aodvv2]: failed\n");
            }
        }
    }

    return NULL;
}

static void _clean_msg_container(msg_container_t *mc)
{
    DEBUG("[aodvv2]: freeing up msg_container_t memory\n");

    switch (mc->type) {
        case RFC5444_MSGTYPE_RREQ:
            {
                rreq_rrep_data_t *ptr = (rreq_rrep_data_t *)mc->data;
                free(ptr->packet_data);
                free(ptr);
                free(mc);
            }
            break;

        case RFC5444_MSGTYPE_RREP:
            {
                rreq_rrep_data_t *ptr = (rreq_rrep_data_t *)mc->data;
                free(ptr->packet_data);
                free(ptr->next_hop);
                free(ptr);
                free(mc);
            }
            break;

        default:
            DEBUG("[aodvv2]: fatal: unknown msg_container_t type\n");
            break;
    }
}
