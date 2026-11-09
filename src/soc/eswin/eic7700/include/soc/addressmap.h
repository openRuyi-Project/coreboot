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
/* BootROM loads the complete MCPU image here after SCPU DDR training. */
#define EIC7700_ROM_BUFFER  0x90000000

#endif
