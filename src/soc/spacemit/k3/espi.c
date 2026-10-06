/* SPDX-License-Identifier: GPL-2.0-only */
/* Register/command sequencing adapted from SpacemiT U-Boot
 * e4043c70d6e921238fe43e485f1efc71b30b00d0 drivers/espi,
 * Copyright (c) 2025 SpacemiT Co., Ltd. (BSD-2-Clause-Patent).
 */
#include <console/console.h>
#include <delay.h>
#include <device/mmio.h>
#include <ec/google/chromeec/ec.h>
#include <ec/google/chromeec/ec_mmio.h>
#include <soc/espi.h>
#include <timer.h>

#define DOWN_HEADER 0x00
#define GLOBAL_CONTROL0 0x30
#define GLOBAL_CONTROL1 0x34
#define PERIPHERAL_BASE0 0x38
#define PERIPHERAL_BASE1 0x3c
#define SLAVE_CONFIG 0x68
#define SLAVE_INT_ENABLE 0x6c
#define SLAVE_INT_STATUS 0x70
#define COMMAND_ENABLE (1U << 3)
#define COMMAND_DONE (1U << 28)
#define COMMAND_ERRORS 0x0000fff7U
#define NO_RESPONSE_STATUS (1U << 4)
#define RESET_DEASSERT (1U << 5)
#define SOFTWARE_RESET (1U << 0)
#define GET_CONFIGURATION 1
#define SET_CONFIGURATION 0
#define IN_BAND_RESET 2
#define GENERAL_CONFIG 0x08
#define GENERAL_CRC (1U << 31)
#define MASTER_CONFIG_FIELDS (GENERAL_CRC | (1U << 30) | (3U << 28) | (7U << 25) | 0xfU)
#define CHANNEL_MASK 0x0f
#define PERIPHERAL_CHANNEL 1
#define COMMAND_TIMEOUT_US 50000
static uintptr_t base;

static uint32_t reg_read(unsigned int offset)
{
	return read32((void *)(base + offset));
}

static void reg_write(unsigned int offset, uint32_t value)
{
	write32((void *)(base + offset), value);
}

static int idle(void)
{
	struct stopwatch deadline;
	stopwatch_init_usecs_expire(&deadline, COMMAND_TIMEOUT_US);
	do {
		if (!(reg_read(DOWN_HEADER) & COMMAND_ENABLE))
			return 0;
		udelay(10);
	} while (!stopwatch_expired(&deadline));
	return -1;
}

static int command(unsigned int operation, uint16_t address, uint32_t *value)
{
	uint8_t header[7] = {0, address >> 8, address};
	size_t count = operation == SET_CONFIGURATION ? 7 : 3;
	struct stopwatch deadline;
	if (idle())
		return -1;
	if (operation == IN_BAND_RESET)
		count = 0;
	if (operation == GET_CONFIGURATION)
		reg_write(4, 0); /* Do not accept a previous transaction's data. */
	if (operation == SET_CONFIGURATION) {
		for (unsigned int i = 0; i < 4; i++)
			header[3 + i] = *value >> (8 * i);
	}
	/* Clear completion before issuing, never after a fast device can reply. */
	reg_write(SLAVE_INT_STATUS, COMMAND_DONE | COMMAND_ERRORS);
	for (size_t i = 0; i < count; i++) {
		unsigned int offset = 4 * ((i + 1) / 4);
		unsigned int shift = 8 * ((i + 1) % 4);
		reg_write(offset, (reg_read(offset) & ~(0xffU << shift)) |
				 ((uint32_t)header[i] << shift));
	}
	__sync_synchronize();
	reg_write(DOWN_HEADER, (reg_read(DOWN_HEADER) & ~0x3fU) |
			      operation | COMMAND_ENABLE);
	if (operation == IN_BAND_RESET)
		return idle();
	stopwatch_init_usecs_expire(&deadline, COMMAND_TIMEOUT_US);
	do {
		uint32_t status = reg_read(SLAVE_INT_STATUS);
		uint32_t handled = status & (COMMAND_DONE | COMMAND_ERRORS);
		uint32_t errors = status & COMMAND_ERRORS;
		if (handled)
			reg_write(SLAVE_INT_STATUS, handled);
		/* K3 reports bit 4 with completed, valid GET/SET_CONFIG transactions.
		 * A fresh GET after clearing status returned 0x8308000f with status
		 * 0x10000010 on the working SPL. Other error bits remain fatal.
		 */
		if (status & COMMAND_DONE)
			errors &= ~NO_RESPONSE_STATUS;
		if (errors) {
			printk(BIOS_ERR, "K3 eSPI command %u status %#x\n", operation, status);
			return -1;
		}
		if (status & COMMAND_DONE) {
			__sync_synchronize();
			if (operation == GET_CONFIGURATION)
				*value = reg_read(4); /* response bytes 3..6 */
			return 0;
		}
		udelay(10);
	} while (!stopwatch_expired(&deadline));
	printk(BIOS_ERR, "K3 eSPI command %u timed out\n", operation);
	return -1;
}

int k3_espi_init(const struct k3_espi_config *config)
{
	static const uint32_t channel_config[] = { 0x1115, 0x70701, 0x111, 0x11125 };
	uint32_t caps = 0, general, master, peripheral;
	struct stopwatch deadline;
	google_chromeec_mmio_disable();
	if (!config || !config->registers || (config->registers & 3) ||
	    config->registers > UINTPTR_MAX - 0x1000 ||
	    !config->peripheral_base0 || config->peripheral_base0 > UINT32_MAX - 0x300 ||
	    config->peripheral_base1 <= config->peripheral_base0)
		return -1;
	base = config->registers;
	reg_write(SLAVE_INT_ENABLE, COMMAND_DONE | COMMAND_ERRORS);
	reg_write(GLOBAL_CONTROL1, reg_read(GLOBAL_CONTROL1) & ~RESET_DEASSERT);
	reg_write(GLOBAL_CONTROL1, reg_read(GLOBAL_CONTROL1) | RESET_DEASSERT | SOFTWARE_RESET);
	if (command(IN_BAND_RESET, 0, NULL))
		return -1;
	/* The EC may still be restarting; readiness is established by its reply. */
	stopwatch_init_usecs_expire(&deadline, 250000);
	while (command(GET_CONFIGURATION, GENERAL_CONFIG, &caps) ||
	       !(caps & PERIPHERAL_CHANNEL) || caps == UINT32_MAX) {
		if (stopwatch_expired(&deadline)) {
			printk(BIOS_ERR, "K3 eSPI has no valid peripheral capability response\n");
			return -1;
		}
	}
	general = GENERAL_CRC | (caps & CHANNEL_MASK); /* 20 MHz, single I/O, IO1 alert */
	for (unsigned int i = 0; i < ARRAY_SIZE(channel_config); i++) {
		uint32_t value = channel_config[i];
		if ((caps & (1U << i)) && command(SET_CONFIGURATION, 0x10 * (i + 1), &value))
			return -1;
	}
	if (command(SET_CONFIGURATION, GENERAL_CONFIG, &general))
		return -1;
	/* Slave channel bits 0..3 map to master enable bits 3..0. */
	master = GENERAL_CRC;
	for (unsigned int i = 0; i < 4; i++)
		if (caps & (1U << i))
			master |= 1U << (3 - i);
	reg_write(SLAVE_CONFIG, (reg_read(SLAVE_CONFIG) & ~MASTER_CONFIG_FIELDS) | master);
	if (command(GET_CONFIGURATION, 0x10, &peripheral) || !(peripheral & 1U)) {
		printk(BIOS_ERR, "K3 eSPI peripheral channel did not enable\n");
		return -1;
	}
	reg_write(GLOBAL_CONTROL0, (reg_read(GLOBAL_CONTROL0) & ~0x00ffff00U) |
				 0x00ffff00U | 1U);
	reg_write(PERIPHERAL_BASE0, config->peripheral_base0);
	reg_write(PERIPHERAL_BASE1, config->peripheral_base1);
	if (reg_read(PERIPHERAL_BASE0) != config->peripheral_base0 ||
	    reg_read(PERIPHERAL_BASE1) != config->peripheral_base1) {
		printk(BIOS_ERR, "K3 eSPI peripheral window configuration failed\n");
		return -1;
	}
	return google_chromeec_mmio_init(config->peripheral_base0,
					config->peripheral_base0 + 0x100, 0x200);
}
