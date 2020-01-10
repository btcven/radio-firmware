/**
 * \file aodv-fwc.c
 * \author Locha Mesh project developers (locha.io)
 * \brief AODV RREQ forward cache.
 * \version 0.1
 * \date 2019-12-10
 *
 * \copyright Copyright (c) 2019 Locha Mesh project developers
 * \license Apache 2.0, see LICENSE file for details
 */

#include "aodv-conf.h"
#include "aodv-fwc.h"

/*
 * Forward cache structure.
 */
static struct {
  uip_ipaddr_t orig; /*!< Origin Address. */
  uint32_t id;       /*!< RREQ ID. */
} fwcache[AODV_NUM_FW_CACHE];
/*---------------------------------------------------------------------------*/
static CC_INLINE unsigned
aodv_fwc_reduce_addr(const uip_ipaddr_t *orig)
{
  unsigned n = (orig->u8[2] + orig->u8[3]) % AODV_NUM_FW_CACHE;
  return n;
}
/*---------------------------------------------------------------------------*/
int
aodv_fwc_lookup(const uip_ipaddr_t *orig, const uint32_t *id)
{
  unsigned n = aodv_fwc_reduce_addr(orig);
  return fwcache[n].id == *id && uip_ipaddr_cmp(&fwcache[n].orig, orig);
}
/*---------------------------------------------------------------------------*/
void
aodv_fwc_add(const uip_ipaddr_t *orig, const uint32_t *id)
{
  unsigned n = aodv_fwc_reduce_addr(orig);
  fwcache[n].id = *id;
  uip_ipaddr_copy(&fwcache[n].orig, orig);
}
/*---------------------------------------------------------------------------*/
