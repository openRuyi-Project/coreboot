/* SPDX-License-Identifier: GPL-2.0-only */

#include <cbmem.h>
#include <device/device.h>
#include <soc/addressmap.h>
#include <symbols.h>

static void read_resources(struct device *dev)
{
	ram_from_to(dev, 0, FU740_DRAM, cbmem_top());
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

struct chip_operations soc_sifive_fu740_ops = {
	.name = "SIFIVE FU740",
	.enable_dev = enable_dev,
};
