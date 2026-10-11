/* SPDX-License-Identifier: GPL-2.0-only */
#include <ec/google/chromeec/ec.h>
#include <ec/google/chromeec/ec_mmio.h>
#include <mainboard/early_ec.h>
#include <device/mmio.h>
#include <soc/addressmap.h>
#include <soc/espi.h>
#include <console/console.h>
#include <string.h>

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

int pico_itx_enable_usb_hub(void)
{
	/* The board's EC GPIO8 switches the FE1.1S hub's 3.3 V supply.
	 * Resolve and check its name before using the EC's name-based SET.
	 */
	struct ec_params_gpio_get_v1 get = {
		.subcmd = EC_GPIO_GET_INFO,
		.get_info.index = 8,
	};
	struct ec_response_gpio_get_v1 info = {0};
	struct chromeec_command command = {
		.cmd_code = EC_CMD_GPIO_GET,
		.cmd_version = 1,
		.cmd_data_in = &get,
		.cmd_size_in = sizeof(get),
		.cmd_data_out = &info,
		.cmd_size_out = sizeof(info),
	};
	if (google_chromeec_command(&command) ||
	    !(info.get_info.flags & (1U << 9)) ||
	    memcmp(info.get_info.name, "EC_SW_USB2_PWREN1", sizeof("EC_SW_USB2_PWREN1")))
		return -1;
	if (info.get_info.val)
		return 0;
	struct ec_params_gpio_set set = { .val = 1 };
	memcpy(set.name, info.get_info.name, sizeof(set.name));
	command = (struct chromeec_command) {
		.cmd_code = EC_CMD_GPIO_SET,
		.cmd_data_in = &set,
		.cmd_size_in = sizeof(set),
	};
	if (google_chromeec_command(&command))
		return -1;
	struct ec_params_gpio_get verify;
	struct ec_response_gpio_get response;
	memcpy(verify.name, set.name, sizeof(verify.name));
	command = (struct chromeec_command) {
		.cmd_code = EC_CMD_GPIO_GET,
		.cmd_data_in = &verify,
		.cmd_size_in = sizeof(verify),
		.cmd_data_out = &response,
		.cmd_size_out = sizeof(response),
	};
	if (google_chromeec_command(&command) || !response.val)
		return -1;
	printk(BIOS_DEBUG, "K3 USB2 hub: EC 3.3 V supply enabled\n");
	return 0;
}
