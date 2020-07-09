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
 * @ingroup     net_aodvv2
 * @{
 *
 * @file
 * @brief       AODVVv2 Message Writer
 *
 * @author      Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 * @author      Gustavo Grisales <gustavosinbandera1@hotmail.com>
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 */

#ifndef PRIV_AODVV2_WRITER_H
#define PRIV_AODVV2_WRITER_H

#include "net/rfc5444.h"
#include "net/aodvv2/msg.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Register AODVv2 message writer
 *
 * @param[in] writer The RFC 5444 writer.
 *
 * @return 0 on success.
 * @return -ENOMEM if failed to allocate necessary RFC 5444 structures.
 */
int _aodvv2_writer_init(void);

/**
 * @brief   Send a RREQ
 *
 * @note This message will be sent over all targets.
 *
 * @pre @p rreq != NULL
 *
 * @param[in]  rreq The RREQ message data.
 *
 * @return 0 on success, otherwise 0< on failure.
 */
int _aodvv2_writer_send_rreq(aodvv2_msg_rreq_t *rreq);

/**
 * @brief   Send a RREP
 *
 * @pre @p rreq != NULL
 *
 * @param[in]  rrep  The RREP message data.
 * @param[in]  dst   Destination IPv6 address.
 * @param[in]  iface Network interface to send the message over.
 *
 * @return 0 on success, otherwise 0< on failure.
 */
int _aodvv2_writer_send_rrep(aodvv2_msg_rrep_t *rrep, const ipv6_addr_t *dst,
                             uint16_t iface);

/**
 * @brief   Send a RREP_Ack
 *
 * @pre @p rrep_ack != NULL
 *
 * @param[in]  rrep_ack The RREP_Ack message data.
 * @param[in]  dst      Destination IPv6 address.
 * @param[in]  iface    Network interface to send the message over.
 *
 * @return 0 on success, otherwise 0< on failure.
 */
int _aodvv2_writer_send_rrep_ack(aodvv2_msg_rrep_ack_t *rrep_ack,
                                 const ipv6_addr_t *dst, uint16_t iface);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PRIV_AODVV2_WRITER_H */
/** @} */
