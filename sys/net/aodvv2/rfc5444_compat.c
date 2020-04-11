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
 * @brief       RFC5444 Utils
 *
 * @author      Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 * @author      Gustavo Grisales <gustavosinbandera1@hotmail.com>
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 * @}
 */

#include "net/aodvv2/aodvv2.h"
#include "net/aodvv2/rfc5444.h"

#include <assert.h>

void ipv6_addr_to_netaddr(ipv6_addr_t *src, struct netaddr *dst)
{
    assert(src != NULL && dst != NULL);

    dst->_type = AF_INET6;
    dst->_prefix_len = AODVV2_PREFIX_LEN;
    memcpy(dst->_addr, src, sizeof(dst->_addr));
}

void netaddr_to_ipv6_addr(struct netaddr *src, ipv6_addr_t *dst)
{
    assert(src != NULL && dst != NULL);

    memcpy(dst, src->_addr, sizeof(uint8_t) * NETADDR_MAX_LENGTH);
}
