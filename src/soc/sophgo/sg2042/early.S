/* SPDX-License-Identifier: GPL-2.0-only */
/* C920 setup from SOPHGO ZSBL, plat/sg2042/early_setup.S. */
#include <arch/encoding.h>
#define CSR_MCOR         0x7c2
#define CSR_MHCR         0x7c1
#define CSR_MCCR2        0x7c3
#define CSR_MHINT        0x7c5
#define CSR_MHINT2       0x7cc
#define CSR_MHINT4       0x7ce
#define CSR_MXSTATUS     0x7c0
#define CSR_MSMPR        0x7f3

.section .text.cache_as_ram, "ax"
.globl cache_as_ram
cache_as_ram:
	/* Only single-socket systems have stacks in this layout. */
	csrr	t0, mhartid
	li	t1, CONFIG_MAX_CPUS
	bgeu	t0, t1, 1f
	li	t0, 0x70013
	csrw	CSR_MCOR, t0

	/* disable l0btb to workround auipc bug */
	li	t0, 0x1ff
	csrw	CSR_MHCR, t0

	li	t0, 0xe0410009
	csrw	CSR_MCCR2, t0

	li	t0, 0xe6e30c
	csrw	CSR_MHINT, t0

	csrr	t0, CSR_MHINT2
	li	t1, 3
	slli	t1, t1, 7
	or	t0, t0, t1
	li	t1, 1
	slli	t1, t1, 22
	or	t0, t0, t1
	csrw	CSR_MHINT2, t0

	li	t0, 0x638000
	csrw	CSR_MXSTATUS, t0

	/* disable dvm */
	li	t0, 0x20
	csrw	CSR_MHINT4, t0

	li	t0, 1
	csrw	CSR_MSMPR, t0

	fence.i
	fence	iorw, iorw
	ret
1:	wfi
	j	1b
