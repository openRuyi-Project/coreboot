/* SPDX-License-Identifier: GPL-2.0-only */
#include <delay.h>
#include <device/mmio.h>
#include <soc/addressmap.h>
#include <soc/system_clock.h>
#include <timer.h>

#define PLL1_SWCR1 (K3_APBS_BASE + 0x100)
#define PLL6_SWCR1 (K3_APBS_BASE + 0x148)
#define PLL_STATUS (K3_MPMU_BASE + 0x10)
#define PLL_ENABLE (1U << 16)
#define PLL_DIV2 (1U << 1)
#define PLL_DIV4 (1U << 3)
#define PLL_DIV8 (1U << 7)
#define ACGR (K3_MPMU_BASE + 0x1024)
#define APB_CLOCK (K3_MPMU_BASE + 0x1050)
#define TOP_DCLK (K3_APMU_BASE + 0x158)
#define CCI_CLOCK (K3_APMU_BASE + 0x300)
#define AXI_CLOCK (K3_APMU_BASE + 0x388)

static int change_clock(uintptr_t address, uint32_t mask, uint32_t target,
			uint32_t request)
{
	void *reg = (void *)address;
	struct stopwatch deadline;
	uint32_t value = read32(reg);

	if (value & request)
		return -1;
	if ((value & mask) == target)
		return 0;
	/* Latch the source/divider before requesting the clock transition. */
	write32(reg, (value & ~mask) | target);
	setbits32(reg, request);
	stopwatch_init_usecs_expire(&deadline, 3000);
	do {
		value = read32(reg);
		if (!(value & request))
			return (value & mask) == target ? 0 : -1;
		udelay(5);
	} while (!stopwatch_expired(&deadline));
	return -1;
}

int k3_system_clock_init(void)
{
	struct stopwatch deadline;
	uint32_t pll6;

	/* PLL1 runs at 2457.6 MHz. PLL6 retains its 3200 MHz parameters but
	 * may still be powered down. Do not reprogram either shared PLL.
	 */
	if (read32((void *)PLL1_SWCR1) != 0x0b330ccc ||
	    (read32((void *)(PLL1_SWCR1 + 4)) & (PLL_ENABLE | 0xff00)) !=
	    (PLL_ENABLE | 0xcd00) ||
	    read32((void *)(PLL1_SWCR1 + 8)) != 0xa0558989 ||
	    read32((void *)PLL6_SWCR1) != 0x0b422aaa ||
	    (read32((void *)(PLL6_SWCR1 + 4)) & 0xff00) != 0xab00 ||
	    read32((void *)(PLL6_SWCR1 + 8)) != 0xa0558e8e ||
	    !(read32((void *)PLL_STATUS) & (1U << 24)))
		return -1;

	pll6 = read32((void *)(PLL6_SWCR1 + 4));
	if (!(pll6 & PLL_ENABLE)) {
		write32((void *)(PLL6_SWCR1 + 4), pll6 | PLL_ENABLE);
		udelay(50);
	}
	stopwatch_init_usecs_expire(&deadline, 3000);
	while (!(read32((void *)PLL_STATUS) & (1U << 29))) {
		if (stopwatch_expired(&deadline)) {
			write32((void *)(PLL6_SWCR1 + 4), pll6);
			return -1;
		}
		udelay(5);
	}

	setbits32((void *)(PLL1_SWCR1 + 4), PLL_DIV2 | PLL_DIV8);
	setbits32((void *)(PLL6_SWCR1 + 4), PLL_DIV4);
	setbits32((void *)ACGR, (1U << 16) | (1U << 13) | (1U << 12));

	/* TOP_DCLK: PLL6 / 4, 800 MHz; sources 4-7 bypass the divider. */
	if (change_clock(TOP_DCLK, 0xfe, (5U << 2) | (1U << 1), 1U << 8))
		return -1;
	/* System AXI: PLL1 / 8, 307.2 MHz. */
	if (change_clock(AXI_CLOCK, 7, 0, 1U << 4))
		return -1;
	/* APB: PLL1 / 24, 102.4 MHz. */
	clrsetbits32((void *)APB_CLOCK, 3, 3);
	if ((read32((void *)APB_CLOCK) & 3) != 3)
		return -1;
	/* CCI: PLL1 / 2, 1228.8 MHz. Preserve the AXI M0 port divider
	 * and the independent automatic-gating and idle controls.
	 */
	return change_clock(CCI_CLOCK, 7, 6, 1U << 12);
}
