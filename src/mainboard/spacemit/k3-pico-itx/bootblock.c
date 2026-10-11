/* SPDX-License-Identifier: GPL-2.0-only */
#include <bootblock_common.h>
#include <console/console.h>
#include <mainboard/early_ec.h>
#include <mainboard/native_boot.h>
#include <mainboard/power.h>
#include <soc/cpu_clock.h>
#include <soc/power_domains.h>
#include <soc/system_clock.h>

void bootblock_mainboard_init(void)
{
	printk(BIOS_INFO, "K3 native coreboot: BootROM SRAM entry, DDR not initialized\n");
	if (pico_itx_early_cooling_init())
		die("K3 native coreboot: early cooling failed; DDR remains untouched\n");
	printk(BIOS_INFO, "K3 native coreboot: EC fan 0 full duty acknowledged\n");
	uint64_t trained_bytes = pico_itx_ddr_init();
	if (k3_system_clock_init())
		die("K3 system clock initialization failed\n");
	if (pico_itx_power_init())
		die("K3 board power initialization failed\n");
	k3_x100_init();
	if (k3_power_domain_set(K3_PMDOMAIN_AUDIO, true))
		die("K3 AUDIO power domain failed to start\n");
	pico_itx_typec_retimer_init();
	pico_itx_native_boot(trained_bytes);
}
