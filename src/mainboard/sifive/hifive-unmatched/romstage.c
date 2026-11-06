/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/encoding.h>
#include <cbmem.h>
#include <console/console.h>
#include <delay.h>
#include <gpio.h>
#include <program_loading.h>
#include <romstage_common.h>
#include <soc/addressmap.h>
#include <soc/clock.h>
#include <soc/sdram.h>

/* Board reset timings follow U-Boot board/sifive/unmatched/spl.c. */
static void reset_device(gpio_t pin, unsigned int pulse_us)
{
	gpio_set(pin, 1);
	gpio_set_direction(pin, GPIO_OUTPUT);
	udelay(1);
	gpio_set(pin, 0);
	udelay(pulse_us);
	gpio_set(pin, 1);
}

void __noreturn romstage_main(void)
{
	write_csr(0x7c1, 0); // CSR_U74_FEATURE_DISABLE (enable all Features)

	clock_init();

	console_init();

	sdram_init((u64)16*GiB);

	/* ASM1042A bridge, ASM1074 hub, and USB3320C ULPI PHY. */
	reset_device(7, 3000);
	reset_device(11, 100);
	reset_device(9, 1);

	cbmem_initialize_empty();

	run_ramstage();
}
