/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MAINBOARD_ESWIN_D560_GPIO_H
#define MAINBOARD_ESWIN_D560_GPIO_H

#include <types.h>

/* Die 0 DW APB GPIO ports A and C and the corresponding EIC7702 pads. */
#define D0_GPIO_BASE		0x51600000UL
#define GPIOA_DIRECTION		(D0_GPIO_BASE + 0x04)
#define GPIOC_DIRECTION		(D0_GPIO_BASE + 0x1c)
#define D0_PINCTRL_BASE		(D0_GPIO_BASE + 0x80)
#define I2S0_BCLK_PAD		(D0_PINCTRL_BASE + 40 * 4)
#define FAN_PWM_PAD		(D0_PINCTRL_BASE + 115 * 4)
#define PAD_PULL_UP		BIT(1)
#define PAD_PULL_DOWN		BIT(2)
#define PAD_FUNC_MASK		(7U << 16)
#define PAD_FUNC_GPIO		(2U << 16)

#endif
