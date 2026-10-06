/* SPDX-License-Identifier: GPL-2.0-only */
#include <commonlib/device_tree.h>
#include <endian.h>
#include <mainboard/addressmap.h>
#include <mainboard/native_boot.h>
#include <string.h>

/* The DT is a verified CBFS file from this image. Change only its fixed-size
 * memory tuple after both controllers and the selected data checks succeed.
 */
int pico_itx_set_fdt_memory(void *fdt, size_t bytes, uint64_t trained_bytes)
{
	struct fdt_header *header = fdt;
	struct fdt_property reg, type;
	u32 address_cells = 0, size_cells = 0;
	if (bytes < sizeof(*header) || be32toh(header->totalsize) != bytes ||
	    (!trained_bytes || trained_bytes % MiB || trained_bytes > 32ULL * GiB) || !fdt_is_valid(fdt))
		return -1;
	u32 node = fdt_find_node_by_path(fdt, "/memory@100000000", &address_cells, &size_cells);
	if (!node || address_cells != 2 || size_cells != 2 ||
	    !fdt_read_prop(fdt, node, "device_type", &type) || type.size != 7 ||
	    memcmp(type.data, "memory", 7) ||
	    !fdt_read_prop(fdt, node, "reg", &reg) || reg.size != 16)
		return -1;
	uint64_t values[] = { htobe64(K3_DRAM_BASE), htobe64(trained_bytes) };
	memcpy(reg.data, values, sizeof(values));
	struct device_tree_region regions[2];
	return fdt_read_memory_regions(fdt, regions, ARRAY_SIZE(regions)) == 1 &&
	       regions[0].addr == K3_DRAM_BASE && regions[0].size == trained_bytes ? 0 : -1;
}
