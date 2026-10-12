/* SPDX-License-Identifier: GPL-2.0-only */
#include <commonlib/bsd/helpers.h>
#include <console/console.h>
#include <ec/google/chromeec/ec.h>
#include <mainboard/ec_update.h>
#include <string.h>

#define EC_TRANSFER_SIZE 64
#define EC_ERASE_SIZE 0x1000

static int command(uint16_t code, const void *params, size_t params_size,
		   void *response, size_t response_size)
{
	struct chromeec_command cmd = {
		.cmd_code = code,
		.cmd_version = 0,
		.cmd_data_in = params,
		.cmd_size_in = params_size,
		.cmd_data_out = response,
		.cmd_size_out = response_size,
	};

	if (google_chromeec_command(&cmd) || cmd.cmd_size_out != response_size)
		return -1;
	return 0;
}

int pico_itx_ec_version(char version[K3_EC_VERSION_SIZE])
{
	struct ec_response_get_version response;

	if (command(EC_CMD_GET_VERSION, NULL, 0, &response, sizeof(response)))
		return -1;
	const char *active = response.current_image == EC_IMAGE_RO ?
		response.version_string_ro : response.version_string_rw;
	memcpy(version, active, K3_EC_VERSION_SIZE);
	version[K3_EC_VERSION_SIZE - 1] = '\0';
	return version[0] ? 0 : -1;
}

static uint32_t le32(const uint8_t *p)
{
	return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 |
		(uint32_t)p[3] << 24;
}

static bool image_valid(const uint8_t *image, size_t size)
{
	static const uint8_t magic[] = { 0xe1, 0, 'i', 'm', 'a', 'g', 'e', 1 };
	static const char board[] = "SPACEMIT_PICO_ITX";

	if (size < 40 || size > K3_EC_UPDATE_SIZE || size % 4 ||
	    memcmp(image, magic, sizeof(magic)) || le32(image + 12) != size - 32)
		return false;
	/* Accept the packaged MCU image, not a SoC NOR or an EFI image. */
	for (size_t i = 32; i <= size - sizeof(board); i++)
		if (!memcmp(image + i, board, sizeof(board)))
			return true;
	return false;
}

int pico_itx_ec_update(const void *image, size_t size, k3_ec_progress progress)
{
	const uint8_t *data = image;
	struct ec_response_flash_info info;
	char version[K3_EC_VERSION_SIZE];
	uint8_t verify[EC_TRANSFER_SIZE];
	struct {
		struct ec_params_flash_write params;
		uint8_t data[EC_TRANSFER_SIZE];
	} write;

	if (!image || !image_valid(data, size) || pico_itx_ec_version(version))
		return -1;
	if (strncmp(version, "SPACEMIT_PICO_ITX-", sizeof("SPACEMIT_PICO_ITX-") - 1) &&
	    strncmp(version, "SPACEMIT_DEB1-", sizeof("SPACEMIT_DEB1-") - 1))
		return -1;
	if (command(EC_CMD_FLASH_INFO, NULL, 0, &info, sizeof(info)) ||
	    info.flash_size < K3_EC_UPDATE_SIZE || !info.write_block_size ||
	    EC_TRANSFER_SIZE % info.write_block_size || size % info.write_block_size ||
	    !info.erase_block_size || EC_ERASE_SIZE % info.erase_block_size)
		return -1;

	/*
	 * The package updates the first 256 KiB through EC host commands.
	 * Do not route these offsets through the SoC SPI flash driver.
	 */
	printk(BIOS_INFO, "EC update: %s, %zu bytes\n", version, size);
	if (progress && progress("erasing", 0, K3_EC_UPDATE_SIZE))
		return -1;
	for (uint32_t offset = 0; offset < K3_EC_UPDATE_SIZE; offset += EC_ERASE_SIZE) {
		struct ec_params_flash_erase erase = { offset, EC_ERASE_SIZE };
		if (command(EC_CMD_FLASH_ERASE, &erase, sizeof(erase), NULL, 0) ||
		    (progress && progress("erasing", offset + EC_ERASE_SIZE,
					  K3_EC_UPDATE_SIZE)))
			return -1;
	}
	if (progress && progress("writing", 0, size))
		return -1;
	for (size_t offset = 0; offset < size;) {
		size_t count = MIN(size - offset, sizeof(write.data));
		write.params.offset = offset;
		write.params.size = count;
		memcpy(write.data, data + offset, count);
		if (command(EC_CMD_FLASH_WRITE, &write, sizeof(write.params) + count,
			    NULL, 0))
			return -1;
		offset += count;
		if (progress && progress("writing", offset, size))
			return -1;
	}
	if (progress && progress("verifying", 0, size))
		return -1;
	for (size_t offset = 0; offset < size;) {
		size_t count = MIN(size - offset, sizeof(verify));
		struct ec_params_flash_read read = { offset, count };
		if (command(EC_CMD_FLASH_READ, &read, sizeof(read), verify, count) ||
		    memcmp(data + offset, verify, count))
			return -1;
		offset += count;
		if (progress && progress("verifying", offset, size))
			return -1;
	}
	return 0;
}

int pico_itx_ec_reboot(void)
{
	return google_chromeec_reboot(EC_REBOOT_JUMP_RW, 0);
}
