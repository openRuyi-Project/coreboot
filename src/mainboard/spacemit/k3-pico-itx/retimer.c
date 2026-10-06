/* SPDX-License-Identifier: GPL-2.0-or-later */
/* RT7451 sequence adapted from SpacemiT U-Boot e4043c70d6e9,
 * drivers/misc/rt7451.c, Copyright (C) 2025 SPACEMIT Micro Limited.
 * Board DT connects both addresses to Chrome EC remote I2C bus 1.
 */
#include <console/console.h>
#include <delay.h>
#include <device/mmio.h>
#include <ec/google/chromeec/ec.h>
#include <ec/google/chromeec/ec_mmio.h>
#include <mainboard/early_ec.h>
#include <soc/addressmap.h>

#define EC_WINDOW 0x84000000
#define EC_REMOTE_I2C_BUS 1

static int ensure_register(uint8_t chip, uint8_t reg, uint8_t expected,
			   unsigned int settle_us)
{
	uint8_t value;
	int rc = google_chromeec_i2c_xfer_on_bus(EC_REMOTE_I2C_BUS, chip, reg,
					       1, &value, 1, 1);
	if (rc)
		return rc;
	if (value == expected)
		return 0;
	value = expected;
	rc = google_chromeec_i2c_xfer_on_bus(EC_REMOTE_I2C_BUS, chip, reg,
					  1, &value, 1, 0);
	if (rc)
		return rc;
	udelay(100);
	rc = google_chromeec_i2c_xfer_on_bus(EC_REMOTE_I2C_BUS, chip, reg,
					  1, &value, 1, 1);
	if (rc)
		return rc;
	if (value != expected) {
		printk(BIOS_ERR, "RT7451: %02x:%02x readback %02x != %02x\n",
		       chip, reg, value, expected);
		return -1;
	}
	if (settle_us)
		udelay(settle_us);
	return 0;
}

void pico_itx_typec_retimer_init(void)
{
	/* Reuse the eSPI link and EC window initialized for early cooling. */
	if (read32((void *)(K3_ESPI_BASE + 0x38)) != EC_WINDOW ||
	    google_chromeec_mmio_init(EC_WINDOW, EC_WINDOW + 0x100, 0x200)) {
		printk(BIOS_ERR, "RT7451: EC peripheral window unavailable\n");
		return;
	}
	if (ensure_register(0x13, 0xf8, 0x16, 1000) ||
	    ensure_register(0x29, 0xa4, 0x28, 0)) {
		printk(BIOS_ERR, "RT7451: Type-C retimer initialization failed\n");
		return;
	}
	printk(BIOS_INFO, "RT7451: Type-C retimer configuration verified\n");
}
