/*
 * Copyright (C) 2020 Locha Inc
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     peripherals
 * @{
 *
 * @file
 * @brief       Fuel gauge BQ27441
 *
 * @author      Luis Ruiz <luisan00@hotmail.com>
 */
#ifndef BQ27441_H
#define BQ27441_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

uint8_t bq27441_address = 0x55;

/**
 * @brief 
 */
enum command
{
    control = 0x00,
    temp = 0x02,
    voltage = 0x04,
    flags = 0x06,
    nom_capacity = 0x08,
    avail_capacity = 0x0A,
    rem_capacity = 0x0C,
    full_capacity = 0x0E,
    avg_current = 0x10,
    stdby_current = 0x12,
    max_current = 0x14,
    avg_power = 0x18,
    soc = 0x1C,
    int_temp = 0x1E,
    soh = 0x20,
    rem_cap_unfl = 0x28,
    rem_cap_fil = 0x2A,
    full_cap_unfl = 0x2C,
    full_cap_fil = 0x2E,
    soc_unfl = 0x30
};

/**
 * @brief Get the control object
 * 
 * @return uint16_t 
 */
uint16_t get_control(void);
uint16_t get_temp(void);
uint16_t get_voltage(void);
uint16_t get_nom_capacity(void);
uint16_t get_avail_capacity(void);
uint16_t get_rem_capacity(void);
uint16_t get_full_capacity(void);
uint16_t get_avg_current(void);
uint16_t get_stdby_current(void);
uint16_t get_max_current(void);
uint16_t get_avg_power(void);
uint16_t get_soc(void);
uint16_t get_int_temp(void);
uint16_t get_soh(void);
uint16_t get_rem_cap_unfl(void);
uint16_t get_rem_cap_fil(void);
uint16_t get_full_cap_unfl(void);
uint16_t get_full_cap_fil(void);
uint16_t get_soc_unfl(void);






#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* BQ27441_H */
       /** @} */