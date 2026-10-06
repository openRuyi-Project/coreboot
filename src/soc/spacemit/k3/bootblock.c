/* SPDX-License-Identifier: GPL-2.0-only */
#include <bootblock_common.h>
#include <console/console.h>
#include <console/uart.h>
#include <soc/uart.h>

void bootblock_soc_early_init(void)
{
	uart_tx_flush(0);
	if (k3_uart_clock_init())
		die("K3 UART: unsupported reference clock\n");
}
