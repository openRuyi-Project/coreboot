// SPDX-License-Identifier: GPL-2.0-only

#include <bootstate.h>
#include <commonlib/device_tree.h>
#include <device/device.h>
#include <soc/usb_boot.h>

static int set_download_ports(struct device_tree_fixup *fixup, struct device_tree *tree)
{
	static const char *const ports[] = {
		"/soc/bus@50400000/usb@50480000",
		"/soc/bus@70400000/usb@70480000",
	};

	if (!eic7700_usb_boot())
		return 0;

	for (size_t i = 0; i < ARRAY_SIZE(ports); i++) {
		struct device_tree_node *usb = dt_find_node_by_path(tree, ports[i],
								    NULL, NULL, 0);
		if (!usb)
			return -1;
		dt_add_string_prop(usb, "dr_mode", "peripheral");
	}
	return 0;
}

static struct device_tree_fixup usb_fixup = { .fixup = set_download_ports };

static void register_usb_fixup(void *unused)
{
	list_insert_after(&usb_fixup.list_node, &device_tree_fixups);
}

BOOT_STATE_INIT_ENTRY(BS_WRITE_TABLES, BS_ON_EXIT, register_usb_fixup, NULL);

struct chip_operations mainboard_ops = {};
