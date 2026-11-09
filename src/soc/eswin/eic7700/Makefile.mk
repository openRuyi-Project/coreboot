# SPDX-License-Identifier: GPL-2.0-only
ifeq ($(CONFIG_SOC_ESWIN_EIC7700),y)
CPPFLAGS_common += -Isrc/soc/eswin/eic7700/include
bootblock-y += bootblock.c uart.c clint.c sdram.c cbmem.c boot_device.c
ramstage-y += uart.c clint.c sdram.c cbmem.c boot_device.c chip.c
endif
