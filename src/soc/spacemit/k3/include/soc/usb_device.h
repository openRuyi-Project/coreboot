/* SPDX-License-Identifier: BSD-2-Clause-Patent */
#ifndef SOC_SPACEMIT_K3_USB_DEVICE_H
#define SOC_SPACEMIT_K3_USB_DEVICE_H
#include <types.h>

#define K3_USB_DMA_BYTES (12 * 4096)
#define K3_USB_BULK_BYTES (16 * 1024)
enum k3_usb_status {
	K3_USB_AGAIN = -1,
	K3_USB_RESET = -2,
	K3_USB_TIMEOUT = -3,
	K3_USB_ERROR = -4,
	K3_USB_INVALID = -5,
	K3_USB_BUSY = -6,
};

/* DMA memory must be page-aligned and remain reserved until stop succeeds. */
int k3_usb_device_start(uintptr_t base, void *dma);
int k3_usb_device_stop(void);
int k3_usb_device_poll(void);
int k3_usb_device_receive(void *buffer, size_t *length);
int k3_usb_device_send(const void *buffer, size_t length);
#endif
