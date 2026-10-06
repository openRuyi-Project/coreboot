/* SPDX-License-Identifier: GPL-2.0-only */
#include <console/console.h>
#include <device/mmio.h>
#include <mainboard/flash.h>
#include <soc/addressmap.h>
#include <soc/qspi.h>

int pico_itx_flash_init(void)
{
	/* Pico ITX pinctrl: QSPI D0..3/CLK mode 0, DS8, no pull; CS0 pull-up.
	 * Keep unused CS1 and unrelated pin fields unchanged.
	 */
	const unsigned int pins[] = { 140, 141, 142, 143, 146 };
	for (size_t i = 0; i < ARRAY_SIZE(pins); i++)
		clrsetbits32((void *)(uintptr_t)(K3_MFPR_BASE + 4 * pins[i]),
			     0xff77, (1U << 6) | (8U << 9));
	clrsetbits32((void *)(uintptr_t)(K3_MFPR_BASE + 4 * 144), 0xff77,
		     (1U << 6) | (8U << 9) | (6U << 13));
	if (k3_qspi_prepare_clock()) {
		printk(BIOS_ERR, "K3 QSPI clock/reset initialization failed\n");
		return -1;
	}
	if (k3_qspi_init()) {
		printk(BIOS_ERR, "K3 QSPI controller initialization failed\n");
		return -1;
	}
	return 0;
}
