/* SPDX-License-Identifier: GPL-2.0-only */
#include <boot_device.h>
#include <bootmem.h>
#include <mainboard/native_boot.h>

/* NOR and USB loaders provide the same complete ROM view in DRAM. */
static const struct mem_region_device boot_dev =
	MEM_REGION_DEV_RO_INIT(CONFIG_K3_CBFS_BASE, CONFIG_ROM_SIZE);

const struct region_device *boot_device_ro(void)
{
	return &boot_dev.rdev;
}

void bootmem_platform_add_ranges(void)
{
	/* Keep sources alive while the payload loader allocates its buffers.
	 * They are reclaimable by the payload after the final handoff.
	 */
	bootmem_add_range(CONFIG_K3_CBFS_BASE, CONFIG_ROM_SIZE, BM_MEM_RAMSTAGE);
	if (CONFIG(SPACEMIT_K3_NATIVE_BOOT))
		bootmem_add_range(K3_NATIVE_FDT, K3_NATIVE_FDT_LIMIT, BM_MEM_RAMSTAGE);
}
