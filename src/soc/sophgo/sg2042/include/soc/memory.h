/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SOC_SOPHGO_SG2042_MEMORY_H
#define SOC_SOPHGO_SG2042_MEMORY_H
#include <types.h>

/* Four DDR channels and at most one split at the PCIe aperture. */
#define SG2042_MAX_RANGES 5
struct sg2042_range {
	uint64_t base;
	uint64_t size;
	unsigned int channel;
};
unsigned int sg2042_memory_map(struct sg2042_range ranges[SG2042_MAX_RANGES]);
#endif
