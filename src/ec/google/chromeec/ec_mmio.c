/* SPDX-License-Identifier: GPL-2.0-only */
#include <console/console.h>
#include <delay.h>
#include <device/mmio.h>
#include <timer.h>
#include "ec.h"
#include "ec_commands.h"
#include "ec_mmio.h"

#define PROTO3_BUFFER_SIZE 268
#define COMMAND_TIMEOUT_US (1 * USECS_PER_SEC)
static uint8_t request[PROTO3_BUFFER_SIZE], response[PROTO3_BUFFER_SIZE];
static uintptr_t command_address, packet_address;
static size_t window_size;

void google_chromeec_mmio_disable(void)
{
	command_address = packet_address = 0;
	window_size = 0;
}

int google_chromeec_mmio_init(uintptr_t command, uintptr_t packet, size_t packet_size)
{
	google_chromeec_mmio_disable();
	if (!command || !packet || packet_size < PROTO3_BUFFER_SIZE ||
	    packet > UINTPTR_MAX - packet_size)
		return -1;
	command_address = command;
	packet_address = packet;
	window_size = packet_size;
	return 0;
}

void *crosec_get_buffer(size_t size, int req)
{
	if (size > PROTO3_BUFFER_SIZE)
		return NULL;
	return req ? request : response;
}

static int wait_ready(void)
{
	struct stopwatch deadline;
	stopwatch_init_usecs_expire(&deadline, COMMAND_TIMEOUT_US);
	do {
		uint8_t status = read8((void *)command_address);
		if (status == 0xff)
			return -1;
		if (!(status & EC_LPC_STATUS_BUSY_MASK))
			return 0;
		udelay(10);
	} while (!stopwatch_expired(&deadline));
	return -1;
}

static int mmio_io(size_t request_size, size_t response_size, void *context)
{
	if (!command_address || request_size > window_size || response_size > window_size ||
	    request_size > sizeof(request) || response_size > sizeof(response) || wait_ready())
		return -1;
	/* The peripheral window supports byte accesses; no unaligned MMIO copies. */
	for (size_t i = 0; i < request_size; i++)
		write8((void *)(packet_address + i), request[i]);
	__sync_synchronize();
	write8((void *)command_address, EC_COMMAND_PROTOCOL_3);
	__sync_synchronize();
	if (wait_ready())
		return -1;
	__sync_synchronize();
	for (size_t i = 0; i < response_size; i++)
		response[i] = read8((void *)(packet_address + i));
	return 0;
}

int google_chromeec_command(struct chromeec_command *command)
{
	if (!command_address)
		return -1;
	return crosec_command_proto(command, mmio_io, NULL);
}

enum host_event_code google_chromeec_get_event(void)
{
	/* ACPI query-event delivery is not part of the MMIO packet transport. */
	return EC_HOST_EVENT_NONE;
}
