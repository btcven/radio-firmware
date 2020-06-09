/*
 * Copyright (C) 2020 Locha Inc
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     sys/peripherals
 * @{
 *
 * @file
 * @brief       fuel gauge BQ27441
 *
 * @author      Locha Mesh Developers <developers@locha.io>
 *
 * @}
 */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "periph_conf.h"
#include "periph/gpio.h"
#include "periph/i2c.h"
#include "shell.h"

#include <xtimer.h>
#include "peripherals/bq27441.h"

i2c_t dev = 0;

uint16_t get_control(void) {
    
}
