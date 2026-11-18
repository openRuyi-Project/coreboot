/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SOC_ESWIN_EIC7700_USB_BOOT_H
#define SOC_ESWIN_EIC7700_USB_BOOT_H

#include <types.h>
struct device_tree;
bool eic7700_usb_boot(void);
int eic7700_set_boot_usb_role(struct device_tree *tree);
#endif
