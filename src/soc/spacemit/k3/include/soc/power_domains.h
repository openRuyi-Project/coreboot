/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef SPACEMIT_K3_POWER_DOMAINS_H
#define SPACEMIT_K3_POWER_DOMAINS_H
#include <stdbool.h>

/* Matches dt-bindings/power/spacemit,k3-power.h. */
#define K3_PMDOMAIN_VPU		0
#define K3_PMDOMAIN_GPU		1
#define K3_PMDOMAIN_AUDIO	2
#define K3_PMDOMAIN_LCD0	3
#define K3_PMDOMAIN_LCD1	4
#define K3_PMDOMAIN_COUNT	5

bool k3_power_domain_is_on(unsigned int id);
int k3_power_domain_set(unsigned int id, bool on);

#endif
