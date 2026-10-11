/* SPDX-License-Identifier: GPL-2.0-only */
#include <console/console.h>
#include <device/i2c_simple.h>
#include <device/mmio.h>
#include <soc/raminit.h>
#include <spi_flash.h>
#include <string.h>
#include <mainboard/flash.h>
#include <mainboard/early_ec.h>
#include <mainboard/power.h>

uint64_t pico_itx_ddr_init(void)
{
	uint8_t eeprom[256];
	uint8_t offset = 0;
	struct i2c_msg messages[] = {
		{ .slave = 0x50, .buf = &offset, .len = 1 },
		{ .slave = 0x50, .flags = I2C_M_RD, .buf = eeprom, .len = sizeof(eeprom) },
	};
	/* Pico ITX EEPROM: TWSI2, GPIO46/47 function 5, as in the SPL board setup. */
	setbits32((void *)0xd401503c, 3);
	uint32_t scl = read32((void *)0xd401e0b8);
	uint32_t sda = read32((void *)0xd401e0bc);
	write32((void *)0xd401e0b8, 0xc045);
	write32((void *)0xd401e0bc, 0xc045);
	int ret = platform_i2c_transfer(2, messages, ARRAY_SIZE(messages));
	write32((void *)0xd401e0b8, scl);
	write32((void *)0xd401e0bc, sda);
	if (ret)
		printk(BIOS_WARNING, "DDR EEPROM read failed; using board default\n");

	/* Same fallback part-number as the original Pico ITX SPL device tree. */
	uint64_t size = k3_dram_initialize("MT62F2G32D4DS", ret ? NULL : eeprom,
					ret ? 0 : sizeof(eeprom));
	pico_itx_parse_power_config(ret ? NULL : eeprom, ret ? 0 : sizeof(eeprom));
	printk(BIOS_INFO, "K3 native coreboot: DDR checkpoint complete, %llu MiB configured\n",
	       size / MiB);
	return size;
}

void mainboard_restore_dram_training(void *record, size_t size)
{
	/* SPL forces full training for USB download and SD boot. This board's
	 * persistent coreboot boot medium is NOR, whose private area is 0x10000.
	 */
	if (!CONFIG(SPACEMIT_K3_NATIVE_BOOT) ||
	    (read32((void *)0xd4282d10) & 0xfff) != 0xb01)
		return;

	struct spi_flash flash;
	if (pico_itx_flash_init() || spi_flash_probe(0, 0, &flash) ||
	    spi_flash_read(&flash, 0x10000, size, record)) {
		memset(record, 0, 128);
		printk(BIOS_WARNING, "DDR training record unavailable; cold training\n");
	}
}
