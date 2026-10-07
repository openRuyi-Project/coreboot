/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SOC_SPACEMIT_K3_CPU_CLOCK_H
#define SOC_SPACEMIT_K3_CPU_CLOCK_H

/* Called after DDR initialization and before the firmware handoff. */
void k3_x100_init(void);

/* The mainboard configures and verifies its CPU regulator in microvolts. */
int mainboard_set_cpu_voltage(unsigned int microvolts);

#endif
