// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2026 Spacemit
 */

#include "k3_ddr.h"

static void restore_ddr_training_message(uint32_t dphy_base, uint16_t* training_msg,
	uint32_t msg_size)
{
	int i, j;
	volatile uint32_t* phy_reg = (uint32_t*)(size_t)dphy_base;

	for (i = DDRPHY_DMEM_BASE_ADDR, j = 0; j < msg_size / sizeof(uint16_t); i++, j++) {
		phy_reg[i] = training_msg[j];
	}
}

static void restore_ddr_training_phypara(uint32_t dphy_base, uint16_t* phy_param)
{
	int i, j;
	volatile uint32_t* phy_reg = (uint32_t*)(size_t)dphy_base;

	for (i = DDRPHY_DMEM_BASE_ADDR + DDR_TRAINING_MESSAGE_HWORDS, j = 0;
		 j < DDR_TRAINING_PHYPARA_HWORDS; i++, j++) {
		phy_reg[i] = phy_param[j];
	}
}

static uint32_t restore_acsm_sram_training_data(uint32_t dphy_base, uint16_t* training_data)
{
	int i, j;
	volatile uint32_t* phy_reg = (uint32_t*)(size_t)dphy_base;

	for (i = ACSM_SRAM_BASE_ADDR, j = 0; j < DDR_TRAINING_ACSMSRAM_HWORDS; i++, j++) {
		phy_reg[i] = training_data[j];
	}

	return i;
}

__maybe_unused static uint32_t restore_pstate_sram_training_data(uint32_t dphy_base, uint16_t* training_data)
{
	int i, j;
	volatile uint32_t* phy_reg = (uint32_t*)(size_t)dphy_base;

	for (i = PSTATE_SRAM_BASE_ADDR, j = 0; j < DDR_TRAINING_PSSRAM_HWORDS; i++, j++) {
		phy_reg[i] = training_data[j];
	}

	return i;
}

void init_snps_ddrc_quick(uint32_t ddrc_base, ddr_part_type type, ddr_training_info_t* training_info)
{
	unsigned long dphy_base = ddrc_base + 0x800000;
	volatile uint32_t* phy_reg = (uint32_t*)dphy_base;
	uint16_t *phypara, *acsm_sram, *training_msg;;
	uint32_t msg_size;
	PMU_SMB_LPDDR5_1D_t* lp5_pmu_smb_info;
	PMU_SMB_LPDDR4X_1D_t* lp4x_pmu_smb_info;

	training_msg = (uint16_t*)&training_info->msg;
	phypara = training_info->phypara;
	acsm_sram = training_info->acsm;

	if (DDR_TYPE_LPDDR5 == type) {
		lp5_pmu_smb_info = &training_info->msg;
		msg_size = sizeof(PMU_SMB_LPDDR5_1D_t);

		// Run DevInit - Device/phy initialization
		lp5_pmu_smb_info->SequenceCtrl = 1;
		// Enable Quickboot
		lp5_pmu_smb_info->Quickboot = 1;

		lp5_pmu_smb_info->MR12_A0 = lp5_pmu_smb_info->TrainedVREFCA_A0;
		lp5_pmu_smb_info->MR12_A1 = lp5_pmu_smb_info->TrainedVREFCA_A1;
		lp5_pmu_smb_info->MR12_B0 = lp5_pmu_smb_info->TrainedVREFCA_B0;
		lp5_pmu_smb_info->MR12_B1 = lp5_pmu_smb_info->TrainedVREFCA_B1;
		lp5_pmu_smb_info->MR14_A0 = lp5_pmu_smb_info->TrainedVREFDQ_A0;
		lp5_pmu_smb_info->MR14_A1 = lp5_pmu_smb_info->TrainedVREFDQ_A1;
		lp5_pmu_smb_info->MR14_B0 = lp5_pmu_smb_info->TrainedVREFDQ_B0;
		lp5_pmu_smb_info->MR14_B1 = lp5_pmu_smb_info->TrainedVREFDQ_B1;
		lp5_pmu_smb_info->MR15_A0 = lp5_pmu_smb_info->TrainedVREFDQU_A0;
		lp5_pmu_smb_info->MR15_A1 = lp5_pmu_smb_info->TrainedVREFDQU_A1;
		lp5_pmu_smb_info->MR15_B0 = lp5_pmu_smb_info->TrainedVREFDQU_B0;
		lp5_pmu_smb_info->MR15_B1 = lp5_pmu_smb_info->TrainedVREFDQU_B1;
		lp5_pmu_smb_info->MR24_A0 = lp5_pmu_smb_info->TrainedDRAMDFE_A0;
		lp5_pmu_smb_info->MR24_A1 = lp5_pmu_smb_info->TrainedDRAMDFE_A1;
		lp5_pmu_smb_info->MR24_B0 = lp5_pmu_smb_info->TrainedDRAMDFE_B0;
		lp5_pmu_smb_info->MR24_B1 = lp5_pmu_smb_info->TrainedDRAMDFE_B1;
		lp5_pmu_smb_info->MR30_A0 = lp5_pmu_smb_info->TrainedDRAMDCA_A0;
		lp5_pmu_smb_info->MR30_A1 = lp5_pmu_smb_info->TrainedDRAMDCA_A1;
		lp5_pmu_smb_info->MR30_B0 = lp5_pmu_smb_info->TrainedDRAMDCA_B0;
		lp5_pmu_smb_info->MR30_B1 = lp5_pmu_smb_info->TrainedDRAMDCA_B1;
	} else if (DDR_TYPE_LPDDR4X == type) {
		lp4x_pmu_smb_info = (PMU_SMB_LPDDR4X_1D_t*)&training_info->msg;
		msg_size = sizeof(PMU_SMB_LPDDR4X_1D_t);

		// Run DevInit - Device/phy initialization
		lp4x_pmu_smb_info->SequenceCtrl = 1;
		// Enable Quickboot
		lp4x_pmu_smb_info->Quickboot = 1;

		lp4x_pmu_smb_info->MR12_A0 = lp4x_pmu_smb_info->TrainedVREFCA_A0;
		lp4x_pmu_smb_info->MR12_A1 = lp4x_pmu_smb_info->TrainedVREFCA_A1;
		lp4x_pmu_smb_info->MR12_B0 = lp4x_pmu_smb_info->TrainedVREFCA_B0;
		lp4x_pmu_smb_info->MR12_B1 = lp4x_pmu_smb_info->TrainedVREFCA_B1;
		lp4x_pmu_smb_info->MR14_A0 = lp4x_pmu_smb_info->TrainedVREFDQ_A0;
		lp4x_pmu_smb_info->MR14_A1 = lp4x_pmu_smb_info->TrainedVREFDQ_A1;
		lp4x_pmu_smb_info->MR14_B0 = lp4x_pmu_smb_info->TrainedVREFDQ_B0;
		lp4x_pmu_smb_info->MR14_B1 = lp4x_pmu_smb_info->TrainedVREFDQ_B1;
	} else {
		pr_err("Unsupported DDR type\n");
	}

	// MemReset Toggle
	phy_reg[MICRO_CONT_MUX_SEL] = 0x0;
	phy_reg[0x3f0a2] = 0xf00;
	phy_reg[0x3f042] = 0xf0f;
	phy_reg[0x3f043] = 0xf0f;
	phy_reg[0x3f0a3] = 0x800;
	udelay(30);
	phy_reg[0x20090] = 0x1;
	phy_reg[0x20060] = 0x3;
	mdelay(2);

	// PMU clock start
	phy_reg[MICRO_CONT_MUX_SEL] = 0x0;
	phy_reg[UCCLK_HCLK_ENABLES] = 0x7;
	// APB only
	phy_reg[0xd0036] = 0x0;
	// ECC disable
	phy_reg[0xc0086] = 0x1;

	if (DDR_TYPE_LPDDR5 == type) {
		load_lp5_quickboot_firmware(dphy_base);
		load_lp5_quickboot_dmem(dphy_base);
	} else if (DDR_TYPE_LPDDR4X == type) {
		load_lp4x_quickboot_firmware(dphy_base);
		load_lp4x_quickboot_dmem(dphy_base);
	} else {
		pr_err("Unsupported DDR type\n");
	}
	restore_ddr_training_message(dphy_base, training_msg, msg_size);
	restore_ddr_training_phypara(dphy_base, phypara);

	phy_reg[MICRO_CONT_MUX_SEL] = 0x0000;
	phy_reg[DCT_WRITE_PROT] = 0x0001;

	phy_reg[MICRO_CONT_MUX_SEL] = 0x0001;
	phy_reg[MICRO_RESET] = 0x0009;
	phy_reg[MICRO_RESET] = 0x0001;
	phy_reg[MICRO_RESET] = 0x0000;

	// Wait for the Quickboot firmware run to finish
	major_message_all(dphy_base);

	phy_reg[MICRO_RESET] = 0x0001;
	phy_reg[MICRO_CONT_MUX_SEL] = 0x0000;

	// set csrACSMWckWriteToggleDelayReserved[0] = 0
	phy_reg[0x20037] &= ~BIT(6);

	restore_acsm_sram_training_data(dphy_base, acsm_sram);
	// restore_pstate_sram_training_data(dphy_base, pstate_sram);

	phy_reg[0xD00E7] = 0x0600;
	phy_reg[UCCLK_HCLK_ENABLES] = 0x0002;
	phy_reg[MICRO_CONT_MUX_SEL] = 0x0001;
}

void load_decompressed_quickboot_firmware(uint32_t dphy_base, const uint8_t *fw_data, size_t fw_size)
{
	int i, j, ret;
	size_t decompress_size;
	uint16_t* fw_ptr;
	volatile uint32_t* phy_reg = (uint32_t*)(size_t)dphy_base;

	fw_ptr = (uint16_t*)DDR_QUICKBOOT_FIRMWARE_FW_ADDR;
	decompress_size = DDR_QUICKBOOT_FIRMWARE_FW_MAX_SIZE;
	ret = lzop_decompress((uint8_t*)fw_data, fw_size, (uint8_t*)fw_ptr, &decompress_size);

	if (0 != ret) {
		pr_err("Failed to decompress quickboot firmware\n");
		return;
	}

	for (i = DDRPHY_IMEM_BASE_ADDR, j = 0; j < decompress_size / sizeof(uint16_t); i++, j++) {
		phy_reg[i] = fw_ptr[j];
	}
}

void load_decompressed_quickboot_dmem(uint32_t dphy_base, const uint8_t *dmem_data, size_t dmem_size)
{
	int i, j, ret;
	size_t decompress_size;
	uint16_t* dmem_ptr;
	volatile uint32_t* phy_reg = (uint32_t*)(size_t)dphy_base;

	dmem_ptr = (uint16_t*)DDR_QUICKBOOT_FIRMWARE_FW_ADDR;
	decompress_size = DDR_QUICKBOOT_FIRMWARE_FW_MAX_SIZE;
	ret = lzop_decompress((uint8_t*)dmem_data, dmem_size, (uint8_t*)dmem_ptr, &decompress_size);

	if (0 != ret) {
		pr_err("Failed to decompress quickboot dmem\n");
		return;
	}

	for (i = DDRPHY_DMEM_BASE_ADDR + DDR_TRAINING_MESSAGE_HWORDS + DDR_TRAINING_PHYPARA_HWORDS;
		 i < 0x5aa00; i++) {
		phy_reg[i] = 0;
	}

	for (j = 0; j < decompress_size / sizeof(uint16_t); i++, j++) {
		phy_reg[i] = dmem_ptr[j];
	}

	for (; i < 0x60000; i++) {
		phy_reg[i] = 0;
	}
}
