// SPDX-License-Identifier: GPL-2.0-only

#include <device/device.h>
#include <soc/addressmap.h>
#include <soc/sdram.h>

static void read_resources(struct device *dev)
{
	ram_from_to(dev, 0, EIC7700_DRAM, EIC7700_DRAM + eic7700_dram_size());
}

static struct device_operations cpu_ops = {
	.read_resources = read_resources,
	.set_resources = noop_set_resources,
};

static void enable_dev(struct device *dev)
{
	if (dev->path.type == DEVICE_PATH_CPU_CLUSTER)
		dev->ops = &cpu_ops;
}

struct chip_operations soc_eswin_eic7700_ops = {
	.name = "ESWIN EIC7700",
	.enable_dev = enable_dev,
};
