// SPDX-License-Identifier: GPL-2.0-only
#include <arch/encoding.h>
#include <device/mmio.h>
#include <mcall.h>
#include <soc/addressmap.h>
#include <timer.h>

void timer_monotonic_get(struct mono_time *mt)
{
	mono_time_set_usecs(mt, rdtime() / 50);
}

void mtime_init(void)
{
	unsigned int hart = read_csr(mhartid);

	/* Each four-hart cluster has its own MTIMECMP bank. */
	HLS()->timecmp = (uint64_t *)(SG2042_MTIMER + (hart / 4) * 0x10000 + (hart % 4) * 8);
}

void set_msip(int hart, int value)
{
	write32((void *)(SG2042_MSWI + (uintptr_t)hart * 4), !!value);

	/* Drain the previous stage's wakeup before this hart parks again. */
	if (!value && read_csr(mhartid) == (uintptr_t)hart) {
		while (read_csr(mip) & MIP_MSIP)
			;
	}
}
