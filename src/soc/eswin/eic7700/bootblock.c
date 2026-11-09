// SPDX-License-Identifier: GPL-2.0-only

#include <bootblock_common.h>
#include <device/mmio.h>
#include <soc/addressmap.h>

/* SYSCRG lsp_clk_en0: UART0 APB clock. */
#define LSP_CLOCK_ENABLE 0x200
#define UART0_CLOCK_ENABLE (1U << 17)
#define AON_INTERRUPT_MUX 0x3c0
#define AON_I2C0_IRQ_LPCPU (1U << 16)
#define AON_I2C1_IRQ_LPCPU (1U << 15)
#define AON_GPIO_IRQ_LPCPU (1U << 12)

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
