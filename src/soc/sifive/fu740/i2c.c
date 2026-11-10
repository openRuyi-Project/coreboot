/* SPDX-License-Identifier: GPL-2.0-or-later */
/* OpenCores I2C sequencing follows U-Boot drivers/i2c/ocores_i2c.c.
 * Copyright Peter Korsgaard <peter@korsgaard.com>
 * Copyright (C) 2020 SiFive, Inc.
 */

#include <delay.h>
#include <console/console.h>
#include <device/i2c_simple.h>
#include <device/mmio.h>
#include <soc/addressmap.h>
#include <soc/clock.h>
#include <soc/i2c.h>

#define PRE_LOW 0x00
#define PRE_HIGH 0x04
#define CONTROL 0x08
#define DATA 0x0c
#define COMMAND 0x10
#define STATUS 0x10
#define ENABLE 0x80
#define START 0x91
#define WRITE 0x11
#define READ 0x21
#define READ_LAST 0x29
#define STOP 0x41
#define IACK 0x01
#define BUSY 0x40
#define ARB_LOST 0x20
#define TIP 0x02
#define IRQ_FLAG 0x01
#define NACK 0x80

static int wait_status(void *base, uint8_t mask, uint8_t expected)
{
	for (unsigned int i = 0; i < 1000; i++) {
		uint8_t status = read8(base + STATUS);
		if ((status & mask) == expected)
			return status;
		udelay(10);
	}
	printk(BIOS_ERR, "FU740 I2C at %p: timeout, status=%02x mask=%02x expected=%02x\n",
	       base, read8(base + STATUS), mask, expected);
	return -1;
}

static int stop_bus(void *base)
{
	write8(base + COMMAND, STOP);
	/* A latched arbitration error must not skip the STOP completion wait. */
	return wait_status(base, IRQ_FLAG | BUSY | TIP, IRQ_FLAG) < 0 ? -1 : 0;
}

static int wait_transfer(void *base, bool ack)
{
	/* IACK clears IF when the command starts. TIP may still be zero then. */
	int status = wait_status(base, IRQ_FLAG | TIP, IRQ_FLAG);
	return status < 0 || (status & ARB_LOST) || (ack && (status & NACK)) ? -1 : 0;
}

static int send_byte(void *base, uint8_t byte, uint8_t command)
{
	write8(base + DATA, byte);
	write8(base + COMMAND, command);
	return wait_transfer(base, true);
}

int fu740_i2c_init(unsigned int bus)
{
	if (bus > 1)
		return -1;

	void *base = (void *)(uintptr_t)(bus ? FU740_I2C_1 : FU740_I2C_0);
	int clock = clock_get_pclk();
	if (clock <= 0) {
		printk(BIOS_ERR, "FU740 I2C%u: invalid peripheral clock %d\n", bus, clock);
		return -1;
	}
	unsigned int divider = DIV_ROUND_UP(clock, 5 * I2C_SPEED_STANDARD) - 1;
	if (divider > UINT16_MAX) {
		printk(BIOS_ERR, "FU740 I2C%u: prescaler out of range %u\n", bus, divider);
		return -1;
	}

	uint8_t status = read8(base + STATUS);
	uint8_t control = read8(base + CONTROL);
	if (status & (BUSY | TIP))
		printk(BIOS_WARNING, "FU740 I2C%u: bus not idle, status=%02x control=%02x\n",
		       bus, status, control);

	/* FU740 manual 18.6: finish a transfer before disabling the controller. */
	if (control & ENABLE) {
		if (wait_status(base, TIP, 0) < 0)
			return -1;
		if ((read8(base + STATUS) & BUSY) && stop_bus(base))
			return -1;
	}

	write8(base + CONTROL, 0);
	write8(base + PRE_LOW, divider);
	write8(base + PRE_HIGH, divider >> 8);
	write8(base + COMMAND, IACK);
	write8(base + CONTROL, ENABLE);
	/* Clear any remaining BUSY indication with a completed STOP. */
	if ((read8(base + STATUS) & (BUSY | TIP)) && stop_bus(base))
		return -1;
	write8(base + COMMAND, IACK);
	return 0;
}

int platform_i2c_transfer(unsigned int bus, struct i2c_msg *segments, int count)
{
	if (bus > 1 || !segments || count <= 0)
		return -1;
	for (int i = 0; i < count; i++) {
		if (segments[i].slave > 0x7f || segments[i].flags & ~I2C_M_RD ||
		    (segments[i].len && !segments[i].buf))
			return -1;
	}

	void *base = (void *)(uintptr_t)(bus ? FU740_I2C_1 : FU740_I2C_0);
	uint8_t saved[] = {read8(base + PRE_LOW), read8(base + PRE_HIGH),
			   read8(base + CONTROL)};
	if (fu740_i2c_init(bus))
		return -1;

	int ret = 0;
	for (int i = 0; i < count && !ret; i++) {
		struct i2c_msg *msg = &segments[i];
		bool read = msg->flags & I2C_M_RD;
		ret = send_byte(base, (msg->slave << 1) | read, START);
		for (unsigned int j = 0; j < msg->len && !ret; j++) {
			if (read) {
				write8(base + COMMAND, j + 1 == msg->len ? READ_LAST : READ);
				ret = wait_transfer(base, false);
				if (!ret)
					msg->buf[j] = read8(base + DATA);
			} else {
				ret = send_byte(base, msg->buf[j], WRITE);
			}
		}
	}
	/* Leave a timed-out STOP pending instead of clearing it and disabling. */
	if (stop_bus(base))
		return -1;
	write8(base + COMMAND, IACK);
	write8(base + CONTROL, 0);
	write8(base + PRE_LOW, saved[0]);
	write8(base + PRE_HIGH, saved[1]);
	write8(base + CONTROL, saved[2]);
	return ret;
}
