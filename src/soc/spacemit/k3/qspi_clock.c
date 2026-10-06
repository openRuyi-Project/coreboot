/* SPDX-License-Identifier: GPL-2.0-only */
/* Based on the pinned SpacemiT ccu-k3.c/ccu_mix.c register definitions.
 * Copyright (c) 2023, spacemit Corporation.
 */
#include <device/mmio.h>
#include <delay.h>
#include <soc/addressmap.h>
#include <soc/qspi.h>
#include <timer.h>

#define PLL1_CTRL (K3_APBS_BASE + 0x104)
#define PLL1_DX_DIV (31U << 23)
#define PLL1_DX_GATE (1U << 22)
#define QSPI_CLOCK (K3_APMU_BASE + 0x60)
#define CLOCK_DIV (7U << 9)
#define CLOCK_MUX (7U << 6)
#define CLOCK_CHANGE (1U << 12)
#define CLOCK_ENABLE ((1U << 4) | (1U << 3))
#define RESET_RELEASE 3U

int k3_qspi_prepare_clock(void)
{
	uint32_t parent = read32((void *)PLL1_CTRL);
	struct stopwatch deadline;
	/* BootROM PLL1 is 2457.6 MHz. Do not retune its VCO or take over an
	 * active, differently configured shared DX output. DX / 32 followed
	 * by QSPI / 3 supplies 25.6 MHz, below Pico ITX's 26.5 MHz limit.
	 */
	if (read32((void *)(K3_APBS_BASE + 0x100)) != 0x0b330ccc ||
	    read32((void *)(K3_APBS_BASE + 0x108)) != 0xa0558989 ||
	    (parent & 0x1ff00) != 0x1cd00 ||
	    !(read32((void *)(K3_MPMU_BASE + 0x10)) & (1U << 24)) ||
	    ((parent & PLL1_DX_GATE) && (parent & PLL1_DX_DIV) != PLL1_DX_DIV))
		return -1;
	/* FC does not acknowledge while controller reset is asserted. Match
	 * the reference order: change rate first, then pulse reset with clocks
	 * enabled. The cold BootROM state already has these resets released.
	 */
	clrsetbits32((void *)QSPI_CLOCK, CLOCK_ENABLE, RESET_RELEASE);
	write32((void *)PLL1_CTRL, parent | PLL1_DX_DIV | PLL1_DX_GATE);
	if ((read32((void *)PLL1_CTRL) & (PLL1_DX_DIV | PLL1_DX_GATE)) !=
	    (PLL1_DX_DIV | PLL1_DX_GATE)) {
		write32((void *)PLL1_CTRL, parent);
		return -1;
	}
	clrsetbits32((void *)QSPI_CLOCK, CLOCK_MUX | CLOCK_DIV,
		     (5U << 6) | (2U << 9));
	setbits32((void *)QSPI_CLOCK, CLOCK_CHANGE);
	stopwatch_init_usecs_expire(&deadline, 3000);
	while (read32((void *)QSPI_CLOCK) & CLOCK_CHANGE) {
		if (stopwatch_expired(&deadline)) {
			write32((void *)PLL1_CTRL, parent);
			return -1;
		}
	}
	setbits32((void *)QSPI_CLOCK, CLOCK_ENABLE);
	/* Retained controller reset pulse with its functional/bus clocks on. */
	clrbits32((void *)QSPI_CLOCK, RESET_RELEASE);
	udelay(1);
	setbits32((void *)QSPI_CLOCK, RESET_RELEASE);
	return (read32((void *)QSPI_CLOCK) &
		(CLOCK_MUX | CLOCK_DIV | CLOCK_ENABLE | RESET_RELEASE)) ==
	       ((5U << 6) | (2U << 9) | CLOCK_ENABLE | RESET_RELEASE) ? 0 : -1;
}
