## SPDX-License-Identifier: GPL-2.0-only
bootblock-y += bootblock.c romstage.c cbmem.c media.c uart.c power.c
romstage-y += romstage.c cbmem.c media.c uart.c
ramstage-y += cbmem.c media.c uart.c
CPPFLAGS_common += -I$(src)/mainboard/$(MAINBOARDDIR)/include
bootblock-$(CONFIG_SPACEMIT_K3_EARLY_ESPI) += early_ec.c retimer.c ec_update.c
bootblock-$(CONFIG_SPACEMIT_K3_DDR) += raminit.c
bootblock-$(CONFIG_SPACEMIT_K3_NATIVE_BOOT) += native_boot.c native_fdt.c
bootblock-$(CONFIG_SPACEMIT_K3_QSPI) += flash.c
ramstage-$(CONFIG_SPACEMIT_K3_NATIVE_BOOT) += cpu_config.c
ramstage-$(CONFIG_SPACEMIT_K3_QSPI) += flash.c
romstage-$(CONFIG_SPACEMIT_K3_EARLY_ESPI) += early_ec.c
ramstage-$(CONFIG_SPACEMIT_K3_EARLY_ESPI) += early_ec.c

bootblock-$(CONFIG_SPACEMIT_K3_NATIVE_BOOT) += fastboot_load.c
