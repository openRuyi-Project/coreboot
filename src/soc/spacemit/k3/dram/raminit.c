/* SPDX-License-Identifier: GPL-2.0-only */
#include <arch/encoding.h>
#include <soc/addressmap.h>
#include <soc/raminit.h>
#include "k3_ddr.h"

#define K3_L1_CACHES ((1UL << 0) | (1UL << 1))
#define K3_BOOT_HART_SNOOP (1UL << 0)

uint64_t k3_dram_initialize(const char *fallback_part, const uint8_t *eeprom, size_t size)
{
	/* Match SPL's audio SRAM PMA before decompressing the PHY firmware. */
	clear_csr(0x7de, 0xffUL << 48);
	set_csr(0x7de, 0x20UL << 48);
	asm volatile("sfence.vma zero, zero" ::: "memory");

	/* The board restores the boot medium record, except for USB/SD boots. */
	memset((void *)DDR_TRAINING_INFO_BUFF, 0, 128);
	mainboard_restore_dram_training((void *)DDR_TRAINING_INFO_BUFF,
				       sizeof(struct ddr_info_t));
	uint64_t trained_bytes = k3_dram_init(fallback_part, eeprom, size);
	write_csr(0x7e8, 0x1100000000ULL >> 2);
	asm volatile("sfence.vma zero, zero" ::: "memory");
	/* X100 reset leaves L1 caches off. DDR is now ordinary memory, but
	 * AMOs fault until cache/snoop are enabled. Match the retained X100
	 * sequence: I/D caches first, then the boot-hart L2 snoop bit. Other
	 * harts are still parked; OpenSBI owns their reset-entry setup.
	 */
	set_csr(0x7c0, K3_L1_CACHES);
	set_csr(0x7f0, K3_BOOT_HART_SNOOP);
	asm volatile("fence iorw, iorw\nfence.i" ::: "memory");
	if ((read_csr(0x7c0) & K3_L1_CACHES) != K3_L1_CACHES ||
	    !(read_csr(0x7f0) & K3_BOOT_HART_SNOOP))
		die("K3 boot-hart cache/snoop enable failed\n");
	printk(BIOS_INFO, "K3 X100: boot-hart I/D cache and L2 snoop enabled\n");
	return trained_bytes;
}
