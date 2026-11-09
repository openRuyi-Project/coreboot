// SPDX-License-Identifier: GPL-2.0-only

#include <bootblock_common.h>
#include <device/mmio.h>
#include <soc/addressmap.h>

/* SYSCRG lsp_clk_en0: UART0 APB clock. */
#define LSP_CLOCK_ENABLE 0x200
#define UART0_CLOCK_ENABLE (1U << 17)

void bootblock_soc_early_init(void)
{
	setbits32((void *)(EIC7700_SYSCRG + LSP_CLOCK_ENABLE), UART0_CLOCK_ENABLE);
}
