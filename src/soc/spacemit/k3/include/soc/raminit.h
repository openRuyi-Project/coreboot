/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SOC_SPACEMIT_K3_RAMINIT_H
#define SOC_SPACEMIT_K3_RAMINIT_H
#include <types.h>
uint64_t k3_dram_init(const char *fallback_part, const uint8_t *eeprom, size_t size);
uint64_t k3_dram_initialize(const char *fallback_part, const uint8_t *eeprom, size_t size);
void mainboard_restore_dram_training(void *record, size_t size);
#endif
