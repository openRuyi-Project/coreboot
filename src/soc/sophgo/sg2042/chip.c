// SPDX-License-Identifier: GPL-2.0-only
#include <bootstate.h>
#include <delay.h>
#include <device/device.h>
#include <device/mmio.h>
#include <soc/addressmap.h>
#include <soc/memory.h>

static void read_resources(struct device *dev)
{
	struct sg2042_range ranges[SG2042_MAX_RANGES];
	unsigned int count = sg2042_memory_map(ranges);
	for (unsigned int i = 0; i < count; i++)
		ram_from_to(dev, i, ranges[i].base, ranges[i].base + ranges[i].size);
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

struct chip_operations soc_sophgo_sg2042_ops = {
	.name = "SOPHGO SG2042",
	.enable_dev = enable_dev,
};

/* The SCP uses SD before releasing the harts. ZSBL pulses its active-low
 * reset for 100 us before handing the controller to the next stage. */
static void reset_sd_host(void *unused)
{
	const uint32_t sd_reset = BIT(28);
	void *reg = (void *)SG2042_TOP_RESET;

	clrbits32(reg, sd_reset);
	udelay(100);
	setbits32(reg, sd_reset);
}
BOOT_STATE_INIT_ENTRY(BS_PAYLOAD_BOOT, BS_ON_ENTRY, reset_sd_host, NULL);
