// SPDX-License-Identifier: GPL-2.0-only
#include <cbmem.h>
#include <commonlib/helpers.h>
#include <console/console.h>
#include <device/mmio.h>
#include <soc/addressmap.h>
#include <soc/memory.h>
#include <symbols.h>

unsigned int sg2042_memory_map(struct sg2042_range ranges[SG2042_MAX_RANGES])
{
	/* The SCP records 40 - log2(bytes) per DIMM. 41 means unpopulated. */
	uint32_t geometry = read32((void *)SG2042_DDR_SIZE);
	uint64_t base = 0;
	unsigned int n = 0;

	for (unsigned int i = 0; i < 4; i++, geometry >>= 8) {
		unsigned int code = geometry & 0xff;
		if (code == 41)
			continue;
		if (code < 4 || code > 40)
			die("Invalid SG2042 DDR geometry");
		uint64_t end = base + (1ULL << (40 - code));
		if (base < SG2042_PCIE_BASE)
			ranges[n++] = (struct sg2042_range){base, MIN(end, SG2042_PCIE_BASE) - base, i};
		if (end > SG2042_PCIE_END) {
			uint64_t start = MAX(base, SG2042_PCIE_END);
			ranges[n++] = (struct sg2042_range){start, end - start, i};
		}
		base = end;
	}
	if (!n || ranges[0].size < (uintptr_t)_epreram_cbfs_cache + 2 * MiB)
		die("Insufficient SG2042 low memory");
	return n;
}

uintptr_t cbmem_top_chipset(void)
{
	struct sg2042_range ranges[SG2042_MAX_RANGES];
	sg2042_memory_map(ranges);
	return ranges[0].base + ranges[0].size;
}
