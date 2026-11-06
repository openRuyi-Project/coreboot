/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <console/console.h>
#include <device/i2c_simple.h>
#include <commonlib/endian.h>
#include <soc/sdram.h>
#include <cbfs.h>
#include <cbmem.h>
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

static int fixup_board_tree(struct device_tree_fixup *fixup, struct device_tree *tree)
{
	fixup_mac(tree->root);
	fixup_memory(tree->root);

	/* Keep the coreboot tables and console out of the UEFI allocator. */
	void *cbmem_base;
	size_t cbmem_size;
	if (cbmem_get_region(&cbmem_base, &cbmem_size) ||
	    !dt_add_reserved_memory_region(tree, "coreboot", NULL,
			(uintptr_t)cbmem_base, cbmem_size, true))
		return -1;
	return 0;
}

static struct device_tree_fixup board_fixup = { .fixup = fixup_board_tree };

static void prepare_fdt(void *unused)
{
	void *fdt = cbfs_map("fallback/DTB", NULL);
	if (!fdt || !fdt_is_valid(fdt))
		die("Unable to load fallback/DTB");

	/* The architecture handoff applies board changes and flattens only once. */
	list_insert_after(&board_fixup.list_node, &device_tree_fixups);
	for (int i = 0; i < CONFIG_MAX_CPUS; i++)
		OTHER_HLS(i)->fdt = fdt;
}

BOOT_STATE_INIT_ENTRY(BS_WRITE_TABLES, BS_ON_EXIT, prepare_fdt, NULL);
