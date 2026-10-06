// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2026 Spacemit
 */

#include "k3_ddr.h"

static void phyinit_lp4x_pre_training(unsigned int ddrc_base, ddr_part_info* part_info)
{
	unsigned int offset = 0;
	unsigned long DPHY_BASE = ddrc_base + 0x800000;
	volatile uint32_t* dphy_reg = (volatile uint32_t*)(size_t)DPHY_BASE;
	const ddr_phy_reg_config* override_table = NULL;

	if (part_info->ranks > 1 && 0 != part_info->x8_mode) {
		// dual rank, x8 DDR
		override_table = phy_override_seq_lp4x_16g;
	} else if (part_info->ranks > 1 && 0 == part_info->x8_mode) {
		// dual rank, x16 DDR
		override_table = phy_override_seq_lp4x_8g;
	} else {
		pr_info("Use default pre-training table for DDR(%d MB)\n", part_info->size_mb);
	}

	lpddr_training_table_init(ddrc_base, lp4x_pre_train_table, override_table, io_override_table);

	for (offset = 0x582a6; offset < 0x60000; offset++)
		dphy_reg[offset] = 0x0;
}

static void phyinit_lp4x_training(unsigned int ddrc_base, ddr_part_info* part_info)
{
	if (part_info->ranks > 1 && 0 != part_info->x8_mode) {
		// dual rank, x8 DDR
		lpddr_training_table_init(ddrc_base, lp4x_16g_train_table, NULL, NULL);
	} else if (part_info->ranks > 1 && 0 == part_info->x8_mode) {
		// dual rank, x16 DDR
		lpddr_training_table_init(ddrc_base, lp4x_8g_train_table, NULL, NULL);
	} else {
		pr_info("Use default training table for DDR(%d MB)\n", part_info->size_mb);
		lpddr_training_table_init(ddrc_base, lp4x_4g_train_table, NULL, NULL);
	}
}

static const uint32_t tx_impedance_array[] = { 0x10040, 0x10042, 0x10043 };
static const uint32_t tx_impedance_array1[] = { 0x30040, 0x30041, 0x30042, 0x30043 };
static int add_soc_phy_write_ds_config(ddr_phy_reg_config* reg_table,
	uint32_t max_item, uint32_t phy_write_ds)
{
	int i, j, k;

	for (i = 0, k = 0; i < 4; i++) {
		for (j = 0; (j < ARRAY_SIZE(tx_impedance_array)) && (k < max_item); j++, k++) {
			reg_table[k].offset = tx_impedance_array[j] + i * 0x1000;
			reg_table[k].value = phy_write_ds + (phy_write_ds << 8);
		}
	}
	for (i = 0; i < 2; i++) {
		for (j = 0; (j < ARRAY_SIZE(tx_impedance_array1)) && (k < max_item); j++, k++) {
			reg_table[k].offset = tx_impedance_array1[j] + i * 0x1000;
			reg_table[k].value = phy_write_ds + (phy_write_ds << 8);
		}
	}
	return k;
}

static const uint32_t odt_impedance_array[] = { 0x10048, 0x1004a, 0x1004b };
static int add_soc_phy_rx_odt_config(ddr_phy_reg_config* reg_table,
	uint32_t max_item, uint32_t phy_rx_odt)
{
	int i, j, k;
	for (i = 0, k = 0; i < 4; i++) {
		for (j = 0; (j < ARRAY_SIZE(odt_impedance_array)) && (k < max_item); j++, k++) {
			reg_table[k].offset = odt_impedance_array[j] + i * 0x1000;
			reg_table[k].value = phy_rx_odt << 8;
		}
	}

	return k;
}

static int add_ddr_tx_odt_config(ddr_phy_reg_config* reg_table, uint32_t max_item,
	uint32_t dq_odt, uint32_t ca_odt, uint32_t soc_odt, uint32_t pdds, uint32_t x8_mode)
{
	if (max_item < 12) {
		pr_err("Must have at least 12 items for ddr tx odt config");
		return 0;
	}

	dq_odt &= 0x07;
	ca_odt &= 0x07;
	soc_odt &= 0x07;
	pdds &= 0x07;

	reg_table[0].offset = 0x5801a;
	reg_table[0].value = (0x83 ^ (x8_mode << 7)) | (pdds << 3);
	reg_table[1].offset = 0x5801b;
	reg_table[1].value = 0x1100 | (ca_odt << 4) | dq_odt;
	reg_table[2].offset = 0x5801e;
	reg_table[2].value = (x8_mode << 7) | 0x28 | soc_odt;
	reg_table[3].offset = 0x58020;
	reg_table[3].value = (0x83 ^ (x8_mode << 7)) | (pdds << 3);
	reg_table[4].offset = 0x58021;
	reg_table[4].value = 0x1100 | (ca_odt << 4) | dq_odt;
	reg_table[5].offset = 0x58024;
	reg_table[5].value = (x8_mode << 7) | soc_odt;
	reg_table[6].offset = 0x58033;
	reg_table[6].value = (0x833f ^ (x8_mode << 15)) | (pdds << 11);
	reg_table[7].offset = 0x58034;
	reg_table[7].value = (ca_odt << 12) | (dq_odt << 8);
	reg_table[8].offset = 0x58037;
	reg_table[8].value = ((x8_mode << 7) | 0x28 | soc_odt) << 8;
	reg_table[9].offset = 0x58039;
	reg_table[9].value = (0x833f ^ (x8_mode << 15)) | (pdds << 11);
	reg_table[10].offset = 0x5803a;
	reg_table[10].value = (ca_odt << 12) | (dq_odt << 8);
	reg_table[11].offset = 0x5803d;
	reg_table[11].value = ((x8_mode << 7) | soc_odt) << 8;

	return 12;
}

static int add_ddr_2dtraining_config(ddr_phy_reg_config* reg_table, uint32_t max_item,
	uint32_t enable_2d_training)
{
	if (max_item < 1) {
		pr_err("Must have at least 1 item for ddr 2d training config");
		return 0;
	}

	reg_table[0].offset = 0x58049;
	// 0: enable 2D training; 1: disable 2D training
	if (0 != enable_2d_training) {
		// lower byte set LP4X mode, higher byte set 2D training mode
		reg_table[0].value = 0x1;
	} else {
		reg_table[0].value = 0x101;
	}

	return 1;
}

void build_lpddr4x_io_para(const ddr_config_t* io_para, ddr_part_info* part_info)
{
	int i = 0;

	memset(io_override_table, 0, sizeof(io_override_table));

	// MUST NOT change function sequence below
	i += add_soc_phy_write_ds_config(&io_override_table[i], MAX_MODIFIED_IO_PARA_ITEMS - i,
		io_para->phy_write_ds);
	i += add_soc_phy_rx_odt_config(&io_override_table[i], MAX_MODIFIED_IO_PARA_ITEMS - i,
		io_para->phy_rx_odt);
	i += add_ddr_tx_odt_config(&io_override_table[i], MAX_MODIFIED_IO_PARA_ITEMS - i,
		io_para->dq_odt, io_para->ca_odt, io_para->soc_odt,
		io_para->pdds, part_info->x8_mode);
	i += add_ddr_2dtraining_config(&io_override_table[i], MAX_MODIFIED_IO_PARA_ITEMS - i,
		io_para->enable_2d_training);

	pr_info("build %d ddr io parameters complete\n", i);
}

void config_lp4x_addrmap(volatile uint32_t* ddrc_reg, ddr_part_info* part_info)
{
	uint32_t ba0_pos, ba1_pos, ba2_pos, ba0_field, ba1_field, ba2_field;
	uint32_t i, j, msb, ba_msb, region_blks;
	// field value of col0~col9, row0~row17
	uint8_t col_field[10], row_field[18];
	uint32_t addrmap3, addrmap5, addrmap6;
	uint32_t addrmap7, addrmap8, addrmap9, addrmap10, addrmap11, addrmap12;
	bool non_binary_density = false;

	memset(col_field, 0, sizeof(col_field));
	memset(row_field, 0x1F, sizeof(row_field));

	// convert MByte to Byte
	msb = fls(part_info->size_mb) - 1 + 20;
	if (!is_power_of_2(part_info->size_mb)) {
		msb++;
		non_binary_density = true;
	}
	ba_msb = msb;
	pr_info("MSB: %d\n", msb);
	if (part_info->ranks > 1) {
		// MSB is reserved for CS bit
		ba_msb--;
	}

	// address bit position
	ba0_pos = 13;
	ba1_pos = 14;
	ba2_pos = 15;

	pr_info("Configuring LPDDR4x address map with BA0 position: %d, BA1 position: %d, BA2 position: %d\n",
		ba0_pos, ba1_pos, ba2_pos);

	// field_value = HIF_bit - Internal_Base
	// ba0 base=3, HIF=ba0_pos - 3, field=HIF-3
	ba0_field = ba0_pos - 3 - 3;
	// ba1 base=4, HIF=ba1_pos - 3, field=HIF-4
	ba1_field = ba1_pos - 3 - 4;
	// ba2 base=5, HIF=ba2_pos - 3, field=HIF-5
	ba2_field = ba2_pos - 3 - 5;

	// BA0/BA1/COLUMN6~9 use bit position BIT9~BIT15
	// addrmap_col_bN Internal Base=N
	for (i = 9, j = 6; (i <= ba_msb) && (j < ARRAY_SIZE(col_field)); i++) {
		if (i == ba0_pos || i == ba1_pos || i == ba2_pos) {
			continue;
		}

		col_field[j] = i - j - 3;
		j++;
	}

	// BA0/BA1/BA2/ROW0~17
	// addrmap_row_bN Internal Base=N+6
	for (j = 0; (i <= ba_msb) && (j < ARRAY_SIZE(row_field)); i++) {
		if (i == ba0_pos || i == ba1_pos || i == ba2_pos) {
			continue;
		}

		row_field[j] = i - j - 6 - 3;
		j++;
	}

	if (non_binary_density) {
		// ADDRMAP12: [row_msb:row_msb-1]==0b11 is invalid
		addrmap12 = j - 13;
	} else {
		// ADDRMAP12: address mapping extension register, use default value
		addrmap12 = 0x00000000;
	}

	// ADDRMAP3: [21:16]=addrmap_ba_b1, [13:8]=addrmap_ba_b1, [5:0]=addrmap_ba_b0
	addrmap3 = (ba2_field << 16) | (ba1_field << 8) | ba0_field;
	// ADDRMAP5: [28:24]=col10(unused=0x1f), [20:16]=col9, [12:8]=col8, [4:0]=col7
	addrmap5 = (0x1f << 24) | ((uint32_t)col_field[9] << 16)
		| ((uint32_t)col_field[8] << 8) | ((uint32_t)col_field[7] << 0);
	// ADDRMAP6: [27:24]=col6, [19:16]=col5, [11:8]=col4, [3:0]=col3(fixed 0)
	addrmap6 = ((uint32_t)col_field[6] << 24) | ((uint32_t)col_field[5] << 16)
		| ((uint32_t)col_field[4] << 8) | 0;
	// ADDRMAP7: [27:24]=row17, [20:16]=row16, [12:8]=row15, [4:0]=row14
	addrmap7 = ((uint32_t)row_field[17] << 24) | ((uint32_t)row_field[16] << 16)
		| ((uint32_t)row_field[15] << 8) | ((uint32_t)row_field[14] << 0);
	// ADDRMAP8: [27:24]=row13, [20:16]=row12, [12:8]=row11, [4:0]=row10
	addrmap8 = ((uint32_t)row_field[13] << 24) | ((uint32_t)row_field[12] << 16)
		| ((uint32_t)row_field[11] << 8) | ((uint32_t)row_field[10] << 0);
	// ADDRMAP9: [27:24]=row9, [20:16]=row8, [12:8]=row7, [4:0]=row6
	addrmap9 = ((uint32_t)row_field[9] << 24) | ((uint32_t)row_field[8] << 16)
		| ((uint32_t)row_field[7] << 8) | ((uint32_t)row_field[6] << 0);
	// ADDRMAP10: [27:24]=row5, [20:16]=row4, [12:8]=row3, [4:0]=row2
	addrmap10 = ((uint32_t)row_field[5] << 24) | ((uint32_t)row_field[4] << 16)
		| ((uint32_t)row_field[3] << 8) | ((uint32_t)row_field[2] << 0);
	// ADDRMAP11: [12:8]=row1, [4:0]=row0
	addrmap11 = ((uint32_t)row_field[1] << 8) | ((uint32_t)row_field[0]);

	// ---- SAR (System Address Region) and address mapping configuration ----
	// split to 4 regions, each 256MB aligned (SARBASE unit: 256MB)
	region_blks = part_info->size_mb / 4 / 256;
	// SARBASE0: SAR Region0 base address = 8×256MB = 2GB
	ddrc_reg[0x000200c0 / 4] = 8;
	// SARSIZE0: SAR Region0 size = region_blks×256MB
	ddrc_reg[0x000200c4 / 4] = region_blks - 1;
	// SARBASE1: SAR Region1 base address
	ddrc_reg[0x000200c8 / 4] = 8 + region_blks * 1;
	// SARSIZE1: SAR Region1 size
	ddrc_reg[0x000200cc / 4] = region_blks - 1;
	// SARBASE2: SAR Region2 base address
	ddrc_reg[0x000200d0 / 4] = 8 + region_blks * 2;
	// SARSIZE2: SAR Region2 size
	ddrc_reg[0x000200d4 / 4] = region_blks - 1;
	// SARBASE3: SAR Region3 base address
	ddrc_reg[0x000200d8 / 4] = 8 + region_blks * 3;
	// SARSIZE3: SAR Region3 size
	ddrc_reg[0x000200dc / 4] = region_blks - 1;

	if (part_info->ranks > 1) {
		// ADDRMAP1: addrmap_cs_bit0=msb, dual rank = msb - interal base(6) - offset(3)
		ddrc_reg[0x00030004 / 4] = msb - 6 - 3;
	} else {
		// ADDRMAP1: addrmap_cs_bit0=0x3f, single rank, CS mapping disabled
		ddrc_reg[0x00030004 / 4] = 0x0000003f;
	}

	// ADDRMAP3: bank address mapping (BA0/BA1/BA2)
	ddrc_reg[0x0003000c / 4] = addrmap3;
	// ADDRMAP4: bank group address mapping (disabled for LP4x)
	ddrc_reg[0x00030010 / 4] = 0x00003f3f;
	// ADDRMAP5: column address mapping (col7~col10)
	ddrc_reg[0x00030014 / 4] = addrmap5;
	// ADDRMAP6: column address mapping (col3~col6)
	ddrc_reg[0x00030018 / 4] = addrmap6;
	// ADDRMAP7: row address mapping (row14~row17)
	ddrc_reg[0x0003001c / 4] = addrmap7;
	// ADDRMAP8: row address mapping (row10~row13)
	ddrc_reg[0x00030020 / 4] = addrmap8;
	// ADDRMAP9: row address mapping (row6~row9)
	ddrc_reg[0x00030024 / 4] = addrmap9;
	// ADDRMAP10: row address mapping (row2~row5)
	ddrc_reg[0x00030028 / 4] = addrmap10;
	// ADDRMAP11: row address mapping (row0~row1)
	ddrc_reg[0x0003002c / 4] = addrmap11;
	// ADDRMAP12: address mapping extension register
	ddrc_reg[0x00030030 / 4] = addrmap12;
}

void init_snps_lp4x_ddrc(unsigned DDRC_BASE, ddr_part_info* part_info,
	ddr_boot_mode ddr_mode, ddr_training_info_t* training_info)
{
	unsigned int read_data;
	unsigned int CFG_BASE = DDRC_BASE + 0x600000;
	unsigned int DPHY_BASE = DDRC_BASE + 0x800000;
	unsigned int count = 0x100;
	unsigned int rst_code = 22;
	volatile uint32_t* ddrc_reg = (volatile uint32_t*)(size_t)DDRC_BASE;
	volatile uint32_t* cfg_reg = (volatile uint32_t*)(size_t)CFG_BASE;
	volatile uint32_t* dphy_reg = (volatile uint32_t*)(size_t)DPHY_BASE;

	ddrc_reg[0x00010b84 / 4] = 0x00000001;
	if (1 == part_info->ranks) {
		// active_ranks=0x1(1CS), burst_rdwr=8(BL16), lpddr4=1
		ddrc_reg[0x00010000 / 4] = 0x01080002;
	} else {
		// active_ranks=0x3(2CS), burst_rdwr=8(BL16), lpddr4=1
		ddrc_reg[0x00010000 / 4] = 0x03080002;
	}

	if (1 == part_info->x8_mode) {
		ddrc_reg[0x00010010 / 4] = 0x00000000;
		ddrc_reg[0x00010100 / 4] = 0x00000000;
		ddrc_reg[0x00010104 / 4] = 0x0000000f;
		ddrc_reg[0x00010108 / 4] = 0x0000000f;
		ddrc_reg[0x00010118 / 4] = 0x00000001;
		ddrc_reg[0x00010180 / 4] = 0x00020001;
		ddrc_reg[0x00010184 / 4] = 0x00000000;
		ddrc_reg[0x0001018c / 4] = 0x00000000;
		ddrc_reg[0x00010200 / 4] = 0x00000309;
		ddrc_reg[0x00010220 / 4] = 0x1f030601;
		ddrc_reg[0x00010224 / 4] = 0x00000017;
		ddrc_reg[0x00010280 / 4] = 0x20000000;
		ddrc_reg[0x00010288 / 4] = 0x00000001;
		ddrc_reg[0x00010300 / 4] = 0x00670067;
		ddrc_reg[0x00010308 / 4] = 0x00000001;
		ddrc_reg[0x00010380 / 4] = 0xa0012014;
		ddrc_reg[0x00010384 / 4] = 0x80002000;
		ddrc_reg[0x00010390 / 4] = 0x001c0001;
		ddrc_reg[0x00010500 / 4] = 0x00100111;
		ddrc_reg[0x00010508 / 4] = 0x40008000;
		ddrc_reg[0x00010510 / 4] = 0x00010005;
		ddrc_reg[0x00010518 / 4] = 0x6d000001;
		ddrc_reg[0x00010580 / 4] = 0x01110110;
		ddrc_reg[0x00010c90 / 4] = 0x0000e00c;
		ddrc_reg[0x00010c94 / 4] = 0x00000000;

		ddrc_reg[0x00010d00 / 4] = 0xc0030008;

		ddrc_reg[0x00010f00 / 4] = 0x80186180;
		ddrc_reg[0x00020004 / 4] = 0x00004000;
		ddrc_reg[0x00020008 / 4] = 0x00004000;
		ddrc_reg[0x00020094 / 4] = 0x00220003;
		ddrc_reg[0x00020098 / 4] = 0x00b10226;
		ddrc_reg[0x0002009c / 4] = 0x01100c07;
		ddrc_reg[0x000200a0 / 4] = 0x00f60532;

		ddrc_reg[0x00021004 / 4] = 0x00010000;
		ddrc_reg[0x00021008 / 4] = 0x00000000;
		ddrc_reg[0x00021098 / 4] = 0x0094039b;
		ddrc_reg[0x0002109c / 4] = 0x01000a08;
		ddrc_reg[0x000210a0 / 4] = 0x00930645;
		ddrc_reg[0x00022004 / 4] = 0x00004000;
		ddrc_reg[0x00022008 / 4] = 0x00002000;
		ddrc_reg[0x00022094 / 4] = 0x00000009;
		ddrc_reg[0x00022098 / 4] = 0x01e804cc;
		ddrc_reg[0x0002209c / 4] = 0x01110e04;
		ddrc_reg[0x000220a0 / 4] = 0x057a0011;
		ddrc_reg[0x00023004 / 4] = 0x00004000;
		ddrc_reg[0x00023008 / 4] = 0x00006000;
		ddrc_reg[0x00023094 / 4] = 0x00010000;
		ddrc_reg[0x00023098 / 4] = 0x003a0150;
		ddrc_reg[0x0002309c / 4] = 0x01000b07;
		ddrc_reg[0x000230a0 / 4] = 0x06ca0086;
		ddrc_reg[0x00024004 / 4] = 0x00002000;
		ddrc_reg[0x00024008 / 4] = 0x00002000;
		ddrc_reg[0x00024094 / 4] = 0x00210001;
		ddrc_reg[0x00024098 / 4] = 0x02970668;
		ddrc_reg[0x0002409c / 4] = 0x01100603;
		ddrc_reg[0x000240a0 / 4] = 0x05460187;
		ddrc_reg[0x00000000 / 4] = 0x6840925a;
		ddrc_reg[0x00000004 / 4] = 0x00101080;
		ddrc_reg[0x00000008 / 4] = 0x12283435;
		ddrc_reg[0x0000000c / 4] = 0x001e517a;
		ddrc_reg[0x00000010 / 4] = 0x27081027;
		ddrc_reg[0x00000014 / 4] = 0x040b2020;
		ddrc_reg[0x00000018 / 4] = 0x00000012;
		ddrc_reg[0x00000024 / 4] = 0x00040000;
		ddrc_reg[0x00000030 / 4] = 0x00040000;
		ddrc_reg[0x00000038 / 4] = 0x0056033b;
		ddrc_reg[0x00000060 / 4] = 0x00000000;
		ddrc_reg[0x00000064 / 4] = 0x00026a10;
		ddrc_reg[0x00000078 / 4] = 0x003b4208;
		ddrc_reg[0x00000080 / 4] = 0x00000000;
		ddrc_reg[0x00000500 / 4] = 0x00fc003f;
		ddrc_reg[0x00000504 / 4] = 0x00020020;
		ddrc_reg[0x00000508 / 4] = 0x00640032;
		ddrc_reg[0x0000050c / 4] = 0x00000001;
		ddrc_reg[0x00000580 / 4] = 0x0223020c;
		ddrc_reg[0x00000584 / 4] = 0x000a0202;
		ddrc_reg[0x00000588 / 4] = 0x0000230c;
		ddrc_reg[0x000005a0 / 4] = 0x00030303;
		ddrc_reg[0x000005a4 / 4] = 0x00000302;
		ddrc_reg[0x000005a8 / 4] = 0x022a000d;
		ddrc_reg[0x000005ac / 4] = 0x00040005;
		ddrc_reg[0x000005b0 / 4] = 0x0000002e;
		ddrc_reg[0x000005b4 / 4] = 0x40000009;
		ddrc_reg[0x000005b8 / 4] = 0x000001cd;
		ddrc_reg[0x00000600 / 4] = 0xc7032084;
		ddrc_reg[0x00000604 / 4] = 0x032b0196;
		ddrc_reg[0x00000608 / 4] = 0x00c00000;
		ddrc_reg[0x0000060c / 4] = 0x00000000;
		ddrc_reg[0x00000650 / 4] = 0x00000196;
		ddrc_reg[0x00000800 / 4] = 0x00400855;
		ddrc_reg[0x00000804 / 4] = 0x06b00063;
		ddrc_reg[0x00000a80 / 4] = 0x00001780;
		ddrc_reg[0x00000b00 / 4] = 0x15d4e973;
		ddrc_reg[0x00000b04 / 4] = 0x2b5e2b14;
		ddrc_reg[0x00000b08 / 4] = 0x00000088;
		ddrc_reg[0x00000b80 / 4] = 0x0f910000;
		ddrc_reg[0x00000c88 / 4] = 0x0f000000;
		ddrc_reg[0x00000d04 / 4] = 0x00000d0d;
		ddrc_reg[0x00000d08 / 4] = 0x00003404;
		ddrc_reg[0x00000d0c / 4] = 0x001e0035;
		ddrc_reg[0x00000d30 / 4] = 0x00379def;
		ddrc_reg[0x00000d34 / 4] = 0x35009f4b;
	} else {
		ddrc_reg[0x00010010 / 4] = 0x00000000;
		ddrc_reg[0x00010100 / 4] = 0x00000001;
		ddrc_reg[0x00010104 / 4] = 0x00000005;
		ddrc_reg[0x00010108 / 4] = 0x00000005;
		ddrc_reg[0x00010118 / 4] = 0x00000001;
		ddrc_reg[0x00010180 / 4] = 0x00000001;
		ddrc_reg[0x00010184 / 4] = 0x00000000;
		ddrc_reg[0x0001018c / 4] = 0x00000000;
		ddrc_reg[0x00010200 / 4] = 0x00000355;
		ddrc_reg[0x00010220 / 4] = 0x1f030601;
		ddrc_reg[0x00010224 / 4] = 0x00000017;
		ddrc_reg[0x00010280 / 4] = 0x20000000;
		ddrc_reg[0x00010288 / 4] = 0x00000001;
		ddrc_reg[0x00010300 / 4] = 0x00670067;
		ddrc_reg[0x00010308 / 4] = 0x00000001;
		ddrc_reg[0x00010380 / 4] = 0xa0013e14;
		ddrc_reg[0x00010384 / 4] = 0x80002000;
		ddrc_reg[0x00010390 / 4] = 0x001c0001;
		ddrc_reg[0x00010500 / 4] = 0x00100111;
		ddrc_reg[0x00010508 / 4] = 0x40008000;
		ddrc_reg[0x00010510 / 4] = 0x00010005;
		ddrc_reg[0x00010518 / 4] = 0x00000001;
		ddrc_reg[0x00010580 / 4] = 0x01110110;
		ddrc_reg[0x00010c90 / 4] = 0x0000e00c;
		ddrc_reg[0x00010c94 / 4] = 0x00000003;

		ddrc_reg[0x00010d00 / 4] = 0xc0030008;

		ddrc_reg[0x00010f00 / 4] = 0x80186180;
		ddrc_reg[0x00020004 / 4] = 0x00004000;
		ddrc_reg[0x00020008 / 4] = 0x00004000;
		ddrc_reg[0x00020094 / 4] = 0x00100003;
		ddrc_reg[0x00020098 / 4] = 0x00b10226;
		ddrc_reg[0x0002009c / 4] = 0x01100c07;
		ddrc_reg[0x000200a0 / 4] = 0x00f60532;

		ddrc_reg[0x00021004 / 4] = 0x00010000;
		ddrc_reg[0x00021008 / 4] = 0x00000000;
		ddrc_reg[0x00021094 / 4] = 0x00100000;
		ddrc_reg[0x00021098 / 4] = 0x0094039b;
		ddrc_reg[0x0002109c / 4] = 0x01000a08;
		ddrc_reg[0x000210a0 / 4] = 0x00930645;
		ddrc_reg[0x00022004 / 4] = 0x00004000;
		ddrc_reg[0x00022008 / 4] = 0x00002000;
		ddrc_reg[0x00022094 / 4] = 0x00210009;
		ddrc_reg[0x00022098 / 4] = 0x01e804cc;
		ddrc_reg[0x0002209c / 4] = 0x01110e04;
		ddrc_reg[0x000220a0 / 4] = 0x057a0011;
		ddrc_reg[0x00023004 / 4] = 0x00004000;
		ddrc_reg[0x00023008 / 4] = 0x00006000;
		ddrc_reg[0x00023094 / 4] = 0x00010000;
		ddrc_reg[0x00023098 / 4] = 0x003a0150;
		ddrc_reg[0x0002309c / 4] = 0x01000b07;
		ddrc_reg[0x000230a0 / 4] = 0x06ca0086;
		ddrc_reg[0x00024004 / 4] = 0x00002000;
		ddrc_reg[0x00024008 / 4] = 0x00002000;
		ddrc_reg[0x00024094 / 4] = 0x00010001;
		ddrc_reg[0x00024098 / 4] = 0x02970668;
		ddrc_reg[0x0002409c / 4] = 0x01100603;
		ddrc_reg[0x000240a0 / 4] = 0x05460187;
		ddrc_reg[0x00000000 / 4] = 0x6440925a;
		ddrc_reg[0x00000004 / 4] = 0x00101080;
		ddrc_reg[0x00000008 / 4] = 0x12243031;
		ddrc_reg[0x0000000c / 4] = 0x001e4d76;
		ddrc_reg[0x00000010 / 4] = 0x27081027;
		ddrc_reg[0x00000014 / 4] = 0x040b2020;
		ddrc_reg[0x00000018 / 4] = 0x00000012;
		ddrc_reg[0x00000024 / 4] = 0x00040000;
		ddrc_reg[0x00000030 / 4] = 0x00040000;
		ddrc_reg[0x00000038 / 4] = 0x0056033b;
		ddrc_reg[0x00000060 / 4] = 0x00000000;
		ddrc_reg[0x00000064 / 4] = 0x00026610;
		ddrc_reg[0x00000078 / 4] = 0x00373e08;
		ddrc_reg[0x00000080 / 4] = 0x00000000;
		ddrc_reg[0x00000500 / 4] = 0x00fc003f;
		ddrc_reg[0x00000504 / 4] = 0x00820000;
		ddrc_reg[0x00000508 / 4] = 0x00640032;
		ddrc_reg[0x0000050c / 4] = 0x00000001;
		ddrc_reg[0x00000580 / 4] = 0x021f020c;
		ddrc_reg[0x00000584 / 4] = 0x000a0202;
		ddrc_reg[0x00000588 / 4] = 0x00001f0c;
		ddrc_reg[0x000005a0 / 4] = 0x00030303;
		ddrc_reg[0x000005a4 / 4] = 0x00000302;
		ddrc_reg[0x000005a8 / 4] = 0x022a000d;
		ddrc_reg[0x000005ac / 4] = 0x0008000c;
		ddrc_reg[0x000005b0 / 4] = 0x0000002e;
		ddrc_reg[0x000005b4 / 4] = 0x40000008;
		ddrc_reg[0x000005b8 / 4] = 0x000001ce;
		ddrc_reg[0x00000600 / 4] = 0xc0032084;
		ddrc_reg[0x00000604 / 4] = 0x032b0196;
		ddrc_reg[0x00000608 / 4] = 0x00c00000;
		ddrc_reg[0x0000060c / 4] = 0x00000000;
		ddrc_reg[0x00000650 / 4] = 0x00000196;
		ddrc_reg[0x00000800 / 4] = 0x00400855;
		ddrc_reg[0x00000804 / 4] = 0x06b00063;
		ddrc_reg[0x00000a80 / 4] = 0x00000dd0;
		ddrc_reg[0x00000b00 / 4] = 0x15d4e973;
		ddrc_reg[0x00000b04 / 4] = 0x2b5e2b14;
		ddrc_reg[0x00000b08 / 4] = 0x00000088;
		ddrc_reg[0x00000b80 / 4] = 0x0f910000;
		ddrc_reg[0x00000c88 / 4] = 0x0f000000;
		ddrc_reg[0x00000d04 / 4] = 0x00000d0d;
		ddrc_reg[0x00000d08 / 4] = 0x00003007;
		ddrc_reg[0x00000d0c / 4] = 0x001e0026;
		ddrc_reg[0x00000d30 / 4] = 0x00379def;
		ddrc_reg[0x00000d34 / 4] = 0x35009f4b;
	}

	// ---- SAR (System Address Region) and address mapping configuration ----
	config_lp4x_addrmap(ddrc_reg, part_info);

	ddrc_reg[0x00010b84 / 4] = 0x00000000;
	ddrc_reg[0x00020090 / 4] = 0x00000001;
	ddrc_reg[0x00021090 / 4] = 0x00000001;
	ddrc_reg[0x00022090 / 4] = 0x00000001;
	ddrc_reg[0x00023090 / 4] = 0x00000001;
	ddrc_reg[0x00024090 / 4] = 0x00000001;
	ddrc_reg[0x00010208 / 4] = 0x00000001;

	cfg_reg[0x18 / 4] |= (1 << rst_code); // RELEASE FOR DCLK
	cfg_reg[0x18 / 4] |= (1 << 1);

	ddrc_reg[0x00010180 / 4] = 0x00000001;
	ddrc_reg[0x00010180 / 4] = 0x00000000;
	ddrc_reg[0x00010100 / 4] = 0x00000000;
	ddrc_reg[0x00010c80 / 4] = 0x00000000;
	ddrc_reg[0x00010510 / 4] = 0x00010004;
	ddrc_reg[0x00010c80 / 4] = 0x00000001;

	read_data = ddrc_reg[0x00010c84 / 4];
	while ((read_data & 0x00000001) != 0x00000001) {
		read_data = ddrc_reg[0x00010c84 / 4];
	}
	ddrc_reg[0x00010c80 / 4] = 0x00000000;
	ddrc_reg[0x00010510 / 4] = 0x00010014;
	ddrc_reg[0x00010c80 / 4] = 0x00000001;
	read_data = ddrc_reg[0x00010c84 / 4];
	while ((read_data & 0x00000001) != 0x00000001) {
		read_data = ddrc_reg[0x00010c84 / 4];
	}
	ddrc_reg[0x00010208 / 4] = 0x00000000;

	if (DDR_QUICKBOOT_MODE != ddr_mode) {
		phyinit_lp4x_pre_training(DDRC_BASE, part_info);

		ddrc_reg[0x00010180 / 4] |= (0x1 << 11);

		dphy_reg[0xd0000] = 0x1;
		dphy_reg[0xd0099] = 0x9;
		dphy_reg[0xd0099] = 0x1;
		dphy_reg[0xd0099] = 0x0;
		major_message_all(DPHY_BASE);
		dphy_reg[0xd0099] = 0x1;
		while (count--)
			;
		dphy_reg[0xd0000] = 0x0;

		phyinit_lp4x_training(DDRC_BASE, part_info);

		// save DDR training info
		save_snps_ddrc_training_result(DDRC_BASE, training_info);
	} else {
		init_snps_ddrc_quick(DDRC_BASE, DDR_TYPE_LPDDR4X, training_info);
	}

	ddrc_reg[0x00010c80 / 4] = 0x00000000;
	ddrc_reg[0x00010510 / 4] = 0x00010034;
	ddrc_reg[0x00010c80 / 4] = 0x00000001;
	read_data = ddrc_reg[0x00010c84 / 4];
	while ((read_data & 0x00000001) != 0x00000001) {
		read_data = ddrc_reg[0x00010c84 / 4];
	}
	read_data = ddrc_reg[0x00010514 / 4];
	while ((read_data & 0x00000001) != 0x00000001) {
		read_data = ddrc_reg[0x00010514 / 4];
	}
	ddrc_reg[0x00010c80 / 4] = 0x00000000;
	ddrc_reg[0x00010510 / 4] = 0x00010014;
	ddrc_reg[0x00010c80 / 4] = 0x00000001;
	read_data = ddrc_reg[0x00010c84 / 4];
	while ((read_data & 0x00000001) != 0x00000001) {
		read_data = ddrc_reg[0x00010c84 / 4];
	}
	ddrc_reg[0x00010c80 / 4] = 0x00000000;
	ddrc_reg[0x00010510 / 4] = 0x00010015;
	ddrc_reg[0x00010c80 / 4] = 0x00000001;
	read_data = ddrc_reg[0x00010c84 / 4];
	while ((read_data & 0x00000001) != 0x00000001) {
		read_data = ddrc_reg[0x00010c84 / 4];
	}
	if (1 == part_info->x8_mode)
		ddrc_reg[0x00010180 / 4] = 0x00020000;
	else
		ddrc_reg[0x00010180 / 4] = 0x00000000;

	read_data = ddrc_reg[0x00010014 / 4];
	while ((read_data & 0x00000003) != 0x00000001) {
		read_data = ddrc_reg[0x00010014 / 4];
	}
	if (1 == part_info->x8_mode)
		ddrc_reg[0x00000a80 / 4] = 0x00001780;
	else // dsty_4GB || dsty_8GB
		ddrc_reg[0x00000a80 / 4] = 0x00000dd0;

	read_data = ddrc_reg[0x00010090 / 4];
	while ((read_data & 0x00000001) != 0x00000000) {
		read_data = ddrc_reg[0x00010090 / 4];
	}
	ddrc_reg[0x00010080 / 4] = 0x00000010;
	ddrc_reg[0x00010084 / 4] = 0x00001740;
	ddrc_reg[0x00010080 / 4] = 0x80000010;
	read_data = ddrc_reg[0x00010090 / 4];
	while ((read_data & 0x00000001) != 0x00000000) {
		read_data = ddrc_reg[0x00010090 / 4];
	}

	if (part_info->ranks > 1) {
		ddrc_reg[0x00010080 / 4] = 0x00000020;
		ddrc_reg[0x00010084 / 4] = 0x00001740;
		ddrc_reg[0x00010080 / 4] = 0x80000020;
	}
	ddrc_reg[0x00010208 / 4] = 0x00000000;
	if (1 == part_info->x8_mode) {
		ddrc_reg[0x00010180 / 4] = 0x00020000;
		ddrc_reg[0x00010180 / 4] = 0x00020001;
		ddrc_reg[0x00010100 / 4] = 0x00000000;
		ddrc_reg[0x00000a80 / 4] = 0x00001780;
	}
	ddrc_reg[0x00010180 / 4] = 0x00000000;
	ddrc_reg[0x00010180 / 4] = 0x00000001;
	ddrc_reg[0x00010100 / 4] = 0x00000001;
	ddrc_reg[0x00000a80 / 4] = 0x00000dd0;
}
