/* SPDX-License-Identifier: GPL-2.0-only */
/* K3 PLL1, UART mux and DDN layout. */
#include <device/mmio.h>
#include <soc/addressmap.h>
#include <soc/uart.h>
#include <stdint.h>

#define APBC_UART0_CLOCK 0xd4015000
#define UART_SOURCE_SHIFT 4
#define UART_SOURCE_MASK 7
#define PLL1_SWCR1 (K3_APBS_BASE + 0x100)
#define PLL1_SWCR2 (K3_APBS_BASE + 0x104)
#define PLL1_SWCR3 (K3_APBS_BASE + 0x108)
#define UART_DDN1 (K3_MPMU_BASE + 0x14)
#define UART_DDN2 (K3_MPMU_BASE + 0x10b0)
#define DDN_FIELD_MASK 0x1fff

static int pll1_valid(void)
{
	/* Match the supported PLL1 rate table, including fractional bits.
	 * Gate/enable bits do not change the configured frequency.
	 */
	return read32((void *)PLL1_SWCR1) == 0x0b330ccc &&
	       (read32((void *)PLL1_SWCR2) & 0xff00) == 0xcd00 &&
	       read32((void *)PLL1_SWCR3) == 0xa0558989;
}

int k3_uart_clock_init(void)
{
	if (!pll1_valid())
		return -1;
	/* PLL1 / 16 * 24 / (2 * 125) = 14.7456 MHz. */
	setbits32((void *)PLL1_SWCR2, 1U << 7);
	clrsetbits32((void *)UART_DDN1, (DDN_FIELD_MASK << 16) | DDN_FIELD_MASK,
		     (125U << 16) | 24);
	setbits32((void *)(K3_MPMU_BASE + 0x1024), 1U << 1);
	/* Select the slow UART clock, enable both clocks and release reset. */
	clrsetbits32((void *)APBC_UART0_CLOCK,
		     (UART_SOURCE_MASK << UART_SOURCE_SHIFT) | (1U << 2),
		     (1U << UART_SOURCE_SHIFT) | 3);
	return k3_uart_clock_hz() == 14745600 ? 0 : -1;
}

unsigned int k3_uart_clock_hz(void)
{
	if (!pll1_valid())
		return 0;

	uint64_t rate = 2457600000ULL;
	unsigned int source = (read32((void *)APBC_UART0_CLOCK) >>
			       UART_SOURCE_SHIFT) & UART_SOURCE_MASK;
	uint32_t ddn;

	switch (source) {
	case 0:
		return rate / 8 * 3 / 16;
	case 1:
		rate /= 16;
		ddn = read32((void *)UART_DDN1);
		break;
	case 2:
		rate /= 4;
		ddn = read32((void *)UART_DDN2);
		break;
	default:
		return 0;
	}

	unsigned int num = (ddn >> 16) & DDN_FIELD_MASK;
	unsigned int den = ddn & DDN_FIELD_MASK;
	if (!num || !den)
		return 0;
	/* Same integer order as Linux: parent * denominator / 2 / numerator. */
	rate = rate * den / 2 / num;
	return rate <= UINT32_MAX ? rate : 0;
}
