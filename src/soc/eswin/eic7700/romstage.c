// SPDX-License-Identifier: GPL-2.0-only

#include <cbmem.h>
#include <commonlib/helpers.h>
#include <console/console.h>
#include <program_loading.h>
#include <romstage_common.h>
#include <soc/sdram.h>

void __noreturn romstage_main(void)
{
	printk(BIOS_INFO, "SCPU initialized %llu MiB DRAM\n", eic7700_dram_size() / MiB);
	cbmem_initialize_empty();
	run_ramstage();
}
