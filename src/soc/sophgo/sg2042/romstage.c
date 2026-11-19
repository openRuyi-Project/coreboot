// SPDX-License-Identifier: GPL-2.0-only
#include <cbmem.h>
#include <console/console.h>
#include <device/mmio.h>
#include <program_loading.h>
#include <soc/addressmap.h>
#include <romstage_common.h>

void __noreturn romstage_main(void)
{
	/* The memory map and hart stacks cover one socket. */
	if (read32((void *)SG2042_TOP_CONF_INFO) & BIT(15))
		die("SG2042 dual-socket boot is unsupported");

	cbmem_initialize_empty();
	run_ramstage();
}
