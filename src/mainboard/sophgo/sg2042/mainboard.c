// SPDX-License-Identifier: GPL-2.0-only
#include <bootstate.h>
#include <cbfs.h>
#include <commonlib/device_tree.h>
#include <console/console.h>
#include <device/device.h>
#include <device/mmio.h>
#include <mcall.h>
#include <soc/addressmap.h>

static void select_board_fdt(void *unused)
{
	const char *name;
	uint32_t type = read32((void *)SG2042_BOARD_TYPE);

	switch (type) {
	case 0x02:
		name = "sg2042-evb-v1.dtb";
		break;
	case 0x03:
		name = "sg2042-milkv-pioneer.dtb";
		break;
	case 0x05:
		name = "sg2042-evb-v2.dtb";
		break;
	default:
		die("Unsupported SG2042 board ID %#x", type);
	}

	void *fdt = cbfs_map(name, NULL);
	if (!fdt || !fdt_is_valid(fdt))
		die("Invalid CBFS device tree: %s", name);
	printk(BIOS_NOTICE, "SG2042 board %#x: %s\n", type, name);
	for (int i = 0; i < CONFIG_MAX_CPUS; i++)
		OTHER_HLS(i)->fdt = fdt;
}
BOOT_STATE_INIT_ENTRY(BS_WRITE_TABLES, BS_ON_EXIT, select_board_fdt, NULL);

struct chip_operations mainboard_ops = { };
