// SPDX-License-Identifier: GPL-2.0-only

#include <bootstate.h>
#include <cbfs.h>
#include <cbmem.h>
#include <commonlib/device_tree.h>
#include <console/console.h>
#include <mcall.h>
#include <soc/addressmap.h>
#include <soc/sdram.h>

static int fixup_memory(struct device_tree_fixup *fixup, struct device_tree *tree)
{
	struct device_tree_node *memory;
	u64 base = EIC7700_DRAM, size = eic7700_dram_size();
	void *cbmem_base;
	size_t cbmem_size;

	memory = dt_find_node_by_path(tree, "/memory@80000000", NULL, NULL, 1);
	if (!memory)
		return -1;
	dt_add_string_prop(memory, "device_type", "memory");
	dt_add_reg_prop(memory, &base, &size, 1, 2, 2);
	if (cbmem_get_region(&cbmem_base, &cbmem_size) ||
	    !dt_add_reserved_memory_region(tree, "coreboot", NULL,
			(uintptr_t)cbmem_base, cbmem_size, true))
		return -1;
	return 0;
}

static struct device_tree_fixup board_fixup = { .fixup = fixup_memory };

static void prepare_fdt(void *unused)
{
	void *fdt = cbfs_map("fallback/DTB", NULL);

	if (!fdt || !fdt_is_valid(fdt))
		die("Unable to load fallback/DTB");
	list_insert_after(&board_fixup.list_node, &device_tree_fixups);
	for (int i = 0; i < CONFIG_MAX_CPUS; i++)
		OTHER_HLS(i)->fdt = fdt;
}

BOOT_STATE_INIT_ENTRY(BS_WRITE_TABLES, BS_ON_EXIT, prepare_fdt, NULL);
