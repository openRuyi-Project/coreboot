/* SPDX-License-Identifier: GPL-2.0-only */
#include <delay.h>
#include <device/mmio.h>
#include <soc/addressmap.h>
#include <soc/power_domains.h>
#include <timer.h>

#define POWER_STATUS 0xf0
#define POWER_REQUEST (1U << 0)
#define POWER_ISOLATION (1U << 1)
#define POWER_SLEEP1 (1U << 2)
#define POWER_SLEEP2 (1U << 3)
#define POWER_HW_MODE (1U << 4)

static const struct {
	unsigned int control, status;
	bool hardware;
} domains[K3_PMDOMAIN_COUNT] = {
	[K3_PMDOMAIN_VPU] = { 0x0a8, 1U << 9, true },
	[K3_PMDOMAIN_GPU] = { 0x0d0, 1U << 8, true },
	[K3_PMDOMAIN_AUDIO] = { 0x378, 1U << 3, false },
	[K3_PMDOMAIN_LCD0] = { 0x380, 1U << 12, true },
	[K3_PMDOMAIN_LCD1] = { 0x3f4, 1U << 15, true },
};

bool k3_power_domain_is_on(unsigned int id)
{
	return id < K3_PMDOMAIN_COUNT &&
		(read32((void *)(K3_APMU_BASE + POWER_STATUS)) & domains[id].status);
}

int k3_power_domain_set(unsigned int id, bool on)
{
	if (id >= K3_PMDOMAIN_COUNT || (id == K3_PMDOMAIN_AUDIO && !on))
		return -1;
	if (k3_power_domain_is_on(id) == on)
		return 0;

	void *control = (void *)(uintptr_t)(K3_APMU_BASE + domains[id].control);
	if (domains[id].hardware) {
		unsigned int mask = POWER_REQUEST | POWER_HW_MODE;
		clrsetbits32(control, mask, on ? mask : 0);
		udelay(on ? 300 : 15);
	} else {
		/* AUDIO uses the software sequence and remains on across handoff. */
		setbits32(control, POWER_SLEEP1);
		udelay(20);
		setbits32(control, POWER_SLEEP1 | POWER_SLEEP2);
		udelay(20);
		setbits32(control, POWER_ISOLATION);
		udelay(15);
	}
	struct stopwatch timeout;
	stopwatch_init_usecs_expire(&timeout, 10000);
	do {
		if (k3_power_domain_is_on(id) == on)
			return 0;
		udelay(5);
	} while (!stopwatch_expired(&timeout));
	return -1;
}
