# SPDX-License-Identifier: GPL-2.0-only

bootblock-y += romstage.c
bootblock-y += spi.c
ramstage-y += spi.c
ifeq ($(CONFIG_HIFIVE_UNMATCHED_JTAG_BOOT),y)
bootblock-y += cbfs_ram.c
ramstage-y += cbfs_ram.c
else
bootblock-y += cbfs_spi.c
ramstage-y += cbfs_spi.c
endif
ramstage-y += fixup_fdt.c

DTB=$(obj)/hifive-unmatched.dtb
DTS=src/mainboard/sifive/hifive-unmatched/hifive-unmatched-a00.dts
$(DTB): $(DTS)
	dtc -I dts -O dtb -o $(DTB) $(DTS)

cbfs-files-y += fallback/DTB
fallback/DTB-file := $(DTB)
fallback/DTB-type := raw
