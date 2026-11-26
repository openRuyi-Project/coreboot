/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SOC_ESWIN_EIC7700_SDRAM_H
#define SOC_ESWIN_EIC7700_SDRAM_H
#include <stdint.h>
uint64_t eic7700_dram_size(unsigned int die_id);
struct eic770x_bank {
	uint64_t base;
	uint64_t size;
};

unsigned int eic770x_memory_map(struct eic770x_bank banks[4]);
#endif
