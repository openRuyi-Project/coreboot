// SPDX-License-Identifier: GPL-2.0-only
#include <bootstate.h>
#include <cbmem.h>
#include <commonlib/device_tree.h>
#include <device/mmio.h>
#include <soc/addressmap.h>
#include <soc/memory.h>
#include <stdio.h>

static int fixup(struct device_tree_fixup *unused, struct device_tree *tree)
{
	struct sg2042_range ranges[SG2042_MAX_RANGES];
	unsigned int count = sg2042_memory_map(ranges);
	void *cbmem_base;
	size_t cbmem_size;

	for (unsigned int i = 0; i < count; i++) {
		char path[40];
		snprintf(path, sizeof(path), "/memory@%llx", (unsigned long long)ranges[i].base);
		struct device_tree_node *node = dt_find_node_by_path(tree, path, NULL, NULL, 1);
		if (!node)
			return -1;
		dt_add_string_prop(node, "device_type", "memory");
		dt_add_reg_prop(node, &ranges[i].base, &ranges[i].size, 1, 2, 2);
		dt_add_u32_prop(node, "numa-node-id", ranges[i].channel);
	}
	/* The SCP releases only enabled four-hart clusters. */
	for (unsigned int i = 0; i < 16; i++) {
		if (read32((void *)SG2042_MP_CONTROL(i)))
			continue;
		unsigned int cluster = read32((void *)SG2042_MP_STATUS(i));
		if (cluster >= 16)
			return -1;
		for (unsigned int j = 0; j < 4; j++) {
			char path[32];
			snprintf(path, sizeof(path), "/cpus/cpu@%x", 4 * cluster + j);
			struct device_tree_node *cpu = dt_find_node_by_path(tree, path, NULL, NULL, 0);
			if (!cpu)
				return -1;
			dt_add_string_prop(cpu, "status", "disabled");
		}
	}
	if (cbmem_get_region(&cbmem_base, &cbmem_size) ||
	    !dt_add_reserved_memory_region(tree, "coreboot", NULL, (uintptr_t)cbmem_base,
					   cbmem_size, true))
		return -1;
	return 0;
}

static struct device_tree_fixup memory_fixup = { .fixup = fixup };

static void register_fixup(void *unused)
{
	list_insert_after(&memory_fixup.list_node, &device_tree_fixups);
}
BOOT_STATE_INIT_ENTRY(BS_WRITE_TABLES, BS_ON_EXIT, register_fixup, NULL);
