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
 * @}
 */

#include "seqnum.h"

static aodvv2_seqnum_t seqnum;

void aodvv2_seqnum_init(void)
{
    seqnum = 1;
}

void aodvv2_seqnum_inc(void)
{
    /* The Sequence Number wraps at 65535 */
    if (seqnum == 65535) {
        seqnum = 1;
    }
    else if (seqnum == 0) {
        seqnum = 1;
    }
    else {
        seqnum += 1;
    }
}

aodvv2_seqnum_t aodvv2_seqnum_get(void)
{
    return seqnum;
}
