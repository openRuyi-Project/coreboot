// SPDX-License-Identifier: GPL-2.0-only

#include <bootstate.h>
#include <cbfs.h>
#include <cbmem.h>
#include <commonlib/device_tree.h>
#include <console/console.h>
#include <device/mmio.h>
#include <mcall.h>
#include <stdio.h>
#include <string.h>
#include <soc/addressmap.h>
#include <soc/sdram.h>
#include <soc/usb_boot.h>

static int fixup_memory(struct device_tree_fixup *fixup, struct device_tree *tree)
{
	struct device_tree_node *memory;
	void *cbmem_base;
	size_t cbmem_size;

	struct eic770x_bank banks[4];
	static char names[4][32];
	unsigned int count = eic770x_memory_map(banks);

	/* Replace the board DTS defaults with the trained capacity. In the
	 * striped window even the second node's starting address can change.
	 */
	if (CONFIG(SOC_ESWIN_EIC7702)) {
		struct list_node *head = &tree->root->children;
		struct list_node *item = list_first(head);
		while (item) {
			struct device_tree_node *node =
				container_of(item, struct device_tree_node, list_node);
			item = list_next(item, head);
			const char *type = dt_find_string_prop(node, "device_type");
			if (type && !strcmp(type, "memory"))
				list_remove(&node->list_node);
		}
	}
	for (unsigned int i = 0; i < count; i++) {
		snprintf(names[i], sizeof(names[i]), "/memory@%llx", banks[i].base);
		memory = dt_find_node_by_path(tree, names[i], NULL, NULL, 1);
		if (!memory)
			return -1;
		dt_add_string_prop(memory, "device_type", "memory");
		dt_add_reg_prop(memory, &banks[i].base, &banks[i].size, 1, 2, 2);
		if (CONFIG(SOC_ESWIN_EIC7702))
			dt_add_u32_prop(memory, "numa-node-id", i % 2);
	}
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

#define BOOT_SEL_STAT  0x33c
#define BOOT_MEDIA_MASK  0x3
#define BOOT_MEDIA_USB   0x3

bool eic7700_usb_boot(void)
{
	return (read32((void *)(EIC7700_SYSCRG + BOOT_SEL_STAT)) & BOOT_MEDIA_MASK) ==
		BOOT_MEDIA_USB;
}

int eic7700_set_boot_usb_role(struct device_tree *tree)
{
	struct device_tree_node *usb0;

	if (!eic7700_usb_boot())
		return 0;
	usb0 = dt_find_node_by_path(tree, "/soc/usb@50480000", NULL, NULL, 0);
	if (!usb0)
		return -1;
	dt_add_string_prop(usb0, "dr_mode", "peripheral");
	return 0;
}
