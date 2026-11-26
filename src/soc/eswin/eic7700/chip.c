// SPDX-License-Identifier: GPL-2.0-only

#include <device/device.h>
#include <soc/addressmap.h>
#include <soc/sdram.h>

static void read_resources(struct device *dev)
{
	struct eic770x_bank banks[4];
	unsigned int count = eic770x_memory_map(banks);

	for (unsigned int i = 0; i < count; i++)
		ram_from_to(dev, i, banks[i].base, banks[i].base + banks[i].size);
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
