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
 * @file aodv-rt.c
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

#include "aodv-rt.h"
#include "aodv-conf.h"

#include "os/lib/list.h"
#include "os/lib/memb.h"

/**
 * @brief LRU (with respect to insertion time) list of route entries.
 *
 */
LIST(route_table);
MEMB(route_mem, aodv_rt_entry_t, AODV_NUM_RT_ENTRIES);

void
aodv_rt_init(void)
{
  list_init(route_table);
  memb_init(&route_mem);
}

aodv_rt_entry_t *
aodv_rt_add(uip_ipaddr_t *dest,
            uip_ipaddr_t *nexthop,
            unsigned hop_count,
            const uint32_t *seqno)
{
  aodv_rt_entry_t *e;

  /* Avoid inserting duplicate entries. */
  e = aodv_rt_lookup_any(dest);
  if (e != NULL) {
    list_remove(route_table, e);
  } else {
    /* Allocate a new entry or reuse the oldest. */
    e = memb_alloc(&route_mem);
    if(e == NULL) {
      /* Remove oldest entry. */
      e = list_chop(route_table);
    }
  }

  uip_ipaddr_copy(&e->dest, dest);
  uip_ipaddr_copy(&e->nexthop, nexthop);
  e->hop_count = hop_count;
  e->hseqno = uip_ntohl(*seqno);
  e->is_bad = 0;

  /* New entry goes first because it's the Least Recently Used. */
  list_push(route_table, e);

  return e;
}

aodv_rt_entry_t *
aodv_rt_lookup_any(uip_ipaddr_t *dest)
{
  aodv_rt_entry_t *e = NULL;

  /* Iterate over the list. */
  for(e = list_head(route_table); e != NULL; e = e->next) {
    if(uip_ipaddr_cmp(dest, &e->dest)) {
      return e;
    }
  }

  /* Not found. */
  return NULL;
}

aodv_rt_entry_t *
aodv_rt_lookup(uip_ipaddr_t *dest)
{
  aodv_rt_entry_t *e;

  e = aodv_rt_lookup_any(dest);
  if(e != NULL && e->is_bad)
    return NULL;

  return e;
}

void aodv_rt_remove(aodv_rt_entry_t *e)
{
  list_remove(route_table, e);
  memb_free(&route_mem, e);
}

void aodv_rt_lru(aodv_rt_entry_t *e)
{
  if(e != list_head(route_table)) {
    list_remove(route_table, e);
    list_push(route_table, e);
  }
}

void aodv_rt_flush_all(void)
{
  aodv_rt_entry_t *e;

  while (1) {
    e = list_pop(route_table);
    if(e != NULL) {
      memb_free(&route_mem, e);
    } else {
      break;
    }
  }
}
