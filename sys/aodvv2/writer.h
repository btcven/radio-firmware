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
 */

#ifndef AODVV2_WRITER_H
#define AODVV2_WRITER_H

#include "mutex.h"

#include "common/netaddr.h"
#include "rfc5444/rfc5444_writer.h"

#include "aodvv2/aodvv2.h"
#include "constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Wrapper for the rfc5444_writer_target that the _write_packet()
 *          callback receives.
 *
 * _write_packet() needs to know the type, payload and target address
 * of the RFC5444 message to be sent as well, but the oonf api does not
 * offer this feature. Having this wrapper enables the use of the
 * container_of macro to fetch this information.
 * It is hacky, but it does the trick.
 */
typedef struct {
    struct rfc5444_writer_target interface; /**< Interface for generating rfc5444 packets */
    struct netaddr target_addr; /**< Address to which the packet should be sent */
    aodvv2_packet_data_t packet_data; /**< Payload of the AODVv2 Message */
    int type; /**< Type of the AODVv2 Message (i.e. rfc5444_msg_type) */
} aodvv2_writer_target_t;

/**
 * @brief   oonf api voodo. Pointer to a callback function which is passed to
 *          writer_init() and called when the packet is ready to send.
 */
typedef void (* write_packet_func_ptr)(struct rfc5444_writer *wr,
                                       struct rfc5444_writer_target *iface,
                                       void *buffer,
                                       size_t length);

/**
 * @brief   Initialize RFC5444 writer
 * @param[in] ptr pointer to "send_packet" callback
 */
void aodvv2_packet_writer_init(write_packet_func_ptr ptr);

/**
 * @brief   Send a RREQ.
 *
 * @note    DO NOT use this function to dispatch packets from anything else than
 *          the sender_thread.
 *
 * @pre (@p packet_data != NULL) && (@p next_hop != NULL)
 *
 * @param[in] packet_data Parameters of the RREQ
 * @param[in] next_hop    Address the RREP is sent to
 */
void aodvv2_packet_writer_send_rreq(aodvv2_packet_data_t *packet_data,
                                    struct netaddr *next_hop);

/**
 * @brief   Send a RREP.
 *
 * @note    DO NOT use this function to dispatch packets from anything else than
 *          the sender_thread.
 *
 * @pre (@p packet_data != NULL) && (@p next_hop != NULLL)
 *
 * @param[in] packet_data Parameters of the RREP
 * @param[in] next_hop    Address the RREP is sent to
 */
void aodvv2_packet_writer_send_rrep(aodvv2_packet_data_t *packet_data,
                                    struct netaddr *next_hop);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AODVV2_WRITER_H */
