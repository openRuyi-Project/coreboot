/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SOC_ESWIN_EIC7700_ADDRESSMAP_H
#define SOC_ESWIN_EIC7700_ADDRESSMAP_H

#define EIC7700_CLINT       0x02000000
#define EIC7700_CCACHE      0x02010000
#define EIC7700_UART0       0x50900000
#define EIC7700_SYSCON      0x51810000
#define EIC7700_SYSCRG      0x51828000
#define EIC7700_DDRC0       0x52300000
#define EIC7700_DDRC1       0x52380000
#define EIC7700_DRAM        0x80000000
#define EIC7702_DIE1_DRAM   0x2000000000ULL
/* Reserve a directly addressed GiB per die, including DMA32 RAM on die 0.
 * The rest of both DDR banks is exposed through the 256-byte striped view.
 */
#define EIC7702_DIRECT_SIZE  0x40000000ULL
#define EIC7702_STRIPED_BASE 0x4000000000ULL
#define EIC770X_DIE_MMIO_STRIDE 0x20000000UL
#define EIC770X_DIES (CONFIG(SOC_ESWIN_EIC7702) ? 2 : 1)

/* BootROM loads the complete MCPU image here after SCPU DDR training. */
#define EIC7700_ROM_BUFFER  EIC7700_DRAM

#endif
