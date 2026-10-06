/* SPDX-License-Identifier: GPL-2.0-only */
#include <arch/encoding.h>

	.section .text.cache_as_ram, "ax", %progbits
	.globl cache_as_ram
cache_as_ram:
	/* BootROM supplies physical SRAM. Guard the boot-hart-only contract
	 * before the architecture entry computes a per-hart stack address.
	 */
	csrw mie, zero
	csrr t0, mhartid
	bnez t0, 1f
	/* BootROM only loads the file bytes. Establish zero-initialized C
	 * storage before any hart-local or console initialization.
	 */
	la t0, _bss
	la t1, _ebss
2:
	bgeu t0, t1, 3f
	sd zero, 0(t0)
	addi t0, t0, 8
	j 2b
3:
	ret
1:
	wfi
	j 1b
