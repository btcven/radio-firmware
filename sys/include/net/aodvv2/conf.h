/*
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
 * @brief       AODVv2 compile time configuration.
 *
 * @author      Locha Mesh Developers <contact@locha.io>
 */

#ifndef AODVV2_CONF_H
#define AODVV2_CONF_H

#include "thread.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_ADOVV2_STACK_SIZE
#define CONFIG_AODVV2_STACK_SIZE (THREAD_STACKSIZE_DEFAULT)
#endif

#ifndef CONFIG_AODVV2_PRIO
#define CONFIG_AODVV2_PRIO (THREAD_PRIORITY_MAIN - 1)
#endif

/**
 * @brief   Control traffic limit
 *
 * @note MUST be a power of 2.
 */
#ifndef CONFIG_AODVV2_CONTROL_TRAFFIC_LIMIT
#define CONFIG_AODVV2_CONTROL_TRAFFIC_LIMIT (16)
#endif

#ifndef CONFIG_AODVV2_MAX_HOPCOUNT
#define CONFIG_AODVV2_MAX_HOPCOUNT (64)
#endif

/**
 * @brief   Active interval value in seconds
 */
#ifndef CONFIG_AODVV2_ACTIVE_INTERVAL
#define CONFIG_AODVV2_ACTIVE_INTERVAL (5)
#endif

#ifndef CONFIG_AODVV2_MAX_IDLETIME
#define CONFIG_AODVV2_MAX_IDLETIME (200)
#endif

#ifndef CONFIG_AODVV2_MAX_BLACKLIST_TIME
#define CONFIG_AODVV2_MAX_BLACKLIST_TIME (200)
#endif

/**
 * @brief   Maximum lifetime for a sequence number in seconds
 */
#ifndef CONFIG_AODVV2_MAX_SEQNUM_LIFETIME
#define CONFIG_AODVV2_MAX_SEQNUM_LIFETIME (300)
#endif

#ifndef CONFIG_AODVV2_RERR_TIMEOUT
#define CONFIG_AODVV2_RERR_TIMEOUT (3)
#endif

#ifndef CONFIG_AODVV2_RTEMSG_ENTRY_TIME
#define CONFIG_AODVV2_RTEMSG_ENTRY_TIME (12)
#endif

#ifndef CONFIG_AODVV2_RREQ_WAIT_TIME
#define CONFIG_AODVV2_RREQ_WAIT_TIME (2)
#endif

#ifndef CONFIG_AODVV2_RREQ_HOLDDOWN_TIME
#define CONFIG_AODVV2_RREQ_HOLDDOWN_TIME (10)
#endif

/**
 * @brief    RREP_Ack sent timeout.
 *
 * This is the timeout (in seconds) for a sent RREP_Ack request, if no reply is
 * received within the timeout the node will be blacklisted.
 */
#ifndef CONFIG_AODVV2_RREP_ACK_SENT_TIMEOUT
#define CONFIG_AODVV2_RREP_ACK_SENT_TIMEOUT (1)
#endif

/**
 * @brief   Packet buffer size.
 *
 * This is the maximum number of entries in the AODVv2 buffered packets set.
 */
#ifndef CONFIG_AODVV2_BUFFER_MAX_ENTRIES
#define CONFIG_AODVV2_BUFFER_MAX_ENTRIES (10)
#endif

/**
 * @brief   Multicast Message Set size.
 *
 * This is the maximum number of entries in the Multicast Message Set.
 */
#ifndef CONFIG_AODVV2_MCMSG_MAX_ENTRIES
#define CONFIG_AODVV2_MCMSG_MAX_ENTRIES (16)
#endif

/**
 * @brief   Router Client Set size.
 *
 * This is the maximum number of entries in the Router Client Set.
 */
#ifndef CONFIG_AODVV2_RCS_MAX_ENTRIES
#define CONFIG_AODVV2_RCS_MAX_ENTRIES (2)
#endif

/**
 * @brief   Local Route Set size
 *
 * This is the maximum number of entries in the Local Route Set.
 */
#ifndef CONFIG_AODVV2_LRS_MAX_ENTRIES
#define CONFIG_AODVV2_LRS_MAX_ENTRIES (16)
#endif

/**
 * @brief   Neighbor Set size
 *
 * This is the maximum number of entries in the Neighbor Set.
 */
#ifndef CONFIG_AODVV2_NEIGH_MAX_ENTRIES
#define CONFIG_AODVV2_NEIGH_MAX_ENTRIES (16)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AODVV2_CONF_H */
/** @} */
