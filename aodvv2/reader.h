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

#ifndef AODVV2_READER_H
#define AODVV2_READER_H

#include <string.h>
#include <stdio.h>

#include "rfc5444/rfc5444_reader.h"

#include "aodvv2/aodvv2.h"

#include "constants.h"
#include "seqnum.h"
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Initialize reader.
 */
void aodvv2_packet_reader_init(void);

/**
 * @brief   Clean up after reader. Only needs to be called upon shutdown.
 */
void aodvv2_packet_reader_cleanup(void);

/**
 * @brief   Read data buffer as a RFC 5444 packet and handle the data it
 *          contains.
 *
 * @param[in] buffer Data to be read and handled.
 * @param[in] length Length of data.
 * @param[in] sender Address of the node from which the packet was received.
 */
int aodvv2_packet_reader_handle_packet(void *buffer,
                                       size_t length,
                                       struct netaddr *sender);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AODVV2_READER_H */

/** @} */
