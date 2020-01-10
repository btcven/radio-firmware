/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * @file aodv.c
 * @author Locha Mesh project developers (locha.io)
 * @brief Implementation of:
 *
 *          Ad Hoc On-demand Distance Vector Version
 *            https://tools.ietf.org/html/rfc3561
 *
 * @version 0.1
 * @date 2019-12-10
 *
 * @copyright Copyright (c) 2019 Locha Mesh project developers
 * @license Apache 2.0, see LICENSE file for details
 */

#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip.h"
#include "sys/process.h"
#include "sys/log.h"

#include "aodv-routing.h"
#include "aodv-conf.h"
#include "aodv-defs.h"
#include "aodv-fwc.h"
#include "aodv-rt.h"

#define LOG_MODULE "aodv"
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(aodv_process, "AODV");
/*---------------------------------------------------------------------------*/
/* Used to send multicast packets */
static struct uip_udp_conn* multicast_tx_conn; 
/* Used to receive multicast packets */
static struct uip_udp_conn* multicast_rx_conn; 
/* Used to send unicast packets */
static struct simple_udp_connection unicast_conn;   
/*---------------------------------------------------------------------------*/
void aodv_routing_init(void)
{
  process_start(&aodv_process, NULL);
}
/*---------------------------------------------------------------------------*/
/**
 * \biref Compare sequence numbers as per RFC 3561 on Section 6.1 "Maintaining
 * Sequence Numbers".
 */
#define SCMP32(a, b) ((int32_t)((int32_t)(a) - (int32_t)(b)))
/*---------------------------------------------------------------------------*/
/**
 * \brief Look the last known sequence number for the host.
 *
 * \param host: Host address.
 *
 * \return Sequence number in network byte order.
 */
static CC_INLINE uint32_t
last_known_seqno(uip_ipaddr_t *host)
{
  aodv_rt_entry_t *route = NULL;

  route = aodv_rt_lookup_any(host);

  if (route != NULL) {
    /* Convert the sequence number to network byte order. */
    return uip_htonl(route->hseqno);
  }

  /* Not found. */
  return 0;
}
/*---------------------------------------------------------------------------*/
static uint32_t rreq_id = 0;   /*!< Current RREQ ID */
static uint32_t my_hseqno = 0; /*!< In host byte order! */

#define uip_udp_sender() (&(UIP_IP_BUF->srcipaddr))
/*---------------------------------------------------------------------------*/
static void
sendto(const uip_ipaddr_t *dest, const void *buf, int len)
{
  simple_udp_sendto(&unicast_conn, buf, len, dest);
}
/*---------------------------------------------------------------------------*/
void
aodv_send_rreq(uip_ipaddr_t *addr)
{
  aodv_msg_rreq_t rm = {0};

  LOG_INFO("sending RREQ.\n");

  uip_ds6_addr_t* lladdr = uip_ds6_get_link_local(-1);

  if(uip_ipaddr_cmp(addr, &lladdr->ipaddr)) {
    LOG_ERR("Can't request route to ourself!\n");
    return;
  }

  rm.type = AODV_TYPE_RREQ;

  rm.dest_seqno = last_known_seqno(addr);
  if (rm.dest_seqno == 0) {
    LOG_INFO("Unknown sequence number\n");
    rm.flags = AODV_RREQ_FLAG_UNKSEQNO;
  } else {
    rm.flags = 0;
  }

  rm.reserved = 0;
  rm.hop_count = 0;
  rm.rreq_id = uip_htonl(rreq_id);
  /* Increment RREQ ID because the current is already used. */
  rreq_id += 1;
  rm.orig_seqno = uip_htonl(my_hseqno);

  uip_ipaddr_copy(&rm.dest_addr, addr);
  uip_ipaddr_copy(&rm.orig_addr, &lladdr->ipaddr);

    /* Always. */
  my_hseqno += 1;

  multicast_tx_conn->ttl = AODV_NET_DIAMETER;
  uip_udp_packet_send(multicast_tx_conn, &rm, sizeof(aodv_msg_rreq_t));
}
/*---------------------------------------------------------------------------*/
void
aodv_send_rrep(uip_ipaddr_t *dest,
               uip_ipaddr_t *nexthop,
               uip_ipaddr_t *orig,
               uint32_t *seqno,
               unsigned hop_count)
{
  aodv_msg_rrep_t rm = {0};

  LOG_INFO("Sending RREP ");
  LOG_INFO_(" to ");
  LOG_INFO_6ADDR(nexthop);
  LOG_INFO_("hops=%u ", hop_count);
  LOG_INFO_("dest=");
  LOG_INFO_6ADDR(dest);
  LOG_INFO_("seq=%lu\n", *seqno);

  rm.type = AODV_TYPE_RREP;
  rm.flags = 0;
  rm.prefix_sz = 0;
  rm.hop_count = hop_count;
  rm.dest_seqno = *seqno;
  uip_ipaddr_copy(&rm.dest_addr, dest);
  uip_ipaddr_copy(&rm.orig_addr, orig);
  rm.lifetime = UIP_HTONL(AODV_ROUTE_TIMEOUT);

  sendto(nexthop, &rm, sizeof(aodv_msg_rrep_t));
}
/*---------------------------------------------------------------------------*/
void
aodv_send_rerr(uip_ipaddr_t *addr, uint32_t *seqno)
{
  aodv_msg_rerr_t rm = {0};

  rm.type = AODV_TYPE_RERR;
  rm.reserved = 0;
  rm.dest_count = 1;
  uip_ipaddr_copy(&rm.unreach[0].addr, addr);
  rm.unreach[0].seqno = *seqno;
  rm.flags = 0;

  uip_udp_packet_send(multicast_tx_conn, &rm, sizeof(aodv_msg_rerr_t));
}
/*---------------------------------------------------------------------------*/
static void
handle_incoming_rreq(const uint8_t* data, uint16_t datalen)
{
  /*
   * Defensive coding. data SHOULD be valid here, but
   * we test to avoid crashes at runtime.
   */
  if(data == NULL) {
    LOG_ERR("RREQ has no data.\n");
    return;
  }

  /*
   * Check the length of the packet, if doesn't met the needed
   * size we don't process it. It can be larger, but we don't
   * worry about any extra bytes.
   */
  if(datalen < sizeof(aodv_msg_rreq_t)) {
    LOG_ERR("RREQ is too short, is %d expected at least %d.\n",
    uip_len,
    sizeof(aodv_msg_rreq_t));
    return;
  }

  aodv_msg_rreq_t *rm = (aodv_msg_rreq_t *)data;

    /*
     * Defensive coding. This is already checked by handle_incoming_packet.
     * Anyway it's checked here to avoid a misuse of the function.
     */
  if(rm->type != AODV_TYPE_RREQ) {
    LOG_ERR("Invalid AODV message type.\n");
    return;
  }

  uip_ds6_addr_t *lladdr = uip_ds6_get_link_local(-1);

  LOG_INFO("RREQ from ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
  LOG_INFO_(" to ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->destipaddr);
  LOG_INFO_(" ttl=%u ", UIP_IP_BUF->ttl);
  LOG_INFO_("orig=");
  LOG_INFO_6ADDR(&rm->orig_addr);
  LOG_INFO_(" seq=%lu ", uip_ntohl(rm->orig_seqno));
  LOG_INFO_("hops=%u ", rm->hop_count);
  LOG_INFO_("dest=");
  LOG_INFO_6ADDR(&rm->dest_addr);
  LOG_INFO_("seq=%lu\n", uip_ntohl(rm->dest_seqno));

  aodv_rt_entry_t *rt = NULL;
  aodv_rt_entry_t *fw = NULL;

  /*
   * Check if we have this route stored, and verify that:
   *
   * 1. It's a new route.
   * 2. Or it is a better route (has less hops).
   */
  rt = aodv_rt_lookup(&rm->orig_addr);
  if(rt == NULL
     || (SCMP32(uip_ntohl(rm->orig_seqno), rt->hseqno) > 0) /* New route. */
     || (SCMP32(uip_ntohl(rm->orig_seqno), rt->hseqno) == 0
     && rm->hop_count < rt->hop_count)) { /* Better route. */
      LOG_INFO("RREQ is a new route.\n");
      rt = aodv_rt_add(&rm->orig_addr, uip_udp_sender(),
                         rm->hop_count, &rm->orig_seqno);
  }

  /*
   * Check if it is for our address or a fresh route.
   *
   * XXX: we currently don't set the DESTONLY flag when sending RREQs.
   */
  if(uip_ipaddr_cmp(&rm->dest_addr, &lladdr->ipaddr)
     || rm->flags & AODV_RREQ_FLAG_DESTONLY) {
      LOG_INFO("RREQ is for our address.\n");
      fw = NULL;
  } else {
    /* Check if we have a route to dest_addr */
    fw = aodv_rt_lookup(&rm->dest_addr);
    if(!(rm->flags & AODV_RREQ_FLAG_UNKSEQNO)
       && fw != NULL
       && SCMP32(fw->hseqno, uip_ntohl(rm->dest_seqno)) <= 0) {
      fw = NULL;
    }
  }

  uip_ipaddr_t dest_addr = {0};
  uip_ipaddr_t orig_addr = {0};

  /* If we have a route to dest_addr, send the RREP back */
  if (fw != NULL) {
    LOG_INFO("Route found! sending RREP.\n");
    uint32_t net_seqno = 0;

    uip_ip6addr_copy(&dest_addr, &rm->dest_addr);
    uip_ip6addr_copy(&orig_addr, &rm->orig_addr);

    net_seqno = uip_htonl(fw->hseqno);

    aodv_send_rrep(&dest_addr, &rt->nexthop, &orig_addr, &net_seqno,
        fw->hop_count + 1);
  } else if(uip_ipaddr_cmp(&rm->dest_addr, &lladdr->ipaddr)) {
    LOG_INFO("RREQ is for our address.\n");
    uint32_t net_seqno = 0;

    uip_ip6addr_copy(&dest_addr, &rm->dest_addr);
    uip_ip6addr_copy(&orig_addr, &rm->orig_addr);

    my_hseqno += 1;
    if(!(rm->flags & AODV_RREQ_FLAG_UNKSEQNO)
       && SCMP32(my_hseqno, uip_ntohl(rm->dest_seqno)) < 0) {
      my_hseqno = uip_ntohl(rm->dest_seqno) + 1;
    }
    net_seqno = uip_htonl(my_hseqno);

    aodv_send_rrep(&dest_addr, &rt->nexthop, &orig_addr, &net_seqno, 0);
  } else if(UIP_IP_BUF->ttl > 1) {
    LOG_INFO("Re-sending RREQ.\n");

    /* Have we seen this RREQ before? */
    if(aodv_fwc_lookup(&rm->orig_addr, &rm->rreq_id)) {
      return;
    }
    aodv_fwc_add(&rm->orig_addr, &rm->rreq_id);

    rm->hop_count += 1;
    multicast_tx_conn->ttl = UIP_IP_BUF->ttl - 1;

    uip_udp_packet_send(multicast_tx_conn, rm, sizeof(aodv_msg_rreq_t));
  }
}
/*---------------------------------------------------------------------------*/
static void
handle_incoming_rrep(const uint8_t* data, uint16_t datalen)
{
  if(data == NULL) {
    LOG_ERR("RREP has no data.\n");
    return;
  }

  if(datalen < sizeof(aodv_msg_rrep_t)) {
    LOG_ERR("RREP is too short, is %d expected at least %d.\n", uip_len, sizeof(aodv_msg_rrep_t));
    return;
  }

  aodv_msg_rrep_t *rm = (aodv_msg_rrep_t *)data;

  if(rm->type != AODV_TYPE_RREP) {
    LOG_ERR("Invalid RREP message type.\n");
    return;
  }

  LOG_INFO("RREP from ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
  LOG_INFO_(" to ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->destipaddr);
  LOG_INFO_(" ttl=%u ", UIP_IP_BUF->ttl);
  LOG_INFO_("prefix_sz=%u hop_count=%u ", rm->prefix_sz, rm->hop_count);
  LOG_INFO_("dest_seqno=%lu ", rm->dest_seqno);
  LOG_INFO_("dest=");
  LOG_INFO_6ADDR(&rm->dest_addr);
  LOG_INFO_(" orig=");
  LOG_INFO_6ADDR(&rm->orig_addr);
  LOG_INFO_(" lifetime=%lu\n", rm->lifetime);

  uip_ds6_addr_t *lladdr = uip_ds6_get_link_local(-1);

  /* Useless HELLO message? */
  if(uip_ipaddr_cmp(&UIP_IP_BUF->destipaddr, &multicast_tx_conn->ripaddr)) {
#ifdef AODV_RESPOND_TO_HELLOS
    /* Sometimes it helps to send a non-requested RREP in response! */
    uint32_t net_seqno = uip_htonl(my_hseqno);
    aodv_send_rrep(&lladdr->ipaddr, &UIP_IP_BUF->srcipaddr,
        &UIP_IP_BUF->srcipaddr, &net_seqno, 0);
#endif
    return;
  }

  aodv_rt_entry_t *rt = aodv_rt_lookup(&rm->dest_addr);

  /* New forward route? */
  if(rt == NULL || (SCMP32(uip_ntohl(rm->dest_seqno), rt->hseqno) > 0)) {
    rt = aodv_rt_add(&rm->dest_addr, uip_udp_sender(),
        rm->hop_count, &rm->dest_seqno);
  } else {
    LOG_INFO("Not inserting\n");
  }

  /* Forward RREP towards originator? */
  if(uip_ipaddr_cmp(&rm->orig_addr, &lladdr->ipaddr)) {
    LOG_INFO("ROUTE FOUND\n");
    if(rm->flags & AODV_RREP_FLAG_ACK) {
      aodv_msg_rrep_ack_t ack = {0};

      ack.type = AODV_TYPE_RREP_ACK;
      ack.reserved = 0;
      sendto(uip_udp_sender(), &ack, sizeof(aodv_msg_rrep_ack_t));
    }
  } else {
    rt = aodv_rt_lookup(&rm->orig_addr);

    if(rt == NULL) {
      LOG_INFO("RREP received, but no route back to originator... :-( \n");
      return;
    }

    if(rm->flags & AODV_RREP_FLAG_ACK) {
      LOG_INFO("RREP with ACK request (ignored)!\n");
      /* Don't want any RREP-ACKs in return! */
      rm->flags &= ~AODV_RREP_FLAG_ACK;
    }

    rm->hop_count += 1;

    LOG_INFO_("Forwarding RREP to ");
    LOG_INFO_6ADDR(&rt->nexthop);
    LOG_INFO_("\n");

    sendto(&rt->nexthop, rm, sizeof(aodv_msg_rrep_t));
  }
}
/*---------------------------------------------------------------------------*/
static void
handle_incoming_rerr(const uint8_t* data, uint16_t datalen)
{
  if(data == NULL) {
    LOG_ERR("RERR has no data.\n");
    return;
  }

  if(datalen < sizeof(aodv_msg_rerr_t)) {
    LOG_ERR("RERR is too short, is %d expected at least %d.\n",
        uip_len, sizeof(aodv_msg_rerr_t));
    return;
  }

  aodv_msg_rerr_t *rm = (aodv_msg_rerr_t *)data;

  if(rm->type != AODV_TYPE_RREP) {
    LOG_ERR("Invalid RERR message type.\n");
    return;
  }

  /* TODO: handle RREP logic */
}
/*---------------------------------------------------------------------------*/
static void
handle_incoming_packet(const uint8_t* data, uint16_t datalen)
{
  if(data == NULL) {
    LOG_ERR("AODV message has no data.\n");
    return;
  }

  if(datalen < sizeof(aodv_msg_t)) {
    LOG_ERR("AODV message is too short.\n");
    return;
  }

  aodv_msg_t *m = (aodv_msg_t *)data;

  switch(m->type) {
  case AODV_TYPE_RREQ:
    handle_incoming_rreq(data, datalen);
    break;
  case AODV_TYPE_RREP:
    handle_incoming_rrep(data, datalen);
    break;
  case AODV_TYPE_RERR:
    handle_incoming_rerr(data, datalen);
    break;
  }
}
/*---------------------------------------------------------------------------*/
static void
unicast_rx(struct simple_udp_connection *c,
           const uip_ipaddr_t *source_addr,
           uint16_t source_port,
           const uip_ipaddr_t *dest_addr,
           uint16_t dest_port,
           const uint8_t *data,
           uint16_t datalen)
{
    handle_incoming_packet(data, datalen);
}
/*---------------------------------------------------------------------------*/
static enum {
  COMMAND_NONE,
  COMMAND_SEND_RREQ,
  COMMAND_SEND_RERR,
} command;
/*---------------------------------------------------------------------------*/
static uip_ipaddr_t bad_dest;
static uint32_t bad_seqno;      /* In network byte order! */
/*---------------------------------------------------------------------------*/
void
aodv_bad_dest(uip_ipaddr_t *dest)
{
  aodv_rt_entry_t *rt = aodv_rt_lookup_any(dest);

  if (rt == NULL) {
    bad_seqno = 0;      /* Or flag this in RERR? */
  } else {
    rt->is_bad = 1;
    bad_seqno = uip_htonl(rt->hseqno);
  }

  uip_ipaddr_copy(&bad_dest, dest);
  command = COMMAND_SEND_RERR;
  process_post(&aodv_process, PROCESS_EVENT_MSG, NULL);
}
/*---------------------------------------------------------------------------*/
static uip_ipaddr_t rreq_addr;
static struct timer next_time;
/*---------------------------------------------------------------------------*/
aodv_rt_entry_t *
aodv_request_route_to(uip_ipaddr_t *host)
{
  LOG_INFO("Requesting route to ");
  LOG_INFO_6ADDR(host);
  LOG_INFO_(".\n");
  /* Look on the Routing Table if this route is know. */
  aodv_rt_entry_t *route = aodv_rt_lookup(host);

  if (route != NULL) {
    LOG_INFO("Route exists in table.\n");
    /* The route exists; set as the Least Recently Used. */
    aodv_rt_lru(route);
    return route;
  }

  /* Broadcast protocols must be rate-limited! */
  if(!timer_expired(&next_time)) {
    LOG_WARN("Route request has been made before timeout!\n");
    return NULL;
  }

  /* We are processing another command. */
  if (command != COMMAND_NONE) {
    LOG_WARN("A command is being processed!\n");
    return NULL;
  }

  LOG_DBG("Sending command to aodv_process.\n");

  uip_ipaddr_copy(&rreq_addr, host);
  command = COMMAND_SEND_RREQ;
  process_post(&aodv_process, PROCESS_EVENT_MSG, NULL);
  timer_set(&next_time, CLOCK_SECOND/8); /* Max 10/s per RFC3561. */
  return NULL;
}

PROCESS_THREAD(aodv_process, ev, data)
{
  PROCESS_EXITHANDLER(goto exit);

  PROCESS_BEGIN();

  /*
   * Create broadcast UDP connection to send packets to the ff02::1,
   * This connection is ONLY to send packets, it doesn't receive anything.
   */
  multicast_tx_conn = udp_broadcast_new(UIP_HTONS(AODV_UDPPORT), NULL);

  /*
   * Create a UDP connection to receive (RX) packets sent to a ff02:1 address.
   * This connection is ONLY to receive packets, it SHOULD NOT be used to
   * send anything as it does not has a Remote Address.
   */
  multicast_rx_conn = udp_new(NULL, 0, NULL);
  if(multicast_rx_conn == NULL) {
    LOG_ERR("Couldn't create multicast connection.\n");
    PROCESS_EXIT();
  }
  /*
   * Bind the connection to the AODV UDP port (per the RFC it's 654).
   */
  udp_bind(multicast_rx_conn, UIP_HTONS(AODV_UDPPORT));

  /*
   * Create a unicast connection, acually the address is changed to the address
   * of the node that the response should be sent.
   */
  simple_udp_register(&unicast_conn, AODV_UDPPORT, NULL, AODV_UDPPORT, unicast_rx);

  while(1) {
    PROCESS_WAIT_EVENT();

    if(ev == tcpip_event) {
      LOG_INFO("New TCPIP event\n");
      /*
       * Check if we have received any UDP data.
       */
      if(uip_newdata()) {
        LOG_INFO("Received UDP datagram\n");
        handle_incoming_packet(uip_appdata, uip_datalen());
        continue;
      }

      if (uip_poll()) {
        if(command == COMMAND_SEND_RREQ) {
          LOG_INFO("Received COMMAND_SEND_RREQ\n");
          /*
           * Search in the Routing Table if we have that address stored,
           * if not, send RREQs to all near nodes.
           */
          if(aodv_rt_lookup(&rreq_addr) == NULL)
            aodv_send_rreq(&rreq_addr);
        } else if(command == COMMAND_SEND_RERR) {
          aodv_send_rerr(&bad_dest, &bad_seqno);
        }

        /*
         * Reset the command state.
         */
        command = COMMAND_NONE;
        continue;
      }
    }

    if (ev == PROCESS_EVENT_MSG) {
      tcpip_poll_udp(multicast_rx_conn);
    }
  }

exit:
  command = COMMAND_NONE;
  aodv_rt_flush_all();

  uip_udp_remove(multicast_tx_conn);
  multicast_tx_conn = NULL;

  uip_udp_remove(multicast_rx_conn);
  multicast_rx_conn = NULL;

  PROCESS_END();
}
