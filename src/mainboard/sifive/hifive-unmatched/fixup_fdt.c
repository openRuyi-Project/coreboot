/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <console/console.h>
#include <device/i2c_simple.h>
#include <commonlib/endian.h>
#include <soc/sdram.h>
#include <cbfs.h>
#include <commonlib/device_tree.h>
#include <bootstate.h>
#include <mcall.h>

static bool valid_mac(const uint8_t *mac, size_t size)
{
	static const uint8_t zero[6];
	return mac && size == 6 && !(mac[0] & 1) && memcmp(mac, zero, 6);
}

static bool read_board_mac(uint8_t mac[6])
{
	/* SiFive board EEPROM v1, also used by U-Boot's Unmatched board code. */
	uint8_t data[37];
	static const uint8_t magic[] = {0xf1, 0x5e, 0x50, 0x45};
	if (i2c_read_bytes(0, 0x54, 0, data, sizeof(data)) ||
	    memcmp(data, magic, sizeof(magic)) || data[4] != 1 ||
	    read_le16(data + 5) != 2)
		return false;

	/* The EEPROM stores the reflected IEEE CRC32, little endian. */
	uint32_t crc = UINT32_MAX;
	for (size_t i = 0; i < 33; i++) {
		crc ^= data[i];
		for (unsigned int bit = 0; bit < 8; bit++)
			crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320 : 0);
	}
	if (~crc != read_le32(data + 33) || !valid_mac(data + 27, 6))
		return false;
	memcpy(mac, data + 27, 6);
	return true;
}

static void do_fixup_mac(struct device_tree_node *node)
{
	const void *data;
	size_t size;
	/* Preserve an intentional valid address supplied with the input DT. */
	dt_find_bin_prop(node, "mac-address", &data, &size);
	if (valid_mac(data, size))
		return;
	dt_find_bin_prop(node, "local-mac-address", &data, &size);
	if (valid_mac(data, size))
		return;

	uint8_t mac[6];
	if (!read_board_mac(mac)) {
		printk(BIOS_ERR, "Unmatched: no valid Ethernet address in board EEPROM\n");
		return;
	}
	/* The tree retains this buffer until the final flattening pass. */
	uint8_t *address = malloc(sizeof(mac));
	if (!address)
		die("Unable to allocate Ethernet address");
	memcpy(address, mac, sizeof(mac));
	dt_add_bin_prop(node, "local-mac-address", address, sizeof(mac));
	printk(BIOS_INFO, "Unmatched Ethernet: %02x:%02x:%02x:%02x:%02x:%02x\n",
	       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void fixup_mac(struct device_tree_node *parent)
{
	struct device_tree_property *prop;
	const char *name = "local-mac-address";

	list_for_each(prop, parent->properties, list_node) {
		if (!strcmp(name, prop->prop.name))
			do_fixup_mac(parent);
	}

	struct device_tree_node *child;
	list_for_each(child, parent->children, list_node) {
		fixup_mac(child);
	}
}

static void do_fixup_memory(struct device_tree_node *node)
{
	u64 addrs[1], sizes[1];
	addrs[0] = 0x80000000;
	sizes[0] = sdram_size();
	dt_add_reg_prop(node, addrs, sizes, 1, 2, 2);
}

static void fixup_memory(struct device_tree_node *parent)
{
	struct device_tree_property *prop;
	const char *name = "device_type";
	const char *value = "memory";

	list_for_each(prop, parent->properties, list_node) {
		if (!strcmp(name, prop->prop.name)) {
			if (!strcmp(value, (char *)prop->prop.data))
				do_fixup_memory(parent);
		}
	}

	struct device_tree_node *child;
	list_for_each(child, parent->children, list_node) {
		fixup_memory(child);
	}
}

static void fixup_fdt(void *unused)
{
	printk(BIOS_DEBUG, "fix up FDT\n");

	void *fdt_rom;
	struct device_tree *tree;

	/* load flat dt from cbfs */
	fdt_rom = cbfs_map("fallback/DTB", NULL);

	if (fdt_rom == NULL) {
		printk(BIOS_ERR, "Unable to load fallback/DTB from CBFS\n");
		return;
	}

	/* Expand DT into a tree */
	tree = fdt_unflatten(fdt_rom);

	/* fixup tree */
	fixup_mac(tree->root);
	fixup_memory(tree->root);

	/* convert the tree to a flat dt */
	void *dt = malloc(dt_flat_size(tree));

	if (dt == NULL) {
		printk(BIOS_ERR, "Unable to allocate memory for flat device tree\n");
		return;
	}

	dt_flatten(tree, dt);

	/* update HLS */
	for (int i = 0; i < CONFIG_MAX_CPUS; i++)
		OTHER_HLS(i)->fdt = dt;
}

BOOT_STATE_INIT_ENTRY(BS_WRITE_TABLES, BS_ON_EXIT, fixup_fdt, NULL);
