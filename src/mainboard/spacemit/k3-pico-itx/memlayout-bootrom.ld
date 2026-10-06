/* SPDX-License-Identifier: GPL-2.0-only */
#include <memlayout.h>
#include <arch/header.ld>
#include <mainboard/addressmap.h>
SECTIONS
{
	/* BootROM places the executable body after its 4 KiB container prefix. */
	BOOTBLOCK(0xc0801000, 384K)
	PRERAM_CBMEM_CONSOLE(0xc0870000, 16K)
	FMAP_CACHE(0xc0874000, 4K)
	CBFS_MCACHE(0xc0875000, 16K)
	STACK(0xc087f000, 4K)
	/* The native loader initializes DDR before using these regions. */
	DRAM_START(K3_DRAM_BASE)
	OPENSBI(K3_DRAM_BASE, 2M)
	ROMSTAGE(K3_COREBOOT_BASE + 128K, 256K)
	RAMSTAGE(K3_COREBOOT_BASE + 2M, 2M)
	REGION(flash, CONFIG_K3_CBFS_BASE, CONFIG_ROM_SIZE, 0)
}
