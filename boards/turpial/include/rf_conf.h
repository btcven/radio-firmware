/*
 * Copyright (C) 2020 Locha Inc
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup         boards_turpial
 * @{
 *
 * @file
 * @brief           Peripheral MCU configuration for Locha Mesh Turpial
 *
 * @author          Jean Pierre Dudey <jeandudey@hotmail.com>
 */

#ifndef RF_CONF_H
#define RF_CONF_H

#include "kernel_defines.h"
#include "cc26x2_cc13x2_rf_params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name   CC26x2/CC13x2 PA table configuration
 * @{
 */
static cc26x2_cc13x2_rf_pa_t cc26x2_cc13x2_rf_patable[] =
{
    { .dbm = 26, .val = 0x00CB },
    { .dbm = 25, .val = 0x00C6 },
    { .dbm = 24, .val = 0x00C4 },
    { .dbm = 23, .val = 0x00C3 },
    { .dbm = 21, .val = 0x00C2 },
    { .dbm = 17, .val = 0x00C1 },
    { .dbm = 11, .val = 0x00C0 },
};
#define CC26X2_CC13X2_PA_TABLE_NUMOF ARRAY_SIZE(cc26x2_cc13x2_rf_patable)
/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RF_CONF_H */
