// SPDX-License-Identifier: GPL-2.0-only

#include <bootstate.h>
#include <console/console.h>
#include <delay.h>
#include <device/mmio.h>
#include <soc/addressmap.h>
#include <timer.h>

#include "gpio.h"

#define CPU_PLL_CFG0		0x64
#define CPU_PLL_CFG1		0x68
#define CPU_PLL_CFG2		0x6c
#define PLL_STATUS		0xa4
#define CPU_CLK_CTRL		0x208
#define CPU_PLL_LOCK		BIT(5)
#define CPU_BUS_1_TO_1		BIT(20)
#define CPU_SOURCE_MASK		3
#define CPU_SOURCE_XTAL		2
#define CPU_SOURCE_PLL		0
#define CPU_PLL_ENABLE		BIT(0)

DEFINE_BITFIELD(PLL_FBDIV, 31, 20)
DEFINE_BITFIELD(PLL_REFDIV, 17, 12)
DEFINE_BITFIELD(PLL_BYPASS, 7, 4)
DEFINE_BIT(PLL_DAC_ENABLE, 8)
DEFINE_BIT(PLL_DSM_ENABLE, 9)
DEFINE_BITFIELD(PLL_FRAC, 27, 4)
DEFINE_BIT(PLL_FOUT1_ENABLE, 1)
DEFINE_BITFIELD(PLL_POSTDIV1_A, 10, 8)
DEFINE_BITFIELD(PLL_POSTDIV1_B, 14, 12)

static bool set_cpu_pll(uintptr_t syscrg)
{
	void *ctrl = (void *)(syscrg + CPU_CLK_CTRL);
	void *cfg0 = (void *)(syscrg + CPU_PLL_CFG0);
	void *stat = (void *)(syscrg + PLL_STATUS);

	/* The crystal is always running. Keep the bus at half core speed. */
	clrsetbits32(ctrl, CPU_SOURCE_MASK | CPU_BUS_1_TO_1, CPU_SOURCE_XTAL);
	if ((read32(ctrl) & (CPU_SOURCE_MASK | CPU_BUS_1_TO_1)) != CPU_SOURCE_XTAL)
		return false;

	clrbits32(cfg0, CPU_PLL_ENABLE);
	if (!wait_us(100, !(read32(stat) & CPU_PLL_LOCK)))
		return false;

	/* 24 MHz * 300 / 4, with both FOUT1 post-dividers set to 1. */
	SET32_BITFIELDS(cfg0, PLL_FBDIV, 300, PLL_REFDIV, 1, PLL_BYPASS, 0,
			PLL_DAC_ENABLE, 1, PLL_DSM_ENABLE, 1);
	SET32_BITFIELDS((void *)(syscrg + CPU_PLL_CFG1),
			PLL_FRAC, 0, PLL_FOUT1_ENABLE, 1);
	SET32_BITFIELDS((void *)(syscrg + CPU_PLL_CFG2), PLL_POSTDIV1_A, 0, PLL_POSTDIV1_B, 0);

	udelay(1);
	setbits32(cfg0, CPU_PLL_ENABLE);
	if (!wait_us(100, read32(stat) & CPU_PLL_LOCK))
		return false;

	clrsetbits32(ctrl, CPU_SOURCE_MASK, CPU_SOURCE_PLL);
	return (read32(ctrl) & CPU_SOURCE_MASK) == CPU_SOURCE_PLL;
}

static void set_cpu_clocks(void *unused)
{
	if ((read32((void *)CPU_VSEL_PAD) & PAD_FUNC_MASK) != PAD_FUNC_GPIO ||
	    !(read32((void *)GPIOC_DIRECTION) & CPU_VSEL) ||
	    !(read32((void *)GPIOC_INPUT) & CPU_VSEL)) {
		printk(BIOS_ERR, "D560: VDD_CPU 0.9 V select is not asserted\n");
		return;
	}

	for (unsigned int die = 0; die < EIC770X_DIES; die++) {
		if (!set_cpu_pll(EIC7700_SYSCRG + die * EIC770X_DIE_MMIO_STRIDE))
			printk(BIOS_ERR, "D560: CPU clock setup failed on die %u\n", die);
	}
}

BOOT_STATE_INIT_ENTRY(BS_DEV_INIT, BS_ON_ENTRY, set_cpu_clocks, NULL);
