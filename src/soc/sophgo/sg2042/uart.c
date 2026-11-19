// SPDX-License-Identifier: GPL-2.0-only
#include <console/uart.h>
#include <soc/addressmap.h>

uintptr_t uart_platform_base(unsigned int index)
{
	return index == 0 ? SG2042_UART0 : 0;
}

unsigned int uart_platform_refclk(void)
{
	return 500000000;
}
