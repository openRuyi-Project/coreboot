// SPDX-License-Identifier: GPL-2.0-only

#include <cbmem.h>
#include <soc/addressmap.h>
#include <soc/sdram.h>

uintptr_t cbmem_top_chipset(void)
{
	return EIC7700_DRAM + (CONFIG(SOC_ESWIN_EIC7702) ? EIC7702_DIRECT_SIZE :
			       eic7700_dram_size(0));
}
