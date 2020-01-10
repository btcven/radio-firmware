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
 * \file aodv-defs.h
 * \author Locha Mesh project developers (locha.io)
 * \brief Implementation of:
 *
 *          Ad Hoc On-demand Distance Vector Version
 *            https://tools.ietf.org/html/rfc3561
 *
 * \version 0.1
 * \date 2019-12-10
 *
 * \copyright Copyright (c) 2019 Locha Mesh project developers
 * \license Apache 2.0, see LICENSE file for details
 */

#ifndef AODV_DEFS_H_
#define AODV_DEFS_H_

/* C Standard includes */
#include <stdint.h>

/* Contiki-NG includes */
#include "contiki-net.h"

/* AODV includes */
#include "aodv-conf.h"

#define AODV_TYPE_RREQ 1     /*!< RREQ message. */
#define AODV_TYPE_RREP 2     /*!< RREP message. */
#define AODV_TYPE_RERR 3     /*!< RERR message. */
#define AODV_TYPE_RREP_ACK 4 /*!< RREP ACK message. */

typedef uint8_t aodv_type_t; /*!< AODV message type */

/**
 * \brief Generic message.
 *
 */
struct aodv_msg {
  uint8_t type;            /*!< Message type. */
};

typedef struct aodv_msg aodv_msg_t;

/**
 * \brief Join flag; reserved formulticast.
 */
#define AODV_RREQ_FLAG_JOIN       (1 << 7)
/**
 * \brief Repair flag; reserved for multicast.
 */
#define AODV_RREQ_FLAG_REPAIR     (1 << 6)
/**
 * \brief Gratuitous RREP flag; indicates whether a gratuitous RREP should be
 * unicast to the node specified in the Destination IP Address field.
 */
#define AODV_RREQ_FLAG_GRATUITOUS (1 << 5)
/**
 * \brief Destination only flag; indicates only the destination may respond to
 * this RREQ.
 */
#define AODV_RREQ_FLAG_DESTONLY   (1 << 4)
/**
 * Unknown sequence number; indicates the destination sequence number is
 * unknown.
 */
#define AODV_RREQ_FLAG_UNKSEQNO   (1 << 3)

/**
 * \brief RREQ flags.
 */
typedef uint8_t aodv_rreq_flags_t;

/**
 * \brief AODV Route Request message structure.
 */
struct aodv_msg_rreq {
  aodv_type_t type;         /*!< Message type; MUST be `AODV_TYPE_RREQ`. */
  aodv_rreq_flags_t flags;  /*!< RREQ flags. */
  uint8_t reserved;         /*!< Sent as 0; ignored on reception. */
  uint8_t hop_count;        /*!< The number of hops from the Originator IP
                                 Address to the node handling the request. */
  uint32_t rreq_id;         /*!< A sequence number uniquely identifying the
                                 particular RREQ when taken in conjunction with
                                 the originating node's IP address.*/
  uint32_t dest_seqno;      /*!< The latest sequence number received in the
                                 past by the originator for any route towards
                                 the destination. */
  uint32_t orig_seqno;      /*!< The current sequence number to be used in the
                                 route entry pointing towards the originator of
                                 the route request. */
  uip_ipaddr_t dest_addr;   /*!< The IP address of the destination for which a
                                 route is desired. Note: This is an IPv6
                                 address. */
  uip_ipaddr_t orig_addr;   /*!< The IP address of the node which originated
                                 the Route Request. */
};

/**
 * \brief AODV Route Request message type.
 */
typedef struct aodv_msg_rreq aodv_msg_rreq_t;

/**
 * \brief Repair flag; used for multicast.
 */
#define AODV_RREP_FLAG_REPAIR (1 << 7)
/**
 * \brief Acknowledgment required.
 */
#define AODV_RREP_FLAG_ACK    (1 << 6)

/**
 * \brief RREP flags.
 */
typedef uint8_t aodv_rrep_flags_t;

/**
 * \brief AODV Route Response message structure.
 */
struct aodv_msg_rrep {
  aodv_type_t type;        /*!< Message type; MUST be `AODV_TYPE_RREP`. */
  aodv_rrep_flags_t flags; /*!< RREP flags. */
  uint8_t reserved;        /*!< Sent as 0; ignored on reception. */
  uint8_t prefix_sz;       /*!< If nonzero, the 5-bit Prefix Size specifies
                                that the indicated next hop may be used for any
                                nodes with the same routing prefix (as defined
                                by the Prefix Size) as the requested
                                destination. */
  uint8_t hop_count;       /*!< The number of hops from the Originator IP
                                Address to the Destination IP Address. For
                                multicast route requests this indicates the
                                number of hops to the multicast tree member
                                sending the RREP. */
  uint32_t dest_seqno;     /*!< The destination sequence number associated to
                                the route. */
  uip_ipaddr_t dest_addr;  /*!< The IP address of the destination for which a
                                route is supplied. */
  uip_ipaddr_t orig_addr;
  uint32_t lifetime;       /*!< The time in milliseconds for which nodes
                                receiving the RREP consider the route to be
                                valid. */
};

/**
 * \brief AODV Route Response message type.
 */
typedef struct aodv_msg_rrep aodv_msg_rrep_t;

/**
 * \brief No delete flag; set when a node has performed a local repair of a
 * link, and upstream nodes should not delete the route. */
#define AODV_RERR_FLAG_NO_DELETE (1 << 7)

typedef uint8_t aodv_rerr_flags_t; /*! AODV Route Response message type. */

struct aodv_msg_rerr {
  aodv_type_t type;        /*!< Message type; MUST be `AODV_TYPE_RERR`. */
  aodv_rerr_flags_t flags; /*!< RERR flags */
  uint8_t reserved;        /*!< Sent as 0; ignored on reception. */
  uint8_t dest_count;      /*!< The number of unreachable destinations included
                                in the message; MUST be at least 1. */
  struct {
    uip_ipaddr_t addr;     /*!< Unreachable Destination IP Address
                                The IP address of the destination that has
                                become unreachable due to a link break. */
    uint32_t seqno;        /*!< Unreachable Destination Sequence Number
                                The sequence number in the route table entry
                                for the destination listed in the previous
                                Unreachable Destination IP Address field. */
  } unreach[1];
};

/**
 * \brief AODV Route Response message type.
 */
typedef struct aodv_msg_rerr aodv_msg_rerr_t;

struct aodv_msg_rrep_ack {
  aodv_type_t type; /*!< Message type; MUST be `AODV_TYPE_RREP_ACK` */
  uint8_t reserved; /*!< Sent as 0; ignored on reception. */
};

/**
 * \brief AODV Route Response message type.
 */
typedef struct aodv_msg_rrep_ack aodv_msg_rrep_ack_t;

#endif /* AODV_DEFS_H_ */
