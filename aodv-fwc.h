/**
 * \file aodv-fwc.h
 * \author Locha Mesh project developers (locha.io)
 * \brief AODV RREQ forward cache.
 * \version 0.1
 * \date 2019-12-10
 *
 * \copyright Copyright (c) 2019 Locha Mesh project developers
 * \license Apache 2.0, see LICENSE file for details
 */

#ifndef OS_NET_ROUTING_AODV_AODV_FWC_
#define OS_NET_ROUTING_AODV_AODV_FWC_

#include "os/net/ipv6/uip.h"

/**
 * \brief Look up a RREQ in the Forward Cache.
 *
 * \param[in] orig: Origin address.
 * \param[in] id: RREQ ID.
 *
 * \return
 *      - `0`: Not cached.
 *      - `1`: The RREQ is cached.
 */
int aodv_fwc_lookup(const uip_ipaddr_t *orig, const uint32_t *id);

/**
 * \brief Add a RREQ in the Forward Cache.
 *
 * \param[in] orig: Origin address.
 * \param[in] id: RREQ ID.
 */
void aodv_fwc_add(const uip_ipaddr_t *orig, const uint32_t *id);

#endif /* OS_NET_ROUTING_AODV_AODV_FWC_ */
