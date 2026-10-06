/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SOC_SPACEMIT_K3_QSPI_H
#define SOC_SPACEMIT_K3_QSPI_H

#include <types.h>

/* Board pads must be configured before enabling the controller. */
int k3_qspi_prepare_clock(void);
int k3_qspi_init(void);

#endif
