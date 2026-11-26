// SPDX-License-Identifier: GPL-2.0-only

#include <cbmem.h>
#include <commonlib/helpers.h>
#include <console/console.h>
#include <program_loading.h>
#include <romstage_common.h>
#include <soc/addressmap.h>
#include <soc/sdram.h>

void __noreturn romstage_main(void)
{
	for (unsigned int i = 0; i < EIC770X_DIES; i++)
		printk(BIOS_INFO, "Die %u: %llu MiB DRAM\n", i, eic7700_dram_size(i) / MiB);
	cbmem_initialize_empty();
	run_ramstage();
}
