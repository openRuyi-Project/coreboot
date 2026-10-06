/* SPDX-License-Identifier: GPL-2.0-only */
#include <console/uart.h>
#include <types.h>
#include <soc/uart.h>
#include <mainboard/addressmap.h>

uintptr_t uart_platform_base(unsigned int idx)
{
	return idx == 0 && k3_uart_clock_hz() ? K3_UART0_BASE : 0;
}

unsigned int uart_platform_refclk(void)
{
	return k3_uart_clock_hz();
}
