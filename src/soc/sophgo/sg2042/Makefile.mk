# SPDX-License-Identifier: GPL-2.0-only
ifeq ($(CONFIG_SOC_SOPHGO_SG2042),y)
CPPFLAGS_common += -Isrc/soc/sophgo/sg2042/include
bootblock-y += early.S boot_device.c memory.c romstage.c timer.c uart.c
ramstage-y += boot_device.c chip.c fdt.c memory.c timer.c uart.c
endif
