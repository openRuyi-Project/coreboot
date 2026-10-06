/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SOC_SPACEMIT_K3_ESPI_H
#define SOC_SPACEMIT_K3_ESPI_H
#include <types.h>

struct k3_espi_config {
	uintptr_t registers;
	uint32_t peripheral_base0;
	uint32_t peripheral_base1;
};

/* Caller owns pinmux and the 20 MHz single-I/O clock/reset prerequisites. */
int k3_espi_init(const struct k3_espi_config *config);
int k3_espi_prepare_clock(void);
#endif
