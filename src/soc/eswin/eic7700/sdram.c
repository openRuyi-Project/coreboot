// SPDX-License-Identifier: GPL-2.0-only

/*
 * Register interface and geometry decode from ESWIN's EIC770x U-Boot port.
 * Copyright 2024 Beijing ESWIN Computing Technology Co., Ltd.
 * Original author: Xiang Xu <xuxiang@eswincomputing.com>
 */
#include <commonlib/helpers.h>
#include <console/console.h>
#include <device/mmio.h>
#include <soc/addressmap.h>
#include <soc/sdram.h>
#include <timer.h>

#define DDR_CONFIG_CLOCK 0x124
#define DDR_CONFIG_ENABLE (1U << 9)
#define MSTR 0x10000
#define MRCTRL0 0x10080
#define MRCTRL1 0x10084
#define MRSTAT 0x10090
#define MRRDATA0 0x10094
#define ECCCFG0 0x10600
#define ECC_MODE_MASK 7U
#define ECC_MODE_INLINE 4U
#define MR_START (1U << 31)
#define MR_DONE_CLEAR (1U << 24)
#define MR_RANK_MASK (15U << 4)
#define MR_ADDRESS_MASK (15U << 12)
#define MR_READ 1U
#define MR_BUSY 1U
#define MR_DONE (1U << 16)

static bool wait_mr(uintptr_t base, uint32_t mask, uint32_t expected)
{
	struct stopwatch watch;

	stopwatch_init_usecs_expire(&watch, 1000);
	while ((read32((void *)(base + MRSTAT)) & mask) != expected)
		if (stopwatch_expired(&watch))
			return false;
	return true;
}

static uint32_t read_mr8(uintptr_t base, unsigned int rank)
{
	uint32_t old0, old1, value;

	if (!wait_mr(base, MR_BUSY, 0))
		die("DDR mode-register interface busy");
	old0 = read32((void *)(base + MRCTRL0)) & ~MR_START;
	old1 = read32((void *)(base + MRCTRL1));
	write32((void *)(base + MRCTRL0), old0 | MR_DONE_CLEAR);
	value = (old0 & ~(MR_START | MR_DONE_CLEAR | MR_RANK_MASK |
			 MR_ADDRESS_MASK | MR_READ)) | MR_READ | (1U << (rank + 4));
	write32((void *)(base + MRCTRL0), value);
	write32((void *)(base + MRCTRL1), 8 << 8);
	write32((void *)(base + MRCTRL0), value | MR_START);
	if (!wait_mr(base, MR_BUSY | MR_DONE, MR_DONE))
		die("DDR mode-register read timed out");
	value = read32((void *)(base + MRRDATA0));
	write32((void *)(base + MRCTRL0), old0 | MR_DONE_CLEAR);
	write32((void *)(base + MRCTRL1), old1);
	write32((void *)(base + MRCTRL0), old0);
	return value;
}

static uint64_t channel_size(uintptr_t base)
{
	static const unsigned int density_gbit[] = {2, 3, 4, 6, 8, 12, 16, 24, 32};
	uint32_t mstr = read32((void *)(base + MSTR));
	unsigned int ranks = (mstr >> 24) & 15;
	uint64_t size = 0;

	/* Each EIC7700 controller has a 32-bit channel. */
	if (!ranks || ((mstr >> 12) & 3))
		die("Unsupported DDR rank or bus-width configuration");
	for (unsigned int rank = 0; rank < 4; rank++) {
		if (!(ranks & (1U << rank)))
			continue;
		uint32_t data = read_mr8(base, rank);
		unsigned int mr8 = data & 0xff;
		unsigned int width_code = mr8 >> 6;
		unsigned int density = (mr8 >> 2) & 15;

		if (width_code > 1 || density >= ARRAY_SIZE(density_gbit))
			die("Unsupported DDR MR8 geometry");
		unsigned int width = width_code ? 8 : 16;

		for (unsigned int lane = width; lane < 32; lane += width)
			if (((data >> lane) & 0xff) != mr8)
				die("DDR devices on one channel have different geometry");
		size += (uint64_t)density_gbit[density] * GiB / 8 * (32 / width);
	}
	return size;
}

uint64_t eic7700_dram_size(void)
{
	static uint64_t size;
	uint32_t clock;
	uint32_t ecc0, ecc1;
	uint64_t channel0, channel1;

	if (size)
		return size;
	clock = read32((void *)(EIC7700_SYSCRG + DDR_CONFIG_CLOCK));
	write32((void *)(EIC7700_SYSCRG + DDR_CONFIG_CLOCK), clock | DDR_CONFIG_ENABLE);
	channel0 = channel_size(EIC7700_DDRC0);
	channel1 = channel_size(EIC7700_DDRC1);
	ecc0 = read32((void *)(EIC7700_DDRC0 + ECCCFG0)) & ECC_MODE_MASK;
	ecc1 = read32((void *)(EIC7700_DDRC1 + ECCCFG0)) & ECC_MODE_MASK;
	write32((void *)(EIC7700_SYSCRG + DDR_CONFIG_CLOCK), clock);
	/* SCPU training configures the two channels for symmetric interleave. */
	if (channel0 != channel1 || !channel0 || channel0 > 16ULL * GiB)
		die("Unsupported DDR channel configuration");
	if (ecc0 != ecc1 || (ecc0 && ecc0 != ECC_MODE_INLINE))
		die("Unsupported DDR ECC configuration");
	size = channel0 + channel1;
	/* Inline ECC stores check bits in the highest eighth of physical DDR. */
	if (ecc0 == ECC_MODE_INLINE)
		size -= size / 8;
	return size;
}
