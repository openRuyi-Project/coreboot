/* SPDX-License-Identifier: BSD-2-Clause */
/* Copyright (c) 2026 Aurelien Jarno */
/* SpacemiT controller adapted to coreboot's polled I2C interface. */
#include <delay.h>
#include <device/i2c_simple.h>
#include <device/mmio.h>
#include <soc/addressmap.h>
#include <timer.h>

#define TWSI2_BASE 0xd4012000
#define TWSI2_CLOCK 0xd4015038
#define TWSI8_BASE 0xd401d800
#define TWSI8_CLOCK 0xd4015020
#define ICR 0x00
#define ISR 0x04
#define IDBR 0x0c
#define START (1U << 0)
#define STOP (1U << 1)
#define NAK (1U << 2)
#define TRANSFER (1U << 3)
#define UNIT_RESET (1U << 10)
#define MASTER_ABORT (1U << 12)
#define UNIT_ENABLE ((1U << 13) | (1U << 14) | (1U << 21))
#define ACK_MISSING (1U << 14)
#define BUS_BUSY ((1U << 15) | (1U << 16))
#define ARB_LOST (1U << 18)
#define TX_EMPTY (1U << 19)
#define RX_FULL (1U << 20)
#define BUS_ERROR (1U << 22)
#define EVENTS (ARB_LOST | TX_EMPTY | RX_FULL | BUS_ERROR)

static uint32_t reg_read(uintptr_t base, unsigned int reg)
{
	return read32((void *)(uintptr_t)(base + reg));
}

static void reg_write(uintptr_t base, unsigned int reg, uint32_t value)
{
	write32((void *)(uintptr_t)(base + reg), value);
}

static int wait_idle(uintptr_t base)
{
	struct stopwatch deadline;
	stopwatch_init_usecs_expire(&deadline, 2000);
	do {
		if (!(reg_read(base, ISR) & BUS_BUSY) && !(reg_read(base, ICR) & TRANSFER))
			return 0;
		udelay(5);
	} while (!stopwatch_expired(&deadline));
	return -1;
}

static int transfer_byte(uintptr_t base, uint8_t *data, bool read, uint32_t flags)
{
	struct stopwatch deadline;
	uint32_t event = read ? RX_FULL : TX_EMPTY;
	reg_write(base, ISR, EVENTS);
	if (!read)
		reg_write(base, IDBR, *data);
	reg_write(base, ICR, UNIT_ENABLE | TRANSFER | flags);
	stopwatch_init_usecs_expire(&deadline, 2000);
	do {
		uint32_t status = reg_read(base, ISR);
		if (status & (ARB_LOST | BUS_ERROR))
			return -1;
		if ((status & event) && !(reg_read(base, ICR) & TRANSFER)) {
			if (!read && (status & ACK_MISSING))
				return -1;
			if (read)
				*data = reg_read(base, IDBR);
			reg_write(base, ISR, event);
			return 0;
		}
		udelay(5);
	} while (!stopwatch_expired(&deadline));
	return -1;
}

int platform_i2c_transfer(unsigned int bus, struct i2c_msg *segments, int count)
{
	if ((bus != 2 && bus != 8) || !segments || count <= 0)
		return -1;
	for (int i = 0; i < count; i++) {
		if (segments[i].slave > 0x7f || !segments[i].len || !segments[i].buf ||
		    (segments[i].flags & ~I2C_M_RD))
			return -1;
	}

	uintptr_t base = bus == 2 ? TWSI2_BASE : TWSI8_BASE;
	uintptr_t clock_reg = bus == 2 ? TWSI2_CLOCK : TWSI8_CLOCK;

	/* TWSI2 and TWSI8 use PLL1 / 78. Keep the shared PLL configuration unchanged. */
	if (read32((void *)(K3_APBS_BASE + 0x100)) != 0x0b330ccc ||
	    (read32((void *)(K3_APBS_BASE + 0x104)) & 0x1ff00) != 0x1cd00 ||
	    read32((void *)(K3_APBS_BASE + 0x108)) != 0xa0558989 ||
	    !(read32((void *)(K3_MPMU_BASE + 0x10)) & (1U << 24)))
		return -1;
	setbits32((void *)(K3_APBS_BASE + 0x104), 1U << 3);
	setbits32((void *)(K3_MPMU_BASE + 0x1024), 1U << 6);
	uint32_t clock = read32((void *)clock_reg) & ~(7U << 4);
	write32((void *)clock_reg, clock | 7);
	udelay(2);
	write32((void *)clock_reg, (clock | 3) & ~(1U << 2));
	reg_write(base, ICR, UNIT_RESET);
	udelay(10);
	reg_write(base, ICR, UNIT_ENABLE);
	reg_write(base, ISR, EVENTS);
	if (wait_idle(base))
		return -1;

	for (int i = 0; i < count; i++) {
		struct i2c_msg *msg = &segments[i];
		bool read = msg->flags & I2C_M_RD;
		uint8_t address = (msg->slave << 1) | read;
		if (transfer_byte(base, &address, false, START))
			goto fail;
		for (unsigned int j = 0; j < msg->len; j++) {
			uint32_t flags = read && j + 1 == msg->len ? NAK : 0;
			if (i + 1 == count && j + 1 == msg->len)
				flags |= STOP;
			if (transfer_byte(base, &msg->buf[j], read, flags))
				goto fail;
		}
	}
	if (wait_idle(base))
		goto fail;
	/* STOP and NAK remain set after the final byte. Release the completed
	 * transfer before handing the controller to the next boot stage.
	 */
	reg_write(base, ICR, UNIT_ENABLE);
	return 0;
fail:
	reg_write(base, ICR, UNIT_ENABLE | MASTER_ABORT | STOP);
	udelay(10);
	reg_write(base, ICR, UNIT_RESET);
	udelay(10);
	reg_write(base, ICR, UNIT_ENABLE);
	return -1;
}
