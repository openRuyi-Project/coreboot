/* SPDX-License-Identifier: GPL-2.0-only */
#include <ec/google/chromeec/ec.h>
#include <ec/google/chromeec/ec_mmio.h>
#include <mainboard/early_ec.h>
#include <device/mmio.h>
#include <soc/addressmap.h>
#include <soc/espi.h>

int pico_itx_early_cooling_init(void)
{
	const struct k3_espi_config config = {
		.registers = K3_ESPI_BASE,
		.peripheral_base0 = 0x84000000,
		.peripheral_base1 = K3_ESPI_BASE,
	};
	google_chromeec_mmio_disable();
	/* Pico ITX GPIO91..98: eSPI D0..3, CS, CLK, RESET_N and ALERT.
	 * The retained board description selects GPIO4's 1.8 V domain.
	 */
	/* Match the retained SPL's protected-write recipe. This selector reads
	 * zero even on the working board; EC communication is validated below.
	 */
	write32((void *)K3_IO_POWER_KEY, 0xbaba);
	__sync_synchronize();
	write32((void *)(K3_IO_POWER_KEY + 4), 0xeb10);
	__sync_synchronize();
	write32((void *)(K3_IO_POWER_BASE + K3_IO_POWER_GPIO4), 1U << 2);
	__sync_synchronize();
	for (unsigned int pin = 91; pin <= 98; pin++)
		clrsetbits32((void *)(uintptr_t)(K3_MFPR_BASE + 4 * pin), 0xff77,
			     4 | (1U << 6) | (8U << 9) | (6U << 13));
	if (k3_espi_prepare_clock() || k3_espi_init(&config))
		return -1;
	return pico_itx_request_full_fan();
}

int pico_itx_request_full_fan(void)
{
	struct ec_params_pwm_set_fan_duty_v1 params = { .percent = 100, .fan_idx = 0 };
	uint32_t versions;
	struct chromeec_command request = {
		.cmd_code = EC_CMD_PWM_SET_FAN_DUTY,
		.cmd_data_in = &params,
	};
	if (google_chromeec_get_cmd_versions(EC_CMD_PWM_SET_FAN_DUTY, &versions))
		return -1;
	if (versions & (1U << 1)) {
		request.cmd_version = 1;
		request.cmd_size_in = sizeof(params);
	} else if (versions & 1) {
		request.cmd_size_in = sizeof(struct ec_params_pwm_set_fan_duty_v0);
	} else {
		return -1;
	}
	return google_chromeec_command(&request);
}
