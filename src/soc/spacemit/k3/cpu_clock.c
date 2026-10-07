/* SPDX-License-Identifier: GPL-2.0-only */
#include <console/console.h>
#include <delay.h>
#include <device/mmio.h>
#include <soc/addressmap.h>
#include <soc/cpu_clock.h>
#include <timer.h>

#define CPU_CLOCK(cluster) (K3_APMU_BASE + 0x38c + 4 * (cluster))
#define CPU_CHANGE (1U << 12)
#define CPU_MUX_DIV 0x3f
#define CPU1_PLL3 (1U << 13)
#define PLL_ENABLE (1U << 16)
#define PLL_DIV1 (1U << 0)

static int select_clock(unsigned int cluster, uint32_t source)
{
	void *control = (void *)(uintptr_t)CPU_CLOCK(cluster);
	struct stopwatch deadline;
	uint32_t value = read32(control);
	uint32_t mask = CPU_MUX_DIV;
	uint32_t target = source;

	if (value & CPU_CHANGE)
		return -1;
	/* Cluster 1 source 7 uses its own PLL4, not cluster 0's PLL3. */
	if (cluster == 1 && source == 7)
		mask |= CPU1_PLL3;
	if ((value & mask) == target)
		return 0;
	/* Program the mux before issuing its frequency-change request. */
	write32(control, (value & ~mask) | target);
	setbits32(control, CPU_CHANGE);
	stopwatch_init_usecs_expire(&deadline, 3000);
	do {
		value = read32(control);
		if (!(value & CPU_CHANGE))
			return (value & mask) == target ? 0 : -1;
		udelay(5);
	} while (!stopwatch_expired(&deadline));
	return -1;
}

static int boot_clock(void)
{
	/* Both clusters can run from the retained 2457.6 MHz PLL1 / 3. */
	if (read32((void *)(K3_APBS_BASE + 0x100)) != 0x0b330ccc ||
	    (read32((void *)(K3_APBS_BASE + 0x104)) & (PLL_ENABLE | 0xff00)) !=
	    (PLL_ENABLE | 0xcd00) ||
	    read32((void *)(K3_APBS_BASE + 0x108)) != 0xa0558989 ||
	    !(read32((void *)(K3_MPMU_BASE + 0x10)) & (1U << 24)))
		return -1;
	setbits32((void *)(K3_APBS_BASE + 0x104), 1U << 2);
	/* Try both clusters even if one still has a pending transition. */
	int rc = select_clock(0, 0);
	int rc1 = select_clock(1, 0);
	return rc || rc1 ? -1 : 0;
}

static int set_cpu_pll(unsigned int cluster)
{
	uintptr_t pll = K3_APBS_BASE + 0x124 + 0xc * cluster;
	struct stopwatch deadline;
	uint32_t control;

	control = read32((void *)(pll + 4));
	write32((void *)(pll + 4), control & ~PLL_ENABLE);
	/* PLL3 and PLL4 use the same 2.4 GHz integer configuration. */
	write32((void *)pll, 0x0b320000);
	write32((void *)(pll + 8), 0xa0558989);
	control = (control & ~(0xff00U | PLL_ENABLE)) | PLL_DIV1;
	write32((void *)(pll + 4), control | PLL_ENABLE);
	stopwatch_init_usecs_expire(&deadline, 3000);
	while (!(read32((void *)(K3_MPMU_BASE + 0x10)) & (1U << (26 + cluster)))) {
		if (stopwatch_expired(&deadline))
			return -1;
		udelay(5);
	}
	if (read32((void *)pll) != 0x0b320000 ||
	    read32((void *)(pll + 8)) != 0xa0558989 ||
	    (read32((void *)(pll + 4)) & (0xff00U | PLL_ENABLE | PLL_DIV1)) !=
	    (PLL_ENABLE | PLL_DIV1))
		return -1;
	return 0;
}

static int max_clock(void)
{
	/* Both clusters must use PLL1 before reprogramming either CPU PLL.
	 * The board must establish the 1.0 V X100 operating point first.
	 */
	for (unsigned int cluster = 0; cluster < 2; cluster++) {
		if (read32((void *)(uintptr_t)CPU_CLOCK(cluster)) &
		    (CPU_MUX_DIV | CPU_CHANGE))
			return -1;
	}
	for (unsigned int cluster = 0; cluster < 2; cluster++) {
		if (set_cpu_pll(cluster))
			return -1;
	}
	for (unsigned int cluster = 0; cluster < 2; cluster++) {
		if (select_clock(cluster, 7))
			return -1;
	}
	return 0;
}

void k3_x100_init(void)
{
	/* Establish a safe clock before changing voltage or either CPU PLL. */
	if (boot_clock() || mainboard_set_cpu_voltage(1000000) || max_clock()) {
		printk(BIOS_ERR, "K3 X100: maximum operating point unavailable\n");
		if (boot_clock())
			die("K3 X100: cannot restore both clusters to the boot clock\n");
		return;
	}
	printk(BIOS_INFO, "K3 X100: both clusters at 2400 MHz, VDD_CPU 1000 mV\n");
}
