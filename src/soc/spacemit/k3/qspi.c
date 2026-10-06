/* SPDX-License-Identifier: GPL-2.0-only */
/* Register definitions and initialization adapted from SpacemiT U-Boot
 * e4043c70d6e921238fe43e485f1efc71b30b00d0, drivers/spi/k1x_qspi.c.
 * Copyright (c) 2023, spacemit Corporation.
 */
#include <console/console.h>
#include <delay.h>
#include <device/mmio.h>
#include <soc/addressmap.h>
#include <soc/qspi.h>
#include <spi-generic.h>
#include <timer.h>

#define MCR 0x000
#define MCR_RESET 3U
#define MCR_ENDIAN (3U << 2)
#define MCR_CLEAR_FIFO ((1U << 10) | (1U << 11))
#define MCR_DISABLE (1U << 14)
#define MCR_IDLE_DATA (15U << 16)
#define IPCR 0x008
#define SOCCR 0x024
#define SFAR 0x100
#define SFACR 0x104
#define SMPR 0x108
#define RBCT 0x110
#define SR 0x15c
#define SR_ACTIVE 7U
#define FR 0x160
#define FR_DONE 1U
#define FR_XIP 2U
#define FR_IP_REJECT (1U << 6)
#define RSER 0x164
#define SPTRCLR 0x16c
#define SFA1AD 0x180
#define SFA2AD 0x184
#define SFB1AD 0x188
#define SFB2AD 0x18c
#define RBDR 0x200
#define LUTKEY 0x300
#define LCKCR 0x304
#define LUT_SLOT1 0x320
#define LUT_KEY 0x5af05af0
#define LUT_CMD 1U
#define LUT_ADDR 2U
#define LUT_DUMMY 3U
#define LUT_READ 7U
#define FIFO_BYTES 128U
#define ADDRESS_BYTES 3U
#define ADDRESS_LIMIT (1U << (ADDRESS_BYTES * 8))

static bool initialized;

static void *reg(unsigned int offset)
{
	return (void *)(uintptr_t)(K3_QSPI_BASE + offset);
}

static int wait_idle(void)
{
	struct stopwatch deadline;
	stopwatch_init_usecs_expire(&deadline, 100000);
	while (read32(reg(SR)) & SR_ACTIVE) {
		if (stopwatch_expired(&deadline))
			return -1;
	}
	return 0;
}

static void reset_buffers(void)
{
	uint32_t value = read32(reg(MCR));
	write32(reg(MCR), value | MCR_RESET);
	/* Retained minimum: one AHB and two serial clocks. */
	udelay(1);
	write32(reg(MCR), value & ~MCR_RESET);
}

static int write_command_register(unsigned int offset, uint32_t value)
{
	struct stopwatch deadline;
	stopwatch_init_usecs_expire(&deadline, 100000);
	do {
		write32(reg(offset), value);
		if (!(read32(reg(FR)) & FR_IP_REJECT))
			return 0;
		/* SFAR/RBCT can reject a write; acknowledge and retry boundedly. */
		write32(reg(FR), FR_IP_REJECT);
	} while (!stopwatch_expired(&deadline));
	return -1;
}

int k3_qspi_init(void)
{
	initialized = false;
	if (wait_idle() || (read32(reg(FR)) & FR_XIP))
		return -1;
	reset_buffers();
	setbits32(reg(MCR), MCR_DISABLE);
	write32(reg(SMPR), 0); /* The selected serial clock is below 104 MHz. */
	write32(reg(SOCCR), 8); /* Retained K3 controller timing setting. */
	if (write_command_register(SFAR, K3_QSPI_WINDOW))
		return -1;
	write32(reg(SFACR), 0);
	/* A1 has the reference 64 MiB decode window. Other chip selects are
	 * disabled by equal upper bounds; only CS0 is exposed by this driver.
	 * Commands below are independently limited to 24-bit flash offsets.
	 */
	write32(reg(SFA1AD), K3_QSPI_WINDOW + 64 * MiB);
	write32(reg(SFA2AD), K3_QSPI_WINDOW + 64 * MiB);
	write32(reg(SFB1AD), K3_QSPI_WINDOW + 64 * MiB);
	write32(reg(SFB2AD), K3_QSPI_WINDOW + 64 * MiB);
	clrsetbits32(reg(MCR), MCR_DISABLE, MCR_ENDIAN | MCR_IDLE_DATA);
	if (write_command_register(RBCT, 1U << 8))
		return -1;
	write32(reg(RSER), 0); /* Polled IP FIFO; no DMA or AHB memory reads. */
	write32(reg(FR), UINT32_MAX);
	initialized = true;
	return 0;
}

static int read_command(const uint8_t *command, size_t command_size,
			void *buffer, size_t size)
{
	uint32_t address = 0, lut[4] = { 0 };
	unsigned int address_bytes = 0, dummy_cycles = 0, instruction = 0;
	struct stopwatch deadline;
	uint8_t *out = buffer;
	if (!initialized || !command || !buffer || !size || size > FIFO_BYTES ||
	    !command_size)
		return -1;
	/* Initial boot-media support is deliberately read-only. Unknown
	 * opcodes (including program, erase and write-enable) cannot execute.
	 */
	switch (command[0]) {
	case 0x9f: /* JEDEC ID */
	case 0x05: /* status register */
	case 0x35: /* second status register */
	case 0x15: /* third status register */
		if (command_size != 1)
			return -1;
		break;
	case 0x03: /* normal read */
	case 0x0b: /* fast read */
	case 0x5a: /* SFDP read */
		address_bytes = ADDRESS_BYTES;
		dummy_cycles = command[0] == 0x03 ? 0 : 8;
		if (command_size != 1 + address_bytes + dummy_cycles / 8)
			return -1;
		address = ((uint32_t)command[1] << 16) | ((uint32_t)command[2] << 8) |
			command[3];
		if (size > ADDRESS_LIMIT - address)
			return -1;
		break;
	default:
		return -1;
	}
	if (wait_idle())
		goto fail;
	setbits32(reg(MCR), MCR_CLEAR_FIFO);
	write32(reg(SPTRCLR), (1U << 8) | 1U);
	write32(reg(FR), UINT32_MAX);
	if (write_command_register(SFAR, K3_QSPI_WINDOW + address))
		goto fail;
	/* One serial line, two 16-bit instructions per LUT word. STOP is 0. */
	#define APPEND_LUT(op, arg) do { \
		lut[instruction / 2] |= (((op) << 10) | (arg)) << (16 * (instruction % 2)); \
		instruction++; \
	} while (0)
	APPEND_LUT(LUT_CMD, command[0]);
	if (address_bytes)
		APPEND_LUT(LUT_ADDR, address_bytes * 8);
	if (dummy_cycles)
		APPEND_LUT(LUT_DUMMY, dummy_cycles);
	APPEND_LUT(LUT_READ, 0);
	#undef APPEND_LUT
	write32(reg(LUTKEY), LUT_KEY);
	write32(reg(LCKCR), 2);
	for (size_t i = 0; i < ARRAY_SIZE(lut); i++)
		write32(reg(LUT_SLOT1 + i * 4), lut[i]);
	write32(reg(LUTKEY), LUT_KEY);
	write32(reg(LCKCR), 1);
	write32(reg(FR), UINT32_MAX);
	write32(reg(IPCR), (1U << 24) | size);
	stopwatch_init_usecs_expire(&deadline, 100000);
	while (!(read32(reg(FR)) & FR_DONE)) {
		if ((read32(reg(FR)) & FR_IP_REJECT) || stopwatch_expired(&deadline))
			goto fail;
	}
	if ((read32(reg(FR)) & FR_IP_REJECT) || wait_idle())
		goto fail;
	for (size_t i = 0; i < size; i += 4) {
		uint32_t value = read32(reg(RBDR + i));
		for (size_t j = 0; j < MIN(4, size - i); j++)
			out[i + j] = value >> (8 * j);
	}
	reset_buffers();
	return 0;
fail:
	/* Do not reuse a controller whose completion is unknown. The board
	 * must reinitialize it through its clock/reset sequence before retry.
	 */
	initialized = false;
	printk(BIOS_ERR, "K3 QSPI command %#x failed: SR=%#x FR=%#x\n",
	       command[0], read32(reg(SR)), read32(reg(FR)));
	return -1;
}

static int setup(const struct spi_slave *slave)
{
	if (slave->bus != 0 || slave->cs != 0)
		return -1;
	if (initialized)
		return 0;
	/* Each firmware stage has its own driver state. */
	if (k3_qspi_prepare_clock())
		return -1;
	return k3_qspi_init();
}

static int xfer_vector(const struct spi_slave *slave, struct spi_op *ops, size_t count)
{
	int result = -1;
	if (!ops || !count)
		return -1;
	for (size_t i = 0; i < count; i++)
		ops[i].status = SPI_OP_NOT_EXECUTED;
	if (slave->bus == 0 && slave->cs == 0 && count == 2 &&
	    ops[0].dout && ops[0].bytesout &&
	    !ops[0].din && !ops[0].bytesin && !ops[1].dout && !ops[1].bytesout &&
	    ops[1].din && ops[1].bytesin)
		result = read_command(ops[0].dout, ops[0].bytesout, ops[1].din, ops[1].bytesin);
	for (size_t i = 0; i < count; i++)
		ops[i].status = result ? SPI_OP_FAILURE : SPI_OP_SUCCESS;
	return result;
}

static const struct spi_ctrlr controller = {
	.setup = setup,
	.xfer_vector = xfer_vector,
	.max_xfer_size = FIFO_BYTES,
};

const struct spi_ctrlr_buses spi_ctrlr_bus_map[] = {
	{ .ctrlr = &controller, .bus_start = 0, .bus_end = 0 },
};
const size_t spi_ctrlr_bus_map_count = ARRAY_SIZE(spi_ctrlr_bus_map);
