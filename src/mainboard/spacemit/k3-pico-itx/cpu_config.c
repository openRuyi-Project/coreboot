/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootstate.h>
#include <commonlib/device_tree.h>
#include <console/console.h>
#include <endian.h>
#include <mainboard/flash.h>
#include <spi_flash.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Last 4 KiB of EARLY_RESERVED, shared with EDK2 K3CpuConfigLib. */
#define CPU_CONFIG_OFFSET 0x20f000
#define CPU_CONFIG_MAGIC 0x5043334b /* "K3CP", little endian */
#define X100_HARTS 8
#define K3_HARTS 16

enum cpu_mode { X100_ONLY, X100_A100, A100_ONLY };

static enum cpu_mode selected_cpu_mode(void)
{
	struct spi_flash flash;
	u8 record[16];
	u32 crc = ~0U;

	/* Read physical NOR even when the firmware itself was loaded over USB. */
	if (pico_itx_flash_init() || spi_flash_probe(0, 0, &flash) ||
	    spi_flash_read(&flash, CPU_CONFIG_OFFSET, sizeof(record), record)) {
		printk(BIOS_WARNING, "K3 CPU configuration unavailable; using X100 only\n");
		return X100_ONLY;
	}

	if (le32dec(record) != CPU_CONFIG_MAGIC || le16dec(record + 4) != 1 ||
	    le16dec(record + 6) != sizeof(record) || le32dec(record + 8) > A100_ONLY)
		return X100_ONLY;

	/* CRC-32/ISO-HDLC, matching EFI CalculateCrc32(), over the first 12 bytes. */
	for (size_t i = 0; i < 12; i++) {
		crc ^= record[i];
		for (unsigned int bit = 0; bit < 8; bit++)
			crc = (crc >> 1) ^ (0xedb88320U & -(crc & 1));
	}
	return ~crc == le32dec(record + 12) ? le32dec(record + 8) : X100_ONLY;
}

static int scalar_isa(struct device_tree_node *cpu)
{
	const void *data;
	size_t size, used = 0;

	dt_find_bin_prop(cpu, "riscv,isa-extensions", &data, &size);
	if (!data || !size)
		return -1;

	char *extensions = xmalloc(size);
	for (size_t offset = 0; offset < size;) {
		const char *name = (const char *)data + offset;
		size_t length = strnlen(name, size - offset);
		if (!length || length == size - offset)
			return -1;
		if (strcmp(name, "v") && strcmp(name, "h") &&
		    strncmp(name, "zv", 2) && strncmp(name, "sh", 2)) {
			memcpy(extensions + used, name, length + 1);
			used += length + 1;
		}
		offset += length + 1;
	}
	dt_add_bin_prop(cpu, "riscv,isa-extensions", extensions, used);
	/* The split ISA properties are authoritative; do not leave a stale alias. */
	dt_delete_prop(cpu, "riscv,isa");
	return 0;
}

static u32 add_phandle(struct device_tree *tree, struct device_tree_node *node)
{
	if (!node->phandle) {
		node->phandle = ++tree->max_phandle;
		dt_add_u32_prop(node, "phandle", node->phandle);
	}
	return node->phandle;
}

static struct device_tree_node *add_a100(struct device_tree *tree, unsigned int hart,
					       enum cpu_mode mode)
{
	/* SpacemiT vendor Linux k3-br-v1.0.y, 0bef28bfeb51: A100 ISA. */
	static char isa[] =
		"i\0m\0a\0f\0d\0c\0b\0"
		"v\0smaia\0smstateen\0ssaia\0ssccptr\0sscofpmf\0sscounterenw\0"
		"ssnpm\0ssstateen\0sstc\0sstvala\0sstvecd\0ssu64xl\0svade\0"
		"svinval\0svnapot\0svpbmt\0za64rs\0zawrs\0zba\0zbb\0"
		"zbc\0zbs\0zca\0zcb\0zcd\0zcmop\0zfa\0"
		"zfbfmin\0zfh\0zfhmin\0zicbom\0zicbop\0zicboz\0ziccamoa\0"
		"ziccif\0zicclsm\0zicntr\0zicond\0zicsr\0zifencei\0zihintntl\0"
		"zihintpause\0zihpm\0zimop\0zkt\0zvbb\0zvbc\0zvfbfmin\0"
		"zvfbfwma\0zvfh\0zvfhmin\0zvkb\0zvkg\0zvkn\0zvknc\0"
		"zvkned\0zvkng\0zvknha\0zvknhb\0zvks\0zvksc\0zvksed\0"
		"zvksg\0zvksh\0zvkt\0sdtrig";
	static char compatible[] = "spacemit,a100\0riscv";
	char path[64];
	struct device_tree_node *cpu, *intc, *core, *cache;

	snprintf(path, sizeof(path), "/cpus/cpu@%x", hart);
	cpu = dt_find_node_by_path(tree, path, NULL, NULL, 1);
	dt_add_string_prop(cpu, "device_type", "cpu");
	dt_add_bin_prop(cpu, "compatible", compatible, sizeof(compatible));
	dt_add_string_prop(cpu, "status", "okay");
	dt_add_u32_prop(cpu, "reg", hart);
	dt_add_string_prop(cpu, "riscv,isa-base", "rv64i");
	dt_add_bin_prop(cpu, "riscv,isa-extensions", isa, sizeof(isa));
	if (mode == X100_A100 && scalar_isa(cpu))
		return NULL;
	dt_add_string_prop(cpu, "mmu-type", "riscv,sv39");
	dt_add_u32_prop(cpu, "riscv,cbom-block-size", 64);
	dt_add_u32_prop(cpu, "riscv,cboz-block-size", 64);
	dt_add_u32_prop(cpu, "riscv,cbop-block-size", 64);
	dt_add_u32_prop(cpu, "i-cache-block-size", 64);
	dt_add_u32_prop(cpu, "i-cache-size", 32768);
	dt_add_u32_prop(cpu, "i-cache-sets", 128);
	dt_add_u32_prop(cpu, "d-cache-block-size", 64);
	dt_add_u32_prop(cpu, "d-cache-size", 32768);
	dt_add_u32_prop(cpu, "d-cache-sets", 128);

	/* Keep Linux logical CPU numbering in ascending hart order. */
	struct device_tree_node *cpus = dt_find_node_by_path(tree, "/cpus", NULL, NULL, 0);
	list_remove(&cpu->list_node);
	list_append(&cpu->list_node, &cpus->children);

	snprintf(path, sizeof(path), "/cpus/cache-controller-%u", hart / 4);
	cache = dt_find_node_by_path(tree, path, NULL, NULL, 1);
	dt_add_string_prop(cache, "compatible", "cache");
	dt_add_u32_prop(cache, "cache-block-size", 64);
	dt_add_u32_prop(cache, "cache-level", 2);
	dt_add_u32_prop(cache, "cache-size", 1048576);
	dt_add_u32_prop(cache, "cache-sets", 1024);
	dt_add_bin_prop(cache, "cache-unified", (void *)"", 0);
	dt_add_u32_prop(cpu, "next-level-cache", add_phandle(tree, cache));

	snprintf(path, sizeof(path), "/cpus/cpu@%x/interrupt-controller", hart);
	intc = dt_find_node_by_path(tree, path, NULL, NULL, 1);
	dt_add_string_prop(intc, "compatible", "riscv,cpu-intc");
	dt_add_u32_prop(intc, "#interrupt-cells", 1);
	dt_add_bin_prop(intc, "interrupt-controller", (void *)"", 0);
	add_phandle(tree, intc);

	snprintf(path, sizeof(path), "/cpus/cpu-map/cluster%u/core%u",
		 (hart - (mode == A100_ONLY ? X100_HARTS : 0)) / 4, hart % 4);
	core = dt_find_node_by_path(tree, path, NULL, NULL, 1);
	dt_add_u32_prop(core, "cpu", add_phandle(tree, cpu));
	return intc;
}

static int configure_cpus(struct device_tree_fixup *fixup, struct device_tree *tree)
{
	static const struct {
		const char *path;
		u32 irq;
		const char *a100_name;
		u64 a100_address;
		u64 a100_size;
	} controllers[] = {
		{ "/soc/interrupt-controller@e0400000", 9,
		  "interrupt-controller@e0600000", 0xe0600000, 0x200000 },
		{ "/soc/interrupt-controller@f1000000", 11, NULL, 0, 0 },
		{ "/soc/interrupt-controller@f1810000", 3, NULL, 0, 0 },
		{ "/soc/timer@f1814000", 7, NULL, 0, 0 },
		{ "/soc/interrupt-controller@f181c000", 1,
		  "interrupt-controller@f181c020", 0xf181c020, 0x20 },
	};
	u32 intcs[K3_HARTS];
	char path[64];

	enum cpu_mode mode = selected_cpu_mode();
	if (mode == X100_ONLY)
		return 0;

	if (mode == A100_ONLY) {
		/* OpenSBI still needs the X100 boot hart during cold initialization.
		 * SEC transfers to hart 8, then disables X100 nodes in its DT copy.
		 */
		struct device_tree_node *chosen = dt_find_node_by_path(tree,
			"/chosen", NULL, NULL, 1);
		dt_add_u32_prop(chosen, "boot-hartid", X100_HARTS);
		struct device_tree_node *map = dt_find_node_by_path(tree,
			"/cpus/cpu-map", NULL, NULL, 1);
		while (!list_is_empty(&map->children))
			list_remove(list_first(&map->children));
	}

	for (unsigned int hart = 0; hart < X100_HARTS; hart++) {
		snprintf(path, sizeof(path), "/cpus/cpu@%x", hart);
		struct device_tree_node *cpu = dt_find_node_by_path(tree, path, NULL, NULL, 0);
		if (!cpu || scalar_isa(cpu))
			return -1;
		struct device_tree_node *intc = dt_find_compat(cpu, "riscv,cpu-intc");
		if (!intc || !intc->phandle)
			return -1;
		intcs[hart] = intc->phandle;
	}
	for (unsigned int hart = X100_HARTS; hart < K3_HARTS; hart++) {
		struct device_tree_node *intc = add_a100(tree, hart, mode);
		if (!intc)
			return -1;
		intcs[hart] = intc->phandle;
	}

	for (size_t i = 0; i < ARRAY_SIZE(controllers); i++) {
		unsigned int first = mode == A100_ONLY && controllers[i].a100_name ?
			X100_HARTS : 0;
		unsigned int count = K3_HARTS - first;
		struct device_tree_node *node = dt_find_node_by_path(tree,
			controllers[i].path, NULL, NULL, 0);
		if (!node && first) {
			/* Allow the same fixup to run again on the renamed node. */
			snprintf(path, sizeof(path), "/soc/%s", controllers[i].a100_name);
			node = dt_find_node_by_path(tree, path, NULL, NULL, 0);
		}
		if (!node)
			return -1;
		u32 *irqs = xmalloc(count * 2 * sizeof(*irqs));
		for (unsigned int index = 0; index < count; index++) {
			irqs[2 * index] = htobe32(intcs[first + index]);
			irqs[2 * index + 1] = htobe32(controllers[i].irq);
		}
		dt_add_bin_prop(node, "interrupts-extended", irqs, count * 2 * sizeof(*irqs));

		if (first) {
			/* S-mode lists start at hart 8. Move the register window with
			 * them: IMSIC uses a 256 KiB stride, SSWI uses 4 bytes.
			 * OpenSBI retains all M-mode routes for its X100 boot hart.
			 */
			node->name = controllers[i].a100_name;
			dt_add_reg_prop(node, &controllers[i].a100_address,
				&controllers[i].a100_size, 1, 2, 2);
		} else if (i == 0) {
			const u64 address = 0xe0400000, size = 0x400000;
			/* Keep guest-index-bits=6: it describes the hart stride too. */
			dt_add_reg_prop(node, &address, &size, 1, 2, 2);
		}
	}
	printk(BIOS_INFO, "K3 CPU mode: %s\n", mode == A100_ONLY ?
	       "A100 only, vector enabled" : "X100 + A100, scalar only");
	return 0;
}

static struct device_tree_fixup cpu_fixup = { .fixup = configure_cpus };

static void register_cpu_fixup(void *unused)
{
	list_insert_after(&cpu_fixup.list_node, &device_tree_fixups);
}

BOOT_STATE_INIT_ENTRY(BS_PAYLOAD_LOAD, BS_ON_ENTRY, register_cpu_fixup, NULL);
