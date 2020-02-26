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

#ifndef AODVV2_SEQNUM_H
#define AODVV2_SEQNUM_H

#include "aodvv2/aodvv2.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Initializ SeqNum.
 * @note Must be called only once during program initialization.
 */
void aodvv2_seqnum_init(void);

/**
 * @brief   Increment the SeqNum.
 */
void aodvv2_seqnum_inc(void);

/**
 * @brief   Get the SeqNum.
 */
aodvv2_seqnum_t aodvv2_seqnum_get(void);

/**
 * @brief   Compare 2 sequence numbers.
 *
 * @param[in] s1 First sequence number
 * @param[in] s2 Second sequence number
 *
 * @return -1, s1 is smaller.
 * @return  0, s1 and s2 are equal.
 * @return  1, s1 is bigger.
 */
static inline int aodvv2_seqnum_cmp(aodvv2_seqnum_t s1, aodvv2_seqnum_t s2)
{
    return s1 == s2 ? 0 : (s1 > s2 ? +1 : -1);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AODVV2_SEQNUM_H */
/** @} */
