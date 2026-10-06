/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef K3_PICO_EARLY_EC_H
#define K3_PICO_EARLY_EC_H
#include <stdint.h>
int pico_itx_request_full_fan(void);
int pico_itx_early_cooling_init(void);
void pico_itx_typec_retimer_init(void);
uint64_t pico_itx_ddr_init(void);
#endif
