/*
 * Copyright (C) 2020 Locha Inc
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup         boards_cc1312_launchpad
 * @{
 *
 * @file
 * @brief           CC1312 Radio configuration
 *
 * @author          Jean Pierre Dudey <jeandudey@hotmail.com>
 */

#ifndef RF_CONF_H
#define RF_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief   Creates a TX power entry for the default PA.
 *
 *          The values for @a bias, @a gain, @a boost and @a coefficient are
 *          usually measured by Texas Instruments for a specific front-end
 *          configuration. They can then be obtained from SmartRF Studio.
 */
#define DEFAULT_PA_ENTRY(bias, gain, boost, coefficient) \
        ((bias) << 0) | ((gain) << 6) | ((boost) << 8) | ((coefficient) << 9)

/**
 * @brief   TX Power dBm lookup table
 */
typedef struct output_config {
    int      dbm; /**< dBm output power */
    uint16_t value; /**< The encoded value */
} output_config_t;

/**
 * @brief     TX Power dBm lookup table.
 *
 *            This table is auto generated from Smart RF Studio.
 */
static const output_config_t output_power_table[] = {
  // The original PA value (12.5 dBm) has been rounded to an integer value.
  {13, DEFAULT_PA_ENTRY(36, 0, 0, 89) },
  {12, DEFAULT_PA_ENTRY(16, 0, 0, 82) },
  {11, DEFAULT_PA_ENTRY(26, 2, 0, 51) },
  {10, DEFAULT_PA_ENTRY(18, 2, 0, 31) },
  {9, DEFAULT_PA_ENTRY(28, 3, 0, 31) },
  {8, DEFAULT_PA_ENTRY(24, 3, 0, 22) },
  {7, DEFAULT_PA_ENTRY(20, 3, 0, 19) },
  {6, DEFAULT_PA_ENTRY(17, 3, 0, 16) },
  {5, DEFAULT_PA_ENTRY(14, 3, 0, 14) },
  {4, DEFAULT_PA_ENTRY(13, 3, 0, 11) },
  {3, DEFAULT_PA_ENTRY(11, 3, 0, 10) },
  {2, DEFAULT_PA_ENTRY(10, 3, 0, 9) },
  {1, DEFAULT_PA_ENTRY(9, 3, 0, 9) },
  {0, DEFAULT_PA_ENTRY(8, 3, 0, 8) },
  {-5, DEFAULT_PA_ENTRY(4, 3, 0, 5) },
  {-10, DEFAULT_PA_ENTRY(2, 3, 0, 5) },
  {-15, DEFAULT_PA_ENTRY(1, 3, 0, 3) },
  {-20, DEFAULT_PA_ENTRY(0, 3, 0, 2) },
};

/**
 * @brief   Number of entries in the output power table.
 */
#define OUTPUT_CONFIG_COUNT \
    (sizeof(output_power_table) / sizeof(output_power_table[0]))

/**
 * @brief   Minimum output power.
 */
#define OUTPUT_POWER_MIN (output_power_table[OUTPUT_CONFIG_COUNT - 1].dbm)

/**
 * @brief   Maximum output power.
 */
#define OUTPUT_POWER_MAX (output_power_table[0].dbm)

/**
 * @brief   Unknown (default) output power.
 */
#define OUTPUT_POWER_UNKNOWN 0xFFFF

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* RF_CONF_H */

/** @} */
