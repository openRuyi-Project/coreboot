/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef GOOGLE_CHROMEEC_MMIO_H
#define GOOGLE_CHROMEEC_MMIO_H
#include <types.h>

/* The platform must initialize and validate its bus before publishing these. */
int google_chromeec_mmio_init(uintptr_t command, uintptr_t packet, size_t packet_size);
void google_chromeec_mmio_disable(void);
#endif
