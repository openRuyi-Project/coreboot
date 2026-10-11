/* SPDX-License-Identifier: GPL-2.0-only */
#include <delay.h>
#include <commonlib/endian.h>
#include <console/console.h>
#include <device/i2c_simple.h>
#include <mainboard/power.h>
#include <soc/cpu_clock.h>
#include <string.h>

#define CPU_REGULATOR_BUS 8
#define CPU_REGULATOR_ADDRESS 0x30
#define PMBUS_OPERATION 0x01
#define PMBUS_VOUT_MODE 0x20
#define PMBUS_VOUT_COMMAND 0x21
#define PMBUS_VOUT_SCALE_LOOP 0x29
#define PMBUS_READ_VOUT 0x8b
#define PMBUS_MFR_MODEL 0x9a
#define PMBUS_MFR_CTRL_VOUT 0xd1
#define CPU_FEEDBACK_SCALE 0x029b
#define AU4562_ADDRESS 0x75
#define PMBUS_PAGE 0x00
#define AU4562_X100_PAGE 1
#define AU4562_A100_PAGE 0
#define P1_ADDRESS 0x41
#define P1_EXTERNAL_SLEEP_CONTROL 0x90
#define P1_EXTERNAL_SLEEP_SHUTDOWN ((1U << 1) | (1U << 2) | (1U << 3))

enum cpu_regulator {
	CPU_REGULATOR_MPQ8645P,
	CPU_REGULATOR_AU4562,
	CPU_REGULATOR_UNKNOWN,
};

static enum cpu_regulator cpu_regulator;

void pico_itx_parse_power_config(const uint8_t *eeprom, size_t size)
{
	/* Older DEB1 records omit PMIC Type and use the MPQ regulator. */
	cpu_regulator = CPU_REGULATOR_MPQ8645P;
	if (!eeprom || size < 11 || memcmp(eeprom, "TlvInfo\0", 8) || eeprom[8] != 1)
		return;
	size_t end = 11 + read_be16(eeprom + 9);
	if (end < 17 || end > size || eeprom[end - 6] != 0xfe || eeprom[end - 5] != 4)
		return;
	uint32_t crc = 0xffffffff;
	for (size_t i = 0; i < end - 4; i++) {
		crc ^= eeprom[i];
		for (unsigned int bit = 0; bit < 8; bit++)
			crc = (crc >> 1) ^ (0xedb88320U & -(crc & 1));
	}
	if (~crc != read_be32(eeprom + end - 4))
		return;
	for (size_t pos = 11; pos + 2 <= end - 6; pos += 2 + eeprom[pos + 1]) {
		size_t len = eeprom[pos + 1];
		if (!eeprom[pos] || eeprom[pos] == 0xff || pos + 2 + len > end - 6)
			return;
		if (eeprom[pos] != 0x80)
			continue;
		const uint8_t *name = eeprom + pos + 2;
		if (len >= 9 && !memcmp(name, "spacemit,", 9)) {
			name += 9;
			len -= 9;
		}
		if (len == 6 && !memcmp(name, "au4562", len))
			cpu_regulator = CPU_REGULATOR_AU4562;
		else if (len != 7 || memcmp(name, "mpq8655", len))
			cpu_regulator = CPU_REGULATOR_UNKNOWN;
	}
}

static int read_word(uint8_t reg, uint16_t *value)
{
	uint8_t data[2];
	if (i2c_read_bytes(CPU_REGULATOR_BUS, CPU_REGULATOR_ADDRESS,
			   reg, data, sizeof(data)))
		return -1;
	*value = data[0] | ((uint16_t)data[1] << 8);
	return 0;
}

static int mpq8645p_set_voltage(unsigned int microvolts)
{
	/* MPQ8645P model block in the device's PMBus byte order. */
	static const uint8_t model[] = {8, 'P', '5', '4', '6', '8', 'Q', 'P', 'M'};
	uint8_t identity[sizeof(model)], operation, mode, control;
	uint16_t value;

	if (microvolts < 600000 || microvolts > 1000000 || microvolts % 2000)
		return -1;
	if (i2c_read_bytes(CPU_REGULATOR_BUS, CPU_REGULATOR_ADDRESS,
			   PMBUS_MFR_MODEL, identity, sizeof(identity)) ||
	    memcmp(identity, model, sizeof(model)) ||
	    i2c_readb(CPU_REGULATOR_BUS, CPU_REGULATOR_ADDRESS, PMBUS_OPERATION,
		      &operation) || operation != 0x80 ||
	    i2c_readb(CPU_REGULATOR_BUS, CPU_REGULATOR_ADDRESS, PMBUS_VOUT_MODE,
		      &mode) || mode != 0x40 ||
	    i2c_readb(CPU_REGULATOR_BUS, CPU_REGULATOR_ADDRESS, PMBUS_MFR_CTRL_VOUT,
		      &control) || (control & 3))
		return -1;

	/* The board's external feedback ratio is 2/3, encoded in thousandths.
	 * Set the command/telemetry scale before VOUT_COMMAND latches it.
	 * Leave the physical divider selection and MTP state unchanged.
	 */
	uint8_t scale[] = {PMBUS_VOUT_SCALE_LOOP, CPU_FEEDBACK_SCALE & 0xff,
			   CPU_FEEDBACK_SCALE >> 8};
	if (i2c_write_raw(CPU_REGULATOR_BUS, CPU_REGULATOR_ADDRESS,
			  scale, sizeof(scale)) ||
	    read_word(PMBUS_VOUT_SCALE_LOOP, &value) || value != CPU_FEEDBACK_SCALE)
		return -1;

	/* MPQ8645P VOUT_COMMAND is 2 mV/LSB. */
	uint16_t target = microvolts / 2000;
	uint8_t command[] = {PMBUS_VOUT_COMMAND, target, target >> 8};
	if (i2c_write_raw(CPU_REGULATOR_BUS, CPU_REGULATOR_ADDRESS,
			  command, sizeof(command)) ||
	    read_word(PMBUS_VOUT_COMMAND, &value) || value != target)
		return -1;

	/* READ_VOUT uses a different scale: 1.25 mV/LSB. */
	for (unsigned int attempt = 0; attempt < 100; attempt++) {
		if (read_word(PMBUS_READ_VOUT, &value))
			return -1;
		unsigned int uv = (value & 0x1fff) * 1250;
		if (uv >= microvolts - 25000 && uv <= microvolts + 25000)
			return 0;
		udelay(100);
	}
	return -1;
}

static int au4562_set_voltage(uint8_t output, unsigned int microvolts)
{
	uint8_t page, saved_page, data[2];
	int ret = -1;

	if (microvolts < 540000 || microvolts > 1000000 || microvolts % 5000)
		return -1;
	if (i2c_readb(CPU_REGULATOR_BUS, AU4562_ADDRESS, PMBUS_PAGE, &saved_page))
		return -1;
	/* ADCDC1 supplies X100 on page 1; page 0 supplies A100. */
	if (i2c_writeb(CPU_REGULATOR_BUS, AU4562_ADDRESS, PMBUS_PAGE, output))
		goto restore_page;
	if (i2c_readb(CPU_REGULATOR_BUS, AU4562_ADDRESS, PMBUS_PAGE, &page) ||
	    page != output)
		goto restore_page;
	/* AU4562 VOUT_COMMAND uses 5 mV steps, independently of MPQ encoding. */
	uint16_t target = microvolts / 5000;
	uint8_t command[] = {PMBUS_VOUT_COMMAND, target, target >> 8};
	if (i2c_write_raw(CPU_REGULATOR_BUS, AU4562_ADDRESS, command, sizeof(command)) ||
	    i2c_read_bytes(CPU_REGULATOR_BUS, AU4562_ADDRESS, PMBUS_VOUT_COMMAND,
			   data, sizeof(data)) || read_le16(data) != target)
		goto restore_page;
	printk(BIOS_DEBUG, "K3 AU4562: %s VOUT_COMMAND=%#x (%u mV)\n",
	       output == AU4562_X100_PAGE ? "X100" : "A100", target, microvolts / 1000);
	ret = 0;
restore_page:
	if (i2c_writeb(CPU_REGULATOR_BUS, AU4562_ADDRESS, PMBUS_PAGE, saved_page))
		return -1;
	return ret;
}

static int p1_update_bits(uint8_t reg, uint8_t mask, uint8_t value)
{
	uint8_t old, updated;

	if (i2c_readb(CPU_REGULATOR_BUS, P1_ADDRESS, reg, &old))
		return -1;
	updated = (old & ~mask) | value;
	if (updated == old)
		return 0;
	if (i2c_writeb(CPU_REGULATOR_BUS, P1_ADDRESS, reg, updated) ||
	    i2c_readb(CPU_REGULATOR_BUS, P1_ADDRESS, reg, &old))
		return -1;
	return (old & mask) == value ? 0 : -1;
}

int pico_itx_power_init(void)
{
	/* P1 rails marked regulator-boot-on on both boards: BUCK1..6,
	 * ALDO1/2/4 and DLDO1..7. ALDO3 is not a boot rail. Like SPL,
	 * retain each rail's voltage when no init voltage is specified.
	 */
	static const uint8_t boot_rails[] = {
		0x47, 0x4a, 0x4d, 0x50, 0x53, 0x56,
		0x5b, 0x5e, 0x64,
		0x67, 0x6a, 0x6d, 0x70, 0x73, 0x76, 0x79,
	};

	for (size_t i = 0; i < ARRAY_SIZE(boot_rails); i++) {
		if (p1_update_bits(boot_rails[i], 1, 1)) {
			printk(BIOS_ERR, "K3 P1: boot rail enable failed at %#x\n", boot_rails[i]);
			return -1;
		}
	}
	/* Preserve X100/A100/external supply enables across standby. */
	if (p1_update_bits(P1_EXTERNAL_SLEEP_CONTROL, P1_EXTERNAL_SLEEP_SHUTDOWN, 0))
		return -1;
	/* DEB1 has a fixed A100 supply. Pico ITX uses AU4562 page 0. */
	if (cpu_regulator == CPU_REGULATOR_AU4562 &&
	    au4562_set_voltage(AU4562_A100_PAGE, 950000)) {
		printk(BIOS_ERR, "K3 AU4562: A100 voltage setup failed\n");
		return -1;
	}
	return 0;
}

int mainboard_set_cpu_voltage(unsigned int microvolts)
{
	int ret;

	switch (cpu_regulator) {
	case CPU_REGULATOR_MPQ8645P:
		ret = mpq8645p_set_voltage(microvolts);
		break;
	case CPU_REGULATOR_AU4562:
		ret = au4562_set_voltage(AU4562_X100_PAGE, microvolts);
		break;
	default:
		printk(BIOS_ERR, "K3 CPU regulator: unsupported EEPROM PMIC Type\n");
		return -1;
	}
	if (ret)
		printk(BIOS_ERR, "K3 CPU regulator: %s voltage setup failed\n",
		       cpu_regulator == CPU_REGULATOR_AU4562 ? "AU4562" : "MPQ8645P");
	return ret;
}
