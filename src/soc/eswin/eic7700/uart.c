// SPDX-License-Identifier: GPL-2.0-only

#include <commonlib/helpers.h>
#include <console/uart.h>
#include <device/mmio.h>
#include <soc/addressmap.h>

#define SYS_CONFIG_CLOCK 0x20c

uintptr_t uart_platform_base(unsigned int index)
{
	return index == 0 ? EIC7700_UART0 : 0;
}

unsigned int uart_platform_refclk(void)
{
	uint32_t value = read32((void *)(EIC7700_SYSCRG + SYS_CONFIG_CLOCK));

	if (value & 1)
		return 24000000;
	return 400000000U / MAX(2U, (value >> 4) & 7);
}
