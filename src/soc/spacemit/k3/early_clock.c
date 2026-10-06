/* SPDX-License-Identifier: GPL-2.0-only */
/* Based on the K3 clock definitions and PLLA enable path in SpacemiT
 * U-Boot e4043c70d6e921238fe43e485f1efc71b30b00d0.
 * Copyright (c) 2024 SpacemiT Technology Co. Ltd
 * Copyright (c) 2024-2025 Haylen Chu <heylenay@4d2.org>
 */
#include <console/console.h>
#include <delay.h>
#include <device/mmio.h>
#include <soc/addressmap.h>
#include <soc/espi.h>
#include <timer.h>

#define PLL2_SWCR1 (K3_APBS_BASE + 0x118)
#define PLL2_SWCR2 (K3_APBS_BASE + 0x11c)
#define PLL2_SWCR3 (K3_APBS_BASE + 0x120)
#define PLL2_ENABLE (1U << 16)
#define PLL2_DIV5_ENABLE (1U << 4)
#define PLL2_LOCK (1U << 25)
#define PLL_STATUS (K3_MPMU_BASE + 0x10)
#define ESPI_CLOCK (K3_APMU_BASE + 0x240)
#define ESPI_SOURCE_MASK (7U << 4)
#define ESPI_CLOCK_ENABLES ((1U << 7) | (1U << 3) | (1U << 1))
#define ESPI_RESET_DEASSERT ((1U << 2) | (1U << 0))

int k3_espi_prepare_clock(void)
{
	struct stopwatch deadline;
	uint32_t value = read32((void *)PLL2_SWCR2);
	/* PLL2 / 5 / 30 supplies 20 MHz. Validate the retained BootROM rate
	 * before selecting it; do not retune a PLL shared with other consumers.
	 */
	if (read32((void *)PLL2_SWCR1) != 0x0b3e2000 ||
	    (value & 0xff00) != 0 || read32((void *)PLL2_SWCR3) != 0xa0558c8c) {
		printk(BIOS_ERR, "K3 eSPI requires the BootROM 3 GHz PLL2 configuration\n");
		return -1;
	}
	write32((void *)PLL2_SWCR2, value | PLL2_ENABLE);
	stopwatch_init_usecs_expire(&deadline, 3000);
	while (!(read32((void *)PLL_STATUS) & PLL2_LOCK)) {
		if (stopwatch_expired(&deadline)) {
			printk(BIOS_ERR, "K3 PLL2 lock timed out\n");
			return -1;
		}
		udelay(5);
	}
	write32((void *)PLL2_SWCR2, read32((void *)PLL2_SWCR2) | PLL2_DIV5_ENABLE);
	if ((read32((void *)PLL2_SWCR2) & (PLL2_ENABLE | PLL2_DIV5_ENABLE)) !=
	    (PLL2_ENABLE | PLL2_DIV5_ENABLE)) {
		printk(BIOS_ERR, "K3 PLL2 output did not enable\n");
		return -1;
	}
	value = (read32((void *)ESPI_CLOCK) & ~ESPI_SOURCE_MASK) | ESPI_CLOCK_ENABLES;
	write32((void *)ESPI_CLOCK, value);
	write32((void *)ESPI_CLOCK, value | ESPI_RESET_DEASSERT);
	return (read32((void *)ESPI_CLOCK) & (ESPI_SOURCE_MASK | ESPI_CLOCK_ENABLES |
		ESPI_RESET_DEASSERT)) == (ESPI_CLOCK_ENABLES | ESPI_RESET_DEASSERT) ? 0 : -1;
}
