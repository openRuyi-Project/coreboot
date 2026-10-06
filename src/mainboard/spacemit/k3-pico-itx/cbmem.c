/* SPDX-License-Identifier: GPL-2.0-only */
#include <cbmem.h>
#include <mainboard/addressmap.h>

uintptr_t cbmem_top_chipset(void)
{
	/* K3 DDR starts above 4 GiB; never use the generic 4 GiB clamp. */
	return K3_CBMEM_TOP;
}
