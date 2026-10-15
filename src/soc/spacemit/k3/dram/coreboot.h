/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef K3_DDR_COREBOOT_H
#define K3_DDR_COREBOOT_H

#include <types.h>
#include <soc/memory_info.h>
#include <commonlib/helpers.h>
#include <commonlib/endian.h>
#include <console/console.h>
#include <delay.h>
#include <device/mmio.h>
#include <halt.h>
#include <string.h>
#include <timer.h>
#include "lzo.h"

#define pr_info(...) printk(BIOS_INFO, __VA_ARGS__)
#define pr_err(...) printk(BIOS_ERR, __VA_ARGS__)
#define printf(...) printk(BIOS_INFO, __VA_ARGS__)
#define CONFIG_SYS_SDRAM_BASE 0x100000000ULL
#define DDR_TRAINING_INFO_BUFF 0xc08d0000
#define DDR_TRAINING_INFO_MAGIC 0x54524444

struct ddr_info_t {
	uint32_t magic;
	uint32_t crc32;
	uint64_t chipid;
	uint64_t mac_addr;
	uint32_t version;
	uint32_t type;
	uint32_t cs_num;
	uint32_t data_rate;
	struct k3_memory_info memory_info;
	uint8_t reserved[128 - 40 - sizeof(struct k3_memory_info)];
	uint8_t training_info[0x9800 - 128];
};

_Static_assert(offsetof(struct ddr_info_t, training_info) == 128, "DDR training offset");
_Static_assert(sizeof(struct ddr_info_t) == 0x9800, "DDR training record size");

typedef uintptr_t fdt_addr_t;
typedef uint64_t fdt_size_t;

#define readl(addr) read32(addr)
#define writel(value, addr) write32(addr, value)
#define get_unaligned_be16(addr) read_be16(addr)
#define get_unaligned_be32(addr) read_be32(addr)
#define get_unaligned_le16(addr) read_le16(addr)
#define get_unaligned(addr) read_le32(addr)
#define put_unaligned(value, addr) write_le32(addr, value)

static inline unsigned int fls(unsigned int x)
{
	return x ? 32 - __builtin_clz(x) : 0;
}
#ifndef is_power_of_2
#define is_power_of_2(x) ((x) && !((x) & ((x) - 1)))
#endif

/* U-Boot's reflected IEEE CRC-32, used by the part table and TLV records. */
static inline uint32_t crc32(uint32_t crc, const uint8_t *data, size_t size)
{
	crc = ~crc;
	while (size--) {
		crc ^= *data++;
		for (unsigned int bit = 0; bit < 8; bit++)
			crc = (crc >> 1) ^ (0xedb88320U & -(crc & 1));
	}
	return ~crc;
}

static inline unsigned long get_timer(unsigned long base)
{
	struct mono_time now, zero;
	mono_time_set_usecs(&zero, 0);
	timer_monotonic_get(&now);
	return (mono_time_diff_microseconds(&zero, &now) / 1000) - base;
}

#endif
