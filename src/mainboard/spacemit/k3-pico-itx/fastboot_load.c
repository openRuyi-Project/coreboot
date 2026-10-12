/* SPDX-License-Identifier: GPL-2.0-only */
#include <commonlib/helpers.h>
#include <console/console.h>
#include <console/streams.h>
#include <delay.h>
#include <device/mmio.h>
#include <halt.h>
#include <mainboard/ec_update.h>
#include <mainboard/native_boot.h>
#include <soc/addressmap.h>
#include <soc/usb_device.h>
#include <stdio.h>
#include <string.h>

#define USB_BASE 0xcad00000
#define USB_DMA_BASE 0x107000000ULL
#define USB_PORT_A_ENABLE ((1U << 4) | (1U << 5) | (1U << 6) | (1U << 7))

static int respond(const char *reply)
{
	return k3_usb_device_send(reply, strlen(reply));
}

static const char *ec_phase;
static unsigned int ec_percent;

static int ec_progress(const char *phase, size_t done, size_t total)
{
	unsigned int percent = done * 100 / total;
	char reply[64];

	if (ec_phase && !strcmp(ec_phase, phase) && percent < ec_percent + 10 &&
	    done != total)
		return 0;
	ec_phase = phase;
	ec_percent = percent;
	snprintf(reply, sizeof(reply), "INFOEC %s %u%%", phase, percent);
	return respond(reply);
}

static bool parse_size(const char *text, size_t *size)
{
	if (strlen(text) != 8)
		return false;
	*size = 0;
	for (unsigned int i = 0; i < 8; i++) {
		unsigned int digit;
		if (text[i] >= '0' && text[i] <= '9')
			digit = text[i] - '0';
		else if (text[i] >= 'a' && text[i] <= 'f')
			digit = text[i] - 'a' + 10;
		else if (text[i] >= 'A' && text[i] <= 'F')
			digit = text[i] - 'A' + 10;
		else
			return false;
		*size = (*size << 4) | digit;
	}
	return true;
}

void pico_itx_fastboot_load(void *destination, size_t bytes)
{
	char command[65], reply[64];
	size_t received = 0, expected = 0;
	bool downloading = false, ready = false;
	int status;

	/* This profile follows a BootROM USB download, which enabled Port A.
	 * Keep that PHY/clock setup and reset only the DWC3 device controller.
	 */
	if ((read32((void *)(K3_APMU_BASE + 0x5c)) & USB_PORT_A_ENABLE) != USB_PORT_A_ENABLE)
		die("K3 BootROM USB clock/reset state is unavailable\n");
	status = k3_usb_device_start(USB_BASE, (void *)USB_DMA_BASE);
	if (status)
		die("K3 recovery USB initialization failed\n");
	uint8_t prompt[] = "K3 recovery: USB Fastboot ready for normal-spi.bin\n";
	console_write_line(prompt, sizeof(prompt) - 1);
	char ec_version[K3_EC_VERSION_SIZE];
	if (!pico_itx_ec_version(ec_version)) {
		snprintf(reply, sizeof(reply), "EC firmware: %s\n", ec_version);
		console_write_line((uint8_t *)reply, strlen(reply));
	}
	console_tx_flush();

	for (;;) {
		size_t length = downloading ? MIN(expected - received, (size_t)K3_USB_BULK_BYTES) : 64;
		void *buffer = downloading ? (uint8_t *)destination + received : (uint8_t *)command;
		status = k3_usb_device_receive(buffer, &length);
		if (status == K3_USB_RESET) {
			downloading = ready = false;
			received = 0;
			continue;
		}
		if (status == K3_USB_AGAIN) {
			udelay(10);
			continue;
		}
		if (status)
			die("K3 recovery USB receive failed\n");
		if (downloading) {
			received += length;
			if (received != expected)
				continue;
			downloading = false;
			ready = true;
			status = respond("OKAY");
		} else {
			command[length] = '\0';
			if (!strcmp(command, "getvar:max-download-size")) {
				snprintf(reply, sizeof(reply), "OKAY0x%08x", (unsigned int)bytes);
				status = respond(reply);
			} else if (!strcmp(command, "getvar:product")) {
				status = respond("OKAYk3-pico-itx");
			} else if (!strcmp(command, "getvar:version-bootloader")) {
				status = respond("OKAYcoreboot-k3-ddr");
			} else if (!strcmp(command, "getvar:version-ec")) {
				if (pico_itx_ec_version(ec_version)) {
					status = respond("FAILEC unavailable");
				} else {
					snprintf(reply, sizeof(reply), "OKAY%s", ec_version);
					status = respond(reply);
				}
			} else if (!strcmp(command, "getvar:partition-size:ec")) {
				status = respond("OKAY0x00040000");
			} else if (!strcmp(command, "getvar:partition-type:ec")) {
				status = respond("OKAYraw");
			} else if (!strcmp(command, "getvar:has-slot:ec")) {
				status = respond("OKAYno");
			} else if (!strcmp(command, "getvar:version")) {
				status = respond("OKAY0.4");
			} else if (!strcmp(command, "getvar:is-userspace")) {
				status = respond("OKAYno");
			} else if (!strncmp(command, "download:", 9)) {
				size_t requested;
				ready = false;
				received = 0;
				if (!parse_size(command + 9, &requested) || !requested ||
				    (requested != bytes && requested > K3_EC_UPDATE_SIZE)) {
					status = respond("FAILrecovery payload size mismatch");
				} else {
					expected = requested;
					snprintf(reply, sizeof(reply), "DATA%08x", (unsigned int)expected);
					status = respond(reply);
					downloading = !status;
				}
			} else if (!strcmp(command, "flash:ec")) {
				ec_phase = NULL;
				int result = ready ? pico_itx_ec_update(destination, received,
								       ec_progress) : -1;
				ready = false;
				received = 0;
				status = respond(result ? "FAILEC update failed" :
						 "OKAYEC verified; reboot EC to activate");
			} else if (!strcmp(command, "oem reboot-ec")) {
				ready = false;
				status = respond(pico_itx_ec_reboot() ? "FAILEC reboot failed" :
						 "OKAYWait 15 seconds, then reset the board");
			} else if (!strcmp(command, "continue")) {
				if (!ready || received != bytes) {
					status = respond("FAILdownload recovery payload first");
				} else {
					if (respond("OKAY") || k3_usb_device_stop())
						die("K3 recovery USB handoff failed\n");
					return;
				}
			} else {
				status = respond("FAILunsupported command");
			}
		}
		if (status == K3_USB_RESET) {
			downloading = ready = false;
			received = 0;
		} else if (status) {
			die("K3 recovery USB response failed\n");
		}
	}
}
