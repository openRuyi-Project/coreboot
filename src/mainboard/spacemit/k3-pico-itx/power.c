/* SPDX-License-Identifier: GPL-2.0-only */
#include <delay.h>
#include <device/i2c_simple.h>
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

static int read_word(uint8_t reg, uint16_t *value)
{
	uint8_t data[2];
	if (i2c_read_bytes(CPU_REGULATOR_BUS, CPU_REGULATOR_ADDRESS,
			   reg, data, sizeof(data)))
		return -1;
	*value = data[0] | ((uint16_t)data[1] << 8);
	return 0;
}

int mainboard_set_cpu_voltage(unsigned int microvolts)
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
