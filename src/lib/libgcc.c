/* SPDX-License-Identifier: GPL-2.0-only */

#include <types.h>

#if !ENV_X86 /* Work around the lack of --gc-sections on x86. */
static int clz32(u32 a)
{
	static const u8 four_bit_table[] = {
		[0x0] = 4, [0x1] = 3, [0x2] = 2, [0x3] = 2,
		[0x4] = 1, [0x5] = 1, [0x6] = 1, [0x7] = 1,
		[0x8] = 0, [0x9] = 0, [0xa] = 0, [0xb] = 0,
		[0xc] = 0, [0xd] = 0, [0xe] = 0, [0xf] = 0,
	};
	int r = 0;

	if (!(a & (0xffffU << 16))) {
		r += 16;
		a <<= 16;
	}

	if (!(a & (0xffU << 24))) {
		r += 8;
		a <<= 8;
	}

	if (!(a & (0xfU << 28))) {
		r += 4;
		a <<= 4;
	}

	return r + four_bit_table[a >> 28];
}
#endif

#if ENV_RISCV
int __clzdi2(u64 a);
int __clzdi2(u64 a)
{
	u32 high = a >> 32;

	return high ? clz32(high) : 32 + clz32((u32)a);
}

u32 __bswapsi2(u32 value);
u32 __bswapsi2(u32 value)
{
	value = ((value >> 8) & 0x00ff00ffU) | ((value & 0x00ff00ffU) << 8);
	return (value >> 16) | (value << 16);
}

u64 __bswapdi2(u64 value);
u64 __bswapdi2(u64 value)
{
	return ((u64)__bswapsi2((u32)value) << 32) | __bswapsi2((u32)(value >> 32));
}
#elif !ENV_X86 /* RV32 libgcc already provides __clzsi2. */
int __clzsi2(u32 a);
int __clzsi2(u32 a)
{
	return clz32(a);
}
#endif
