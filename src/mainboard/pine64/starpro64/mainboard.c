// SPDX-License-Identifier: GPL-2.0-only

#include <bootstate.h>
#include <commonlib/device_tree.h>
#include <device/device.h>
#include <device/mmio.h>
#include <soc/usb_boot.h>

#define GPIO_B_DR       ((void *)0x5160000c)
#define GPIO_B_DDR      ((void *)0x51600010)
#define USB0_PWREN_PAD  ((void *)(0x51600080 + 89 * 4))
#define FUNC_SEL        (7U << 16)
#define FUNC_GPIO       (2U << 16)
#define USB_PWREN       (1U << 10)

static int fixup_usb_role(struct device_tree_fixup *fixup, struct device_tree *tree)
{
	/* GPIO42 controls the shared Type-A VBUS switches on this board. */
	clrsetbits32(GPIO_B_DR, USB_PWREN, eic7700_usb_boot() ? 0 : USB_PWREN);
	setbits32(GPIO_B_DDR, USB_PWREN);
	clrsetbits32(USB0_PWREN_PAD, FUNC_SEL, FUNC_GPIO);
	return eic7700_set_boot_usb_role(tree);
}

static struct device_tree_fixup usb_fixup = { .fixup = fixup_usb_role };

static void register_usb_fixup(void *unused)
{
	list_insert_after(&usb_fixup.list_node, &device_tree_fixups);
}

BOOT_STATE_INIT_ENTRY(BS_WRITE_TABLES, BS_ON_EXIT, register_usb_fixup, NULL);

struct chip_operations mainboard_ops = { };
