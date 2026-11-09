// SPDX-License-Identifier: GPL-2.0-only

#include <cbmem.h>
#include <soc/addressmap.h>
#include <soc/sdram.h>

uintptr_t cbmem_top_chipset(void)
{
	return EIC7700_DRAM + eic7700_dram_size();
}
