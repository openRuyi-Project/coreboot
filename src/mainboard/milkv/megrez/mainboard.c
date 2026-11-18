// SPDX-License-Identifier: GPL-2.0-only

#include <bootstate.h>
#include <commonlib/device_tree.h>
#include <device/device.h>
#include <soc/usb_boot.h>

static int fixup_usb_role(struct device_tree_fixup *fixup, struct device_tree *tree)
{
	return eic7700_set_boot_usb_role(tree);
}

static struct device_tree_fixup usb_fixup = { .fixup = fixup_usb_role };

static void register_usb_fixup(void *unused)
{
	list_insert_after(&usb_fixup.list_node, &device_tree_fixups);
}

BOOT_STATE_INIT_ENTRY(BS_WRITE_TABLES, BS_ON_EXIT, register_usb_fixup, NULL);

struct chip_operations mainboard_ops = { };
