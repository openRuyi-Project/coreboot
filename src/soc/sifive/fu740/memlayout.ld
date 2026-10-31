/* SPDX-License-Identifier: GPL-2.0-only */

#include <memlayout.h>
#include <soc/addressmap.h>

#include <arch/header.ld>

SECTIONS
{
	SRAM_START(FU740_L2LIM)
	/* The 64K size is not allocated. It's just for basic size checking. */
	BOOTBLOCK(FU740_L2LIM, 64K)
	/* Each of the five harts needs one page for its stack and HLS. */
	CAR_STACK(FU740_L2LIM + 64K, 20K)
	PRERAM_CBMEM_CONSOLE(FU740_L2LIM + 84K, 8K)
	CBFS_MCACHE(FU740_L2LIM + 92K, 8K)
	FMAP_CACHE(FU740_L2LIM + 100K, 2K)
	PRERAM_CBFS_CACHE(FU740_L2LIM + 102K, 128K)
	SRAM_END(FU740_L2LIM + 2M)

	DRAM_START(FU740_DRAM)
	OPENSBI(FU740_DRAM, 2M)
	/* The 256K size is not allocated. It's just for basic size checking. */
	RAMSTAGE(FU740_DRAM + 2M, 256K)
	MEM_STACK(FU740_DRAM + 2M + 256K, 20K)
	POSTRAM_CBFS_CACHE(FU740_DRAM + 2M + 276K, 30M - 276K)
}
