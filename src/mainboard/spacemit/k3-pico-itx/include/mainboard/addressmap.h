/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef K3_PICO_ADDRESSMAP_H
#define K3_PICO_ADDRESSMAP_H
#include <soc/addressmap.h>
/* Keep OpenSBI and EDK2 outside coreboot's allocator. */
#define K3_CBMEM_TOP 0x110000000
#define K3_COREBOOT_BASE 0x104000000
#endif
