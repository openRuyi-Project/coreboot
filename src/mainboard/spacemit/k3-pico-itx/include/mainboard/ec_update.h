/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MAINBOARD_SPACEMIT_K3_EC_UPDATE_H
#define MAINBOARD_SPACEMIT_K3_EC_UPDATE_H

#include <types.h>

#define K3_EC_VERSION_SIZE 32
#define K3_EC_UPDATE_SIZE 0x40000

typedef int (*k3_ec_progress)(const char *phase, size_t done, size_t total);

int pico_itx_ec_version(char version[K3_EC_VERSION_SIZE]);
int pico_itx_ec_update(const void *image, size_t size, k3_ec_progress progress);
int pico_itx_ec_reboot(void);

#endif
