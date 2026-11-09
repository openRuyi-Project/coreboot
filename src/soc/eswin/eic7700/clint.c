// SPDX-License-Identifier: GPL-2.0-only

#include <device/mmio.h>
#include <mcall.h>
#include <soc/addressmap.h>

void mtime_init(void)
{
	unsigned long hart = read_csr(mhartid);

	HLS()->time = (void *)(EIC7700_CLINT + 0xbff8);
	HLS()->timecmp = (void *)(EIC7700_CLINT + 0x4000 + hart * 8);
}

void set_msip(int hart, int value)
{
	write32((void *)(EIC7700_CLINT + (uintptr_t)hart * 4), !!value);
}
