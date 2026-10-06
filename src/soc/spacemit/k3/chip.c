/* SPDX-License-Identifier: GPL-2.0-only */
#include <commonlib/device_tree.h>
#include <console/console.h>
#include <device/device.h>
#include <mcall.h>
#include <soc/addressmap.h>

/* Match the common FDT reader's region capacity; reject saturation. */
#define K3_MAX_MEMORY_REGIONS 16

static void k3_read_resources(struct device *dev)
{
	struct device_tree_region regions[K3_MAX_MEMORY_REGIONS];
	size_t count = fdt_read_memory_regions(HLS()->fdt, regions, ARRAY_SIZE(regions));
	const uint64_t reserved_end = K3_DRAM_BASE + K3_FIRMWARE_RESERVED_SIZE;
	unsigned int index = 0;

	if (!count || count >= ARRAY_SIZE(regions))
		die("K3 memory handoff has no complete region list\n");
	/* Validate the whole list before publishing any allocator resources. */
	for (size_t i = 0; i < count; i++) {
		uint64_t start = regions[i].addr;
		uint64_t size = regions[i].size;
		if (start < K3_DRAM_BASE || !size || start + size < start)
			die("K3 memory handoff has an invalid range\n");
		for (size_t j = 0; j < i; j++) {
			if (start < regions[j].addr + regions[j].size &&
			    regions[j].addr < start + size)
				die("K3 memory handoff has overlapping ranges\n");
		}
	}
	for (size_t i = 0; i < count; i++) {
		uint64_t start = regions[i].addr;
		uint64_t end = start + regions[i].size;
		if (start < reserved_end) {
			uint64_t boundary = MIN(end, reserved_end);
			reserved_ram_range(dev, index++, start, boundary - start);
			start = boundary;
		}
		if (start < end)
			ram_from_to(dev, index++, start, end);
	}
}

struct device_operations spacemit_k3_cpu_ops = {
	.read_resources = k3_read_resources,
	/* Native bootblock initialization has programmed DDR. */
	.set_resources = noop_set_resources,
};

struct chip_operations soc_spacemit_k3_ops = {
	.name = "SpacemiT K3",
};
