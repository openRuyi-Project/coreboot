// SPDX-License-Identifier: GPL-2.0-only

#include <bootblock_common.h>
#include <console/console.h>
#include <device/mmio.h>
#include <soc/addressmap.h>
#include <symbols.h>

/* SYSCRG lsp_clk_en0: UART0 APB clock. */
#define LSP_CLOCK_ENABLE 0x200
#define UART0_CLOCK_ENABLE (1U << 17)
#define AON_INTERRUPT_MUX 0x3c0
#define AON_I2C0_IRQ_LPCPU (1U << 16)
#define AON_I2C1_IRQ_LPCPU (1U << 15)
#define AON_GPIO_IRQ_LPCPU (1U << 12)
#define CCACHE_CONFIG 0x00
#define CCACHE_WAY_ENABLE 0x08
#define CCACHE_WAYS_SHIFT 8
#define CCACHE_WAYS_MASK 0xff
#define MCPU_BOOT_ADDRESS_HI 0x31c
#define MCPU_BOOT_ADDRESS_LO 0x320
#define MCPU_BOOT_ADDRESS_STRIDE 8
#define MCPU_RESET_CONTROL 0x44c
#define MCPU_CORE_RESET_N 0xf
#define MCPU_BUS_RESET_N (1U << 4)
#define MCPU_DEBUG_RESET_N (1U << 5)

static void start_die1_harts(void)
{
	uintptr_t crg = EIC7700_SYSCRG + EIC770X_DIE_MMIO_STRIDE;
	uint64_t entry = (uintptr_t)_bootblock;

	/* BootROM starts the boot cluster. Point the other four harts at
	 * the same bootblock before allowing them to leave reset.
	 */
	for (unsigned int core = 0; core < 4; core++) {
		uintptr_t vector = crg + core * MCPU_BOOT_ADDRESS_STRIDE;
		write32((void *)(vector + MCPU_BOOT_ADDRESS_HI), entry >> 32);
		write32((void *)(vector + MCPU_BOOT_ADDRESS_LO), (uint32_t)entry);
	}
	asm volatile("fence iorw, iorw" ::: "memory");
	setbits32((void *)(crg + MCPU_RESET_CONTROL),
		  MCPU_CORE_RESET_N | MCPU_BUS_RESET_N | MCPU_DEBUG_RESET_N);
}

void bootblock_soc_early_init(void)
{
	setbits32((void *)(EIC7700_SYSCRG + LSP_CLOCK_ENABLE), UART0_CLOCK_ENABLE);
	/*
	 * These peripherals belong to the MCPU OS. Keep security and timer
	 * interrupt destinations unchanged for the other processors.
	 */
	clrbits32((void *)(EIC7700_SYSCON + AON_INTERRUPT_MUX),
		  AON_I2C0_IRQ_LPCPU | AON_I2C1_IRQ_LPCPU | AON_GPIO_IRQ_LPCPU);
}

static void enable_cache(uintptr_t base)
{
	unsigned int ways = (read32((void *)(base + CCACHE_CONFIG)) >>
			     CCACHE_WAYS_SHIFT) & CCACHE_WAYS_MASK;

	if (!ways)
		die("Invalid EIC770x L3 cache configuration");

	/*
	 * BootROM releases the harts into DRAM after DDR training. The bootblock
	 * and every hart's stack are in DRAM, so none uses the L3 LIM backing.
	 */
	write32((void *)(base + CCACHE_WAY_ENABLE), ways - 1);
	asm volatile("fence rw, rw" ::: "memory");
	printk(BIOS_DEBUG, "L3 cache at 0x%lx: %u ways enabled\n", base, ways);
}

void bootblock_soc_init(void)
{
	if (CONFIG(SOC_ESWIN_EIC7702))
		start_die1_harts();

	/* Configure only the boot hart's local L3 at handoff. */
	enable_cache(EIC7700_CCACHE);
}
