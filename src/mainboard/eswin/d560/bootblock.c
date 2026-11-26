// SPDX-License-Identifier: GPL-2.0-only

#include <bootblock_common.h>
#include <device/mmio.h>
#include <types.h>

#include "gpio.h"

void bootblock_mainboard_early_init(void)
{
	/*
	 * After S_MODE was sampled at reset, GPIO94 selects the shared CPU
	 * supply. Request 0.9 V here so it settles before the ramstage PLL
	 * changes. Preload the output before connecting it to the pad.
	 */
	setbits32((void *)GPIOC_OUTPUT, CPU_VSEL);
	setbits32((void *)GPIOC_DIRECTION, CPU_VSEL);
	clrsetbits32((void *)CPU_VSEL_PAD, PAD_FUNC_MASK | PAD_PULL_UP | PAD_PULL_DOWN,
		     PAD_FUNC_GPIO | PAD_INPUT_ENABLE);

	/*
	 * J15 on the C560 A3 carrier: R1778 and R286 pull down the inputs
	 * of Q45 (fan power disable) and Q24 (inverted PWM). Release both
	 * pins after SCPU calibration to power the fan and request full speed.
	 * Set the GPIO direction before muxing the pad to avoid a low PWM pulse.
	 */
	clrbits32((void *)GPIOA_DIRECTION, BIT(18));
	clrbits32((void *)GPIOC_DIRECTION, BIT(4)); /* GPIO68 */
	clrsetbits32((void *)I2S0_BCLK_PAD,
		     PAD_FUNC_MASK | PAD_PULL_UP | PAD_PULL_DOWN, PAD_FUNC_GPIO);
	clrsetbits32((void *)FAN_PWM_PAD,
		     PAD_FUNC_MASK | PAD_PULL_UP | PAD_PULL_DOWN, PAD_FUNC_GPIO);
}
