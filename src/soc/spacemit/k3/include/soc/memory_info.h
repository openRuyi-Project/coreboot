/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SOC_SPACEMIT_K3_MEMORY_INFO_H
#define SOC_SPACEMIT_K3_MEMORY_INFO_H

#include <types.h>

/* Payload memory inventory in the reserved part of the DDR training header. */
#define K3_MEMORY_INFO_MAGIC 0x494d334b /* K3MI */
#define K3_MEMORY_INFO_VERSION 1

struct k3_memory_info {
	uint32_t magic;
	uint32_t crc32;
	uint32_t version;
	uint32_t type; /* DDR_TYPE_LPDDR4X = 0, DDR_TYPE_LPDDR5 = 1 */
	uint32_t chip_selects;
	uint32_t data_rate_mtps;
	uint32_t device_count;
	uint32_t device_size_mib;
	char part_number[32];
};

_Static_assert(sizeof(struct k3_memory_info) == 64, "K3 memory inventory layout");

#endif
