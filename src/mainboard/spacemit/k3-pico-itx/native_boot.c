/* SPDX-License-Identifier: GPL-2.0-only */
#include <cbfs.h>
#include <commonlib/device_tree.h>
#include <console/console.h>
#include <fmap.h>
#include <device/mmio.h>
#include <mainboard/addressmap.h>
#include <mainboard/flash.h>
#include <mainboard/native_boot.h>
#include <mcall.h>
#include <spi_flash.h>

void pico_itx_native_boot(uint64_t trained_bytes)
{
	struct region cbfs;
	bool usb = (read32((void *)0xd4282d10) & 0xfff) == 0x55a;
	struct spi_flash flash;

	if (usb) {
		pico_itx_fastboot_load((void *)CONFIG_K3_CBFS_BASE, CONFIG_ROM_SIZE);
	} else {
		if (pico_itx_flash_init())
			die("K3 native boot-media initialization failed\n");
		if (spi_flash_probe(0, 0, &flash) || flash.size != CONFIG_ROM_SIZE)
			die("K3 native NOR capacity/probe failed\n");
		if (spi_flash_read(&flash, FMAP_OFFSET, FMAP_SIZE,
				   (void *)(CONFIG_K3_CBFS_BASE + FMAP_OFFSET)))
			die("K3 native FMAP read failed\n");
	}
	/* The standard FMAP reader verifies the bootblock's embedded hash. */
	if (fmap_locate_area("COREBOOT", &cbfs) ||
	    region_offset(&cbfs) != FMAP_SECTION_COREBOOT_START ||
	    region_sz(&cbfs) != FMAP_SECTION_COREBOOT_SIZE)
		die("K3 native NOR layout differs from this image\n");
	if (!usb && spi_flash_read(&flash, region_offset(&cbfs), region_sz(&cbfs),
			   (void *)(CONFIG_K3_CBFS_BASE + region_offset(&cbfs))))
		die("K3 native CBFS read failed\n");
	void *fdt = (void *)K3_NATIVE_FDT;
	size_t bytes = cbfs_load("k3-pico-itx.dtb", fdt, K3_NATIVE_FDT_LIMIT);
	if (!bytes || pico_itx_set_fdt_memory(fdt, bytes, trained_bytes))
		die("K3 native memory FDT validation failed\n");
	HLS()->fdt = fdt;
	printk(BIOS_INFO, "K3 native coreboot: verified CBFS/FDT, continuing to ramstage\n");
}
