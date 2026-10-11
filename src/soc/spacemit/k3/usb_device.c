/* SPDX-License-Identifier: BSD-2-Clause-Patent */
/* DWC3 USB 2.0 transport for the K3 BootROM download port. */
#include <commonlib/endian.h>
#include <commonlib/helpers.h>
#include <console/console.h>
#include <delay.h>
#include <device/mmio.h>
#include <soc/usb_device.h>
#include <string.h>
#include <timer.h>
#include "dwc3.h"

static uint64_t time_us(void)
{
	struct mono_time now, origin;
	mono_time_set_usecs(&origin, 0);
	timer_monotonic_get(&now);
	return mono_time_diff_microseconds(&origin, &now);
}

static void dma_fence(void)
{
	asm volatile("fence iorw, iorw" ::: "memory");
}

#define CACHE_RANGE(name, instruction) \
static void name(const void *buffer, size_t length) \
{ \
	uintptr_t end = (uintptr_t)buffer + length; \
	for (uintptr_t line = (uintptr_t)buffer & ~63UL; line < end; line += 64) \
		asm volatile(".option push\n.option arch, +zicbom\n" instruction " (%0)\n.option pop" \
			     :: "r"(line) : "memory"); \
}
CACHE_RANGE(cache_clean, "cbo.clean")
CACHE_RANGE(cache_invalidate, "cbo.inval")
CACHE_RANGE(cache_flush, "cbo.flush")

typedef enum {
	setup, data_in, wait_status_in, wait_status_out, status_in, status_out
} CONTROL_STATE;
static uintptr_t m_base;
static uint8_t *m_dma;
static struct dwc3_trb *m_trb;
static uint8_t *m_setup, *m_control, *m_rx, *m_tx;
static uint32_t m_event_offset, m_generation, m_seen_generation;
static uint8_t m_resource[4], m_configuration;
static bool m_started, m_rx_ready, m_tx_busy;
static size_t m_rx_bytes;
static size_t m_rx_transfer_bytes;
static uint16_t m_bulk_packet;
static int m_error;
static CONTROL_STATE m_control_state;
// separate pages avoid invalidating unrelated CPU-owned data.
#define DMA_PAGES 12
#define COMMAND_TIMEOUT_US 1000
#define HALT_TIMEOUT_US 100000
#define TRANSFER_TIMEOUT_NS 2000000000ULL

static const uint8_t m_device[] = {
	18, 1, 0, 2, 0, 0, 0, 64, 0x1c, 0x36, 1, 0x10, 0, 1, 1, 2, 0, 1
};
static const uint8_t m_config[] = {
	9, 2, 32, 0, 1, 1, 0, 0x80, 50,
	9, 4, 0, 0, 2, 0xff, 0x42, 3, 0,
	7, 5, 1, 2, 0, 2, 0,
	7, 5, 0x81, 2, 0, 2, 0
};
static const uint8_t m_qualifier[] = { 10, 6, 0, 2, 0, 0, 0, 64, 1, 0 };

static uint32_t read(uint32_t offset)
{
	return read32((void *)(m_base + offset));
}

static void write(uint32_t offset, uint32_t value)
{
	write32((void *)(m_base + offset), value);
}

static int wait(uint32_t offset, uint32_t mask, uint32_t value, size_t usec)
{
	uint64_t start = time_us();

	do {
		if ((read(offset) & mask) == value) {
			return 0;
		}

		udelay(1);
	} while (((time_us() - start) * 1000ULL) < usec * 1000ULL);

	return K3_USB_TIMEOUT;
}

static int command(size_t ep, uint32_t cmd, uint32_t p0, uint32_t p1, uint32_t p2)
{
	int status;

	write(DEPCMDPAR2(ep), p2);
	write(DEPCMDPAR1(ep), p1);
	write(DEPCMDPAR0(ep), p0);
	write(DEPCMD(ep), cmd | CMD_ACTIVE);
	status = wait(DEPCMD(ep), CMD_ACTIVE, 0, COMMAND_TIMEOUT_US);
	if (!status && (read(DEPCMD(ep)) & CMD_STATUS)) {
		status = K3_USB_ERROR;
	}

	if (status) {
		printk(BIOS_ERR,
			"DWC3 command failed: ep=%zu cmd=%x result=%08x status=%d\r\n",
			ep,
			cmd,
			read(DEPCMD(ep)),
			status
			);
	}

	return status;
}

static int configure(size_t ep, size_t packet, bool modify)
{
	uint32_t p0 = (uint32_t)packet << 3;
	int status;

	if (ep >= 2) {
		// bulk endpoint type.
		p0 |= 2U << 1;
	}

	if (ep & 1) {
		p0 |= (uint32_t)(ep / 2) << 17;
	}

	if (modify) {
		p0 |= 2U << 30;
	}

	status = command(ep, CMD_CONFIG, p0, ((uint32_t)ep << 25) | (1U << 8) | (1U << 10), 0);
	if (status || modify) {
		return status;
	}

	return command(ep, CMD_RESOURCE, 1, 0, 0);
}

static int start(size_t ep, void *buffer, size_t size, size_t type)
{
	struct dwc3_trb *trb = &m_trb[ep];
	int status;
	uint64_t address = (size_t)buffer, trb_address = (size_t)trb;

	if (m_resource[ep]) {
		return K3_USB_BUSY;
	}

	if (size) {
		if (ep & 1) {
			cache_clean(buffer, size);
		} else {
			cache_flush(buffer, size);
		}
	}

	trb->low = (uint32_t)address;
	trb->high = (uint32_t)(address >> 32);
	trb->size = (uint32_t)size;
	trb->control = TRB_HWO | TRB_LAST | TRB_ISP | TRB_IOC | ((uint32_t)type << 4);
	cache_clean(trb, sizeof(*trb));
	dma_fence();
	status = command(ep, CMD_START, (uint32_t)(trb_address >> 32), (uint32_t)trb_address, 0);
	if (!status) {
		m_resource[ep] = (uint8_t)((read(DEPCMD(ep)) >> 16) & 0x7f);
	}

	return status;
}

static int end(size_t ep)
{
	int status = 0;

	if (m_resource[ep]) {
		status = command(ep, CMD_END | (1U << 11) | ((uint32_t)m_resource[ep] << 16), 0, 0, 0);
		if (!status) {
			m_resource[ep] = 0;
		}
	}

	return status;
}

static int arm_setup(void)
{
	m_control_state = setup;
	return start(0, m_setup, 8, TRB_SETUP);
}

static int stall_control(void)
{
	int status = end(1);

	if (!status) {
		status = end(0);
	}

	if (!status) {
		status = command(0, CMD_STALL, 0, 0, 0);
	}

	return status ? status : arm_setup();
}

static int set_configuration(uint8_t configuration)
{
	int status;

	for (size_t ep = 2; ep < 4; ep++) {
		status = end(ep);
		if (status) {
			return status;
		}
	}

	write(DALEPENA, 3);
	m_configuration = 0;
	m_rx_ready = m_tx_busy = false;
	m_generation++;
	if (!configuration) {
		return 0;
	}

	status = command(0, CMD_NEW | (2U << 16), 0, 0, 0);
	for (size_t ep = 2; !status && ep < 4; ep++) {
		status = configure(ep, m_bulk_packet, false);
	}

	if (status) {
		return status;
	}

	write(DALEPENA, 15);
	m_configuration = configuration;
	// arm OUT only when the consumer provides its next receive length.
	return 0;
}

static int setup_request(void)
{
	uint8_t request_type, request;
	uint16_t value, index, length;
	const void *data = NULL;
	size_t bytes = 0;
	int status;

	cache_invalidate(m_setup, 64);
	dma_fence();
	request_type = m_setup[0];
	request = m_setup[1];
	value = read_le16((uint16_t *)(m_setup + 2));
	index = read_le16((uint16_t *)(m_setup + 4));
	length = read_le16((uint16_t *)(m_setup + 6));
	if ((request_type == 0x80) && (request == 6)) {
		switch (value >> 8) {
			case 1: if ((value & 0xff) == 0) {
					data = m_device;
					bytes = sizeof(m_device);
			}

				break;
			case 2:
			case 7:
				if ((value & 0xff) == 0) {
					uint16_t packet = (value >> 8) == 2 ? m_bulk_packet : (m_bulk_packet == 512 ? 64 : 512);
					memcpy(m_control, m_config, sizeof(m_config));
					m_control[1] = (uint8_t)(value >> 8);
					write_le16((uint16_t *)(m_control + 22), packet);
					write_le16((uint16_t *)(m_control + 29), packet);
					data = m_control;
					bytes = sizeof(m_config);
				}

				break;
			case 6: if ((value & 0xff) == 0) {
					data = m_qualifier;
					bytes = sizeof(m_qualifier);
			}

				break;
			case 3:
				if ((value & 0xff) == 0) {
					m_control[0] = 4;
					m_control[1] = 3;
					m_control[2] = 9;
					m_control[3] = 4;
					data = m_control;
					bytes = 4;
				} else if (((value & 0xff) <= 2) && (index == 0x409)) {
					const char *text = (value & 0xff) == 1 ? "SpacemiT" : "K3 DDR fastboot";
					bytes = 2 + 2 * strlen(text);
					m_control[0] = (uint8_t)bytes;
					m_control[1] = 3;
					for (size_t i = 0; text[i]; i++) {
						m_control[2 + 2 * i] = text[i];
						m_control[3 + 2 * i] = 0;
					}

					data = m_control;
				}

				break;
			default: break;
		}
	} else if ((request_type == 0) && (request == 5) && (value <= 127) && !index && !length) {
		write(DCFG, (read(DCFG) & ~(0x7fU << 3)) | ((uint32_t)value << 3));
		m_control_state = wait_status_in;
		return 0;
	} else if ((request_type == 0) && (request == 9) && (value <= 1) && !index && !length) {
		status = set_configuration((uint8_t)value);
		if (status) {
			return status;
		}

		m_control_state = wait_status_in;
		return 0;
	} else if ((request_type == 0x80) && (request == 8) && !value && !index && (length == 1)) {
		m_control[0] = m_configuration;
		data = m_control;
		bytes = 1;
	} else if (((request_type == 0x80) || (request_type == 0x81) || (request_type == 0x82)) &&
						(request == 0) && !value && (length == 2))
	{
		m_control[0] = m_control[1] = 0;
		data = m_control;
		bytes = 2;
	} else if ((request_type == 0x81) && (request == 10) && !value && !index && (length == 1)) {
		m_control[0] = 0;
		data = m_control;
		bytes = 1;
	} else if ((request_type == 1) && (request == 11) && !value && !index && !length) {
		m_control_state = wait_status_in;
		return 0;
	} else if ((request_type == 2) && (request == 1) && !value && !length &&
						((index == 1) || (index == 0x81)))
	{
		status = command(index == 1 ? 2 : 3, CMD_UNSTALL, 0, 0, 0);
		m_control_state = wait_status_in;
		return status;
	}

	if ((data == NULL) || !length) {
		return stall_control();
	}

	bytes = MIN(bytes, length);
	if (data != m_control) {
		memcpy(m_control, data, bytes);
	}

	m_control_state = data_in;
	return start(1, m_control, bytes, TRB_DATA);
}

static int event(uint32_t value)
{
	int status;

	if (value & 1) {
		size_t type = (value >> 8) & 0xf;
		if ((type == 0) || (type == 1)) {
			// disconnect or USB reset invalidates a download.
			// DWC3 keeps EP0's setup transfer across bus reset. stopping and
			// immediately restarting it here races the controller reset phase.
			// non-control transfers belong to the configuration being discarded.
			for (size_t ep = 2; ep < 4; ep++) {
				status = end(ep);
				if (status) {
					return status;
				}
			}

			m_configuration = 0;
			m_rx_ready = m_tx_busy = false;
			m_generation++;
			write(DALEPENA, 3);
			write(DCFG, read(DCFG) & ~(0x7fU << 3));
			return 0;
		}

		if (type == 2) {
			// USB 2.0 transport supports high- and full-speed negotiation.
			uint32_t speed = read(DSTS) & 7;
			if (speed && (speed != 1) && (speed != 3)) {
				return K3_USB_ERROR;
			}

			m_bulk_packet = speed == 0 ? 512 : 64;
			status = configure(0, 64, true);
			return status ? status : configure(1, 64, true);
		}

		return type == 9 ? K3_USB_ERROR : 0;
	}

	size_t ep = (value >> 1) & 0x1f, type = (value >> 6) & 0xf;

	if (ep >= 4) {
		return 0;
	}

	if (type == 1) {
		// transfer complete.
		m_resource[ep] = 0;
		cache_invalidate(&m_trb[ep], sizeof(m_trb[ep]));
		dma_fence();
		if ((ep == 0) && (m_control_state == setup)) {
			return setup_request();
		}

		if ((ep == 1) && (m_control_state == data_in)) {
			m_control_state = wait_status_out;
		} else if (((ep == 0) && (m_control_state == status_out)) ||
							((ep == 1) && (m_control_state == status_in)))
		{
			return arm_setup();
		} else if (ep == 2) {
			uint32_t residual = m_trb[2].size & 0xffffff;
			if (residual > m_rx_transfer_bytes) {
				return K3_USB_ERROR;
			}

			m_rx_bytes = m_rx_transfer_bytes - residual;
			cache_invalidate(m_rx, BULK_BYTES);
			dma_fence();
			m_rx_ready = true;
		} else if (ep == 3) {
			m_tx_busy = false;
		}
	} else if (type == 3) {
		// transfer not ready: host entered the status phase.
		if ((ep == 1) && (m_control_state == wait_status_in)) {
			m_control_state = status_in;
			return start(1, m_control, 0, TRB_STATUS2);
		}

		if ((ep == 0) && (m_control_state == wait_status_out)) {
			m_control_state = status_out;
			return start(0, m_control, 0, TRB_STATUS3);
		}
	}

	return 0;
}

int k3_usb_device_poll(void)
{
	if (!m_started) {
		return K3_USB_ERROR;
	}

	if (m_error) {
		return m_error;
	}

	// bound each call even if the host continuously produces events.
	for (size_t i = 0; i < EVENT_BYTES / 4; i++) {
		uint32_t count = read(GEVNTCOUNT) & 0xffff;
		if (!count) {
			break;
		}

		if ((count > EVENT_BYTES) || (count & 3)) {
			printk(BIOS_ERR, "DWC3 invalid event count: %08x\r\n", count);
			m_error = K3_USB_ERROR;
			break;
		}

		cache_invalidate(m_dma + m_event_offset, 4);
		dma_fence();
		uint32_t value = read_le32((uint32_t *)(m_dma + m_event_offset));
		m_event_offset = (m_event_offset + 4) % EVENT_BYTES;
		write(GEVNTCOUNT, 4);
		m_error = event(value);
		if (m_error) {
			printk(BIOS_ERR,
				"DWC3 event failed: event=%08x control=%u DSTS=%08x status=%d\r\n",
				value,
				m_control_state,
				read(DSTS),
				m_error
				);
			break;
		}
	}

	if (!m_error && (m_seen_generation != m_generation)) {
		m_seen_generation = m_generation;
		return K3_USB_RESET;
	}

	return m_error;
}

int k3_usb_device_stop(void)
{
	if (m_dma == NULL) {
		return 0;
	}

	// A host that stopped reading can leave bulk IN active indefinitely.
	// end transfers while the endpoint command interface is still running.
	for (size_t ep = 0; ep < ARRAY_SIZE(m_resource); ep++) {
		end(ep);
	}

	write(DCTL, read(DCTL) & ~RUN_STOP);
	if (wait(DSTS, CORE_HALTED, CORE_HALTED, HALT_TIMEOUT_US)) {
		m_error = K3_USB_TIMEOUT;
		m_started = false;
		printk(BIOS_ERR, "DWC3 did not halt; DMA buffer retained.\r\n");
		return K3_USB_TIMEOUT;
	}

	write(DEVTEN, 0);
	write(GEVNTSIZ, (1U << 31));
	write(GEVNTCOUNT, read(GEVNTCOUNT) & 0xffff);
	write(GEVNTADRLO, 0);
	write(GEVNTADRHI, 0);
	m_dma = NULL;
	m_started = false;
	return 0;
}

int k3_usb_device_start(uintptr_t base, void *dma)
{
	int status;

	if (m_dma != NULL) {
		return K3_USB_BUSY;
	}

	m_base = base;
	if (!dma || ((uintptr_t)dma & 4095)) {
		return K3_USB_INVALID;
	}

	if ((read(GSNPSID) & 0xffff0000U) != 0x55330000U) {
		return K3_USB_ERROR;
	}

	write(DCTL, (read(DCTL) & ~RUN_STOP) | CORE_RESET);
	status = wait(DCTL, CORE_RESET, 0, HALT_TIMEOUT_US);
	if (status) {
		return status;
	}

	m_dma = dma;

	memset(m_dma, 0, DMA_PAGES * 4096);
	m_trb = (void *)(m_dma + 4096);
	m_setup = m_dma + 2 * 4096;
	m_control = m_dma + 3 * 4096;
	m_rx = m_dma + 4 * 4096;
	m_tx = m_dma + 8 * 4096;
	memset(m_resource, 0, sizeof(m_resource));
	m_event_offset = 0;
	m_configuration = 0;
	m_rx_ready = m_tx_busy = false;
	m_bulk_packet = 512;
	m_generation++;
	m_seen_generation = m_generation;
	m_error = 0;
	cache_flush(m_dma, DMA_PAGES * 4096);
	write(GCTL, (read(GCTL) & ~(3U << 12)) | (2U << 12));
	// retained K3 DT requires these PHY suspend/low-power quirks.
	write(GUSB2PHYCFG, read(GUSB2PHYCFG) & ~((1U << 6) | (1U << 8)));
	write(GUSB3PIPECTL, read(GUSB3PIPECTL) & ~(1U << 17));
	write(DCFG, read(DCFG) & ~((0x7fU << 3) | 7U | (1U << 22)));
	write(GEVNTADRLO, (uint32_t)(size_t)m_dma);
	write(GEVNTADRHI, (uint32_t)((uint64_t)(size_t)m_dma >> 32));
	write(GEVNTSIZ, EVENT_BYTES | (1U << 31));
	write(GEVNTCOUNT, read(GEVNTCOUNT) & 0xffff);
	write(DEVTEN, (1U << 0) | (1U << 1) | (1U << 2) | (1U << 9));
	status = command(0, CMD_NEW, 0, 0, 0);
	for (size_t ep = 0; !status && ep < 2; ep++) {
		status = configure(ep, 64, false);
	}

	write(DALEPENA, 3);
	if (!status) {
		status = arm_setup();
	}

	if (!status) {
		m_started = true;
		write(DCTL, read(DCTL) | RUN_STOP);
		status = wait(DSTS, CORE_HALTED, 0, HALT_TIMEOUT_US);
	}

	if (status) {
		k3_usb_device_stop();
	}

	return status;
}

int k3_usb_device_receive(void *buffer, size_t *length)
{
	int status = k3_usb_device_poll();

	if (status) {
		return status;
	}

	if (!m_configuration) {
		return K3_USB_AGAIN;
	}

	if (!m_rx_ready) {
		if (!m_resource[2]) {
			if (!*length || (*length > BULK_BYTES)) {
				return K3_USB_INVALID;
			}

			// DWC3 OUT lengths must be whole packets. A short final USB packet
			// completes an odd-sized tail; a packet-aligned tail completes at the
			// requested length without requiring an extra host zero-length packet.
			m_rx_transfer_bytes = (*length + m_bulk_packet - 1) & ~(size_t)(m_bulk_packet - 1);
			status = start(2, m_rx, m_rx_transfer_bytes, TRB_NORMAL);
			if (status) {
				return status;
			}
		}

		return K3_USB_AGAIN;
	}

	if (*length < m_rx_bytes) {
		*length = m_rx_bytes;
		return K3_USB_INVALID;
	}

	memcpy(buffer, m_rx, m_rx_bytes);
	*length = m_rx_bytes;
	m_rx_ready = false;
	return 0;
}

int k3_usb_device_send(const void *buffer, size_t length)
{
	int status;
	uint32_t generation = m_generation;
	uint64_t begin;

	if (!m_started || !m_configuration) {
		return K3_USB_AGAIN;
	}

	if ((length > BULK_BYTES) || m_tx_busy) {
		return K3_USB_INVALID;
	}

	memcpy(m_tx, buffer, length);
	m_tx_busy = true;
	status = start(3, m_tx, length, TRB_NORMAL);
	if (status) {
		m_tx_busy = false;
		return status;
	}

	begin = time_us();
	while (m_tx_busy) {
		status = k3_usb_device_poll();
		if (status) {
			return status;
		}

		if (generation != m_generation) {
			return K3_USB_RESET;
		}

		if (((time_us() - begin) * 1000ULL) >= TRANSFER_TIMEOUT_NS) {
			return K3_USB_TIMEOUT;
		}

		udelay(10);
	}

	return 0;
}
