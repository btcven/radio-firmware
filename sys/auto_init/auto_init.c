/*
 * Copyright (C) 2020 Locha Inc
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 *
 * @file
 * @brief       Auto-initialization of modules
 *
 * @author      Locha Mesh Developers <contact@locha.io>
 * @}
 */

#include "kernel_defines.h"

void radio_firmware_auto_init(void)
{
    if (IS_USED(MODULE_AUTO_INIT_RFC5444)) {
        extern void rfc5444_auto_init(void);
        rfc5444_auto_init();
    }

    if (IS_USED(MODULE_AUTO_INIT_AODVV2)) {
        extern void aodvv2_auto_init(void);
        aodvv2_auto_init();
    }
}
