/* SPDX-License-Identifier: GPL-2.0-only */
#include <arch/encoding.h>
#include <device/mmio.h>
#include <mcall.h>
#include <timer.h>
#include <soc/addressmap.h>

void mtime_init(void)
{
	unsigned int hart = read_csr(mhartid);
	HLS()->time = (uint64_t *)(K3_CLINT_BASE + 0xbff8);
	HLS()->timecmp = (uint64_t *)(uintptr_t)(K3_CLINT_BASE + 0x4000 + 8 * hart);
}

void set_msip(int hart, int value)
{
	if (hart >= 0 && hart < CONFIG_MAX_CPUS)
		write32((void *)(K3_CLINT_BASE + 4 * (uintptr_t)hart), !!value);
}

void timer_monotonic_get(struct mono_time *mt)
{
	/* Match the retained SPL's RV64 timer path and its 24 MHz timebase.
	 * TIME advances on this board; split CLINT MTIME reads did not provide
	 * a usable monotonic counter during the initial RAM boot.
	 */
	mono_time_set_usecs(mt, rdtime() / 24);
}
