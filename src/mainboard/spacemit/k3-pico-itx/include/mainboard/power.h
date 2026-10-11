/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MAINBOARD_POWER_H
#define MAINBOARD_POWER_H

#include <types.h>

void pico_itx_parse_power_config(const uint8_t *eeprom, size_t size);
int pico_itx_power_init(void);

#endif
