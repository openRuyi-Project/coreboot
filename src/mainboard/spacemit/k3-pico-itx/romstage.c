/* SPDX-License-Identifier: GPL-2.0-only */
#include <cbmem.h>
#include <program_loading.h>
#include <romstage_common.h>

void __noreturn romstage_main(void)
{
	/* DDR is ready from the native bootblock. */
	cbmem_initialize_empty();
	run_ramstage();
}
