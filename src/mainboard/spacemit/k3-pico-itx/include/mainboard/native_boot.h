/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MAINBOARD_SPACEMIT_K3_NATIVE_BOOT_H
#define MAINBOARD_SPACEMIT_K3_NATIVE_BOOT_H
#include <stdint.h>
#include <stddef.h>

#define K3_NATIVE_FDT 0x13c000000ULL
#define K3_NATIVE_FDT_LIMIT (256 * 1024)

void pico_itx_fastboot_load(void *destination, size_t bytes);
void pico_itx_native_boot(uint64_t trained_bytes);
int pico_itx_set_fdt_memory(void *fdt, size_t bytes, uint64_t trained_bytes);

#endif
