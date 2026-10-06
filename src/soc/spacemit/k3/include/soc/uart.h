/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SOC_SPACEMIT_K3_UART_H
#define SOC_SPACEMIT_K3_UART_H

/* Read the configured UART0 clock; zero means unsupported clock state. */
unsigned int k3_uart_clock_hz(void);
/* Set UART0's reference clock before initializing the console divisor. */
int k3_uart_clock_init(void);
#endif
