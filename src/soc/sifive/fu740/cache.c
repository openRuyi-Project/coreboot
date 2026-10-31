/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/cache.h>
#include <arch/encoding.h>
#include <arch/smp/atomic.h>
#include <console/console.h>
#include <device/mmio.h>
#include <timer.h>

#define CCACHE_CONFIG ((void *)0x02010000)
#define CCACHE_WAY_ENABLE ((void *)0x02010008)

static atomic_t dram_harts;

/* Called by every hart with its ramstage stack and trap vector in DRAM. */
void exit_car(void)
{
	atomic_inc(&dram_harts);
	if (read_csr(mhartid) != CONFIG_RISCV_WORKING_HARTID)
		return;

	struct stopwatch timeout;
	stopwatch_init_msecs_expire(&timeout, 100);
	while (atomic_read(&dram_harts) != CONFIG_MAX_CPUS) {
		if (stopwatch_expired(&timeout))
			die("FU740: harts did not leave L2 LIM");
	}

	/* Enabling cache ways removes their L2 LIM backing. None of the
	 * harts may still fetch instructions or use a stack there. */
	unsigned int ways = (read32(CCACHE_CONFIG) >> 8) & 0xff;
	if (!ways)
		die("FU740: invalid L2 cache configuration");
	write32(CCACHE_WAY_ENABLE, ways - 1);
	asm volatile("fence rw, rw" ::: "memory");
}
