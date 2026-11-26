// SPDX-License-Identifier: GPL-2.0-only

#include <device/mmio.h>
#include <mcall.h>
#include <soc/addressmap.h>

/* Each die has a four-hart CLINT, indexed by the hart number on that die. */
#define HARTS_PER_DIE 4

static uintptr_t clint_base(unsigned long hart)
{
	return EIC7700_CLINT + hart / HARTS_PER_DIE * EIC770X_DIE_MMIO_STRIDE;
}

void mtime_init(void)
{
	unsigned long hart = read_csr(mhartid);
	uintptr_t base = clint_base(hart);

	HLS()->time = (void *)(base + 0xbff8);
	HLS()->timecmp = (void *)(base + 0x4000 + (hart % HARTS_PER_DIE) * 8);
}

void set_msip(int hart, int value)
{
	write32((void *)(clint_base(hart) + (hart % HARTS_PER_DIE) * 4), !!value);
}
