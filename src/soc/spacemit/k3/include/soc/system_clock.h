/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SOC_SPACEMIT_K3_SYSTEM_CLOCK_H
#define SOC_SPACEMIT_K3_SYSTEM_CLOCK_H

/* Set system bus rates after DDR training, before raising the CPU rate. */
int k3_system_clock_init(void);

#endif
