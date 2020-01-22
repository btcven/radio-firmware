/**
 * @file aodv-defs.h
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

#ifndef AODV_CONF_H_
#define AODV_CONF_H_

#ifndef AODV_UDPPORT
/*! UDP port for AODV */
#define AODV_UDPPORT 654
#endif

#ifndef AODV_NUM_RT_ENTRIES
#define AODV_NUM_RT_ENTRIES 8 /*!< Max. number of entries in the Routing Table. */
#endif

#ifndef AODV_NUM_FW_CACHE
#define AODV_NUM_FW_CACHE 16 /*!< Max. number of entries in the Forward Cache */
#endif

#ifndef AODV_RESPOND_TO_HELLOS
#define AODV_RESPOND_TO_HELLOS 0 /*! Whether to respond to hellos or not */
#endif

#ifndef AODV_ROUTE_TIMEOUT
#define AODV_ROUTE_TIMEOUT 0x7fffffff /*!< Route timeout, the time it takes for a route to expire  */
#endif

#ifndef AODV_NET_DIAMETER
#define AODV_NET_DIAMETER 20 /*!< Network diameter */
#endif

#endif // AODV_CONF_H_
