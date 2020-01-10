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
 * \file aodv-rt.h
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
#ifndef AODV_RT_H_
#define AODV_RT_H_

#include <stdint.h>

#include "net/ipv6/uip.h"

typedef struct aodv_rt_entry aodv_rt_entry_t; /*!< AODV routing table entry. */

/**
 * \brief AODV routing table entry.
 *
 */
struct aodv_rt_entry {
  aodv_rt_entry_t* next; /*!< Next entry in the Routing Table. */
  uip_ipaddr_t dest;     /*!< Destination. */
  uip_ipaddr_t nexthop;  /*!< Next hop. */
  uint32_t hseqno;       /*!< In host byte order! */
  uint8_t hop_count;     /*!< Hop count. */
  uint8_t is_bad;        /*!< Only one bit is used. */
};

/**
 * \param Initailize Routing Table.
 */
void aodv_rt_init(void);

/**
 * \brief Add an entry to the Routing Table.
 *
 * \param dest: Destination address.
 * \param nexthop: Next hop.
 * \param hopcount: Hop count.
 * \param seqno: Sequence number.
 *
 * \return pointer to aodv_rt_entry_t.
 */
aodv_rt_entry_t *aodv_rt_add(uip_ipaddr_t *dest,
                             uip_ipaddr_t *nexthop,
                             unsigned hop_count,
                             const uint32_t *seqno);

/**
 * \brief Lookup for any entry by the destination address.
 *
 * \param dest: Destination address.
 */
aodv_rt_entry_t *aodv_rt_lookup_any(uip_ipaddr_t *dest);

/**
 * \brief Look for an entry by the destination address. Checks if it's bad.
 *
 * \param dest: Destination address.
 *
 * \return pointer to aodv_rt_entry_t, or NULL if not found or if the entry is
 * bad, if you want to get any entry use `aodv_rt_lookup_any`.
 */
aodv_rt_entry_t *aodv_rt_lookup(uip_ipaddr_t *dest);

/**
 * \brief Remove an entry from the Routing Table.
 *
 * \param e: Entry.
 */
void aodv_rt_remove(aodv_rt_entry_t *e);

/**
 * \biref Set the entry as the Least Recently Used.
 */
void aodv_rt_lru(aodv_rt_entry_t *e);

/**
 * \brief Remove all table entries.
 */
void aodv_rt_flush_all(void);

#endif /* AODV_RT_H_ */
