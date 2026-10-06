# SPDX-License-Identifier: GPL-2.0-only
ifeq ($(CONFIG_SOC_SPACEMIT_K3),y)
CPPFLAGS_common += -I$(src)/soc/spacemit/k3/include
bootblock-y += bootblock.c timer.c uart_clock.c i2c.c cpu_clock.c power_domains.c
bootblock-y += system_clock.c
bootblock-$(CONFIG_SPACEMIT_K3_NATIVE_BOOT) += usb_device.c
bootblock-$(CONFIG_SPACEMIT_K3_QSPI) += qspi.c qspi_clock.c
romstage-$(CONFIG_SPACEMIT_K3_QSPI) += qspi.c qspi_clock.c
ramstage-$(CONFIG_SPACEMIT_K3_QSPI) += qspi.c qspi_clock.c
bootblock-$(CONFIG_SPACEMIT_K3_BOOTROM) += bootrom_entry.S
bootblock-$(CONFIG_SPACEMIT_K3_DDR) += dram/raminit.c dram/lpddr5_init.c
bootblock-$(CONFIG_SPACEMIT_K3_DDR) += dram/lpddr5_training_table.c
bootblock-$(CONFIG_SPACEMIT_K3_DDR) += dram/lpddr5_pre_training_table.c
bootblock-$(CONFIG_SPACEMIT_K3_DDR) += dram/lpddr_training_result.c dram/ddr_init.c dram/lzo1x_decompress.c
bootblock-$(CONFIG_SPACEMIT_K3_DDR) += dram/lpddr4x_init.c dram/lpddr4x_training_table.c
bootblock-$(CONFIG_SPACEMIT_K3_DDR) += dram/lpddr4x_pre_training_table.c dram/lpddr_quick_boot.c
bootblock-$(CONFIG_SPACEMIT_K3_DDR) += dram/lpddr5_quickboot_table.c dram/lpddr4x_quickboot_table.c
bootblock-$(CONFIG_SPACEMIT_K3_EARLY_ESPI) += espi.c early_clock.c
romstage-$(CONFIG_SPACEMIT_K3_EARLY_ESPI) += espi.c early_clock.c
ramstage-$(CONFIG_SPACEMIT_K3_EARLY_ESPI) += espi.c early_clock.c
romstage-y += timer.c uart_clock.c
ramstage-y += timer.c chip.c uart_clock.c power_domains.c
endif
