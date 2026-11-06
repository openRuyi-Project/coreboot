/* SPDX-License-Identifier: GPL-2.0-only */
#include <boot_device.h>
#include <commonlib/region.h>

/* The JTAG loader supplies this image after DDR initialization. */
static const struct mem_region_device boot_dev =
	MEM_REGION_DEV_RO_INIT(0x90000000, CONFIG_ROM_SIZE);

const struct region_device *boot_device_ro(void)
{
	return &boot_dev.rdev;
}
