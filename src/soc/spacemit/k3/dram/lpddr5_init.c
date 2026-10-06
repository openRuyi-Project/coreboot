// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2025 Spacemit
 */

#include "k3_ddr.h"

void lpddr_training_table_init(uint32_t ddrc_base, const phy_init_config* train_table[],
	const ddr_phy_reg_config* override_table, ddr_phy_reg_config* io_table)
{
	uint32_t reg_base, reg_offset, phy_value;
	unsigned long DPHY_BASE = ddrc_base + 0x800000;
	volatile uint32_t* phy_reg = (uint32_t*)DPHY_BASE;
	uint16_t* phy_data;
	const phy_init_config* sub_table;
	int i, j, k0, k1;
	bool need_table_check = false, need_io_check = false;

	for (i = 0, k0 = 0, k1 = 0; NULL != train_table[i]; i++) {
		sub_table = train_table[i];
		reg_base = sub_table->base;

		if ((NULL != override_table) && ((override_table[k0].offset & ~0x7FFF) == reg_base)) {
			need_table_check = true;
		} else {
			need_table_check = false;
		}

		if ((NULL != io_table) && ((io_table[k1].offset & ~0x7FFF) == reg_base)) {
			need_io_check = true;
		} else {
			need_io_check = false;
		}

		if (sub_table->is_linear_increase) {
			phy_data = (uint16_t*)sub_table->sequence;
			for (j = 0; j < sub_table->count; j++) {
				reg_offset = reg_base + j;
				phy_value = phy_data[j];

				if (need_table_check && (override_table[k0].offset == reg_offset)) {
					phy_value = override_table[k0++].value;
					// skip the PHY setting when value is 0xdeadbeef
					if (DDR_CONFIG_BYPASS_MAGIC == phy_value) {
						continue;
					}
					if ((override_table[k0].offset & ~0x7FFF) != reg_base) {
						need_table_check = false;
					}
				}

				// configuration in IO table has higher priority
				if (need_io_check && (io_table[k1].offset == reg_offset)) {
					phy_value = io_table[k1++].value;
					if ((io_table[k1].offset & ~0x7FFF) != reg_base) {
						need_io_check = false;
					}
				}

				phy_reg[reg_offset] = phy_value;
			}
		} else {
			for (j = 0; j < sub_table->count; j++) {
				reg_offset = reg_base + sub_table->sequence[j].a.offset;
				phy_value = sub_table->sequence[j].a.value;
				if (need_table_check && (override_table[k0].offset == reg_offset)) {
					phy_value = override_table[k0++].value;
					if ((override_table[k0].offset & ~0x7FFF) != reg_base) {
						need_table_check = false;
					}
					// skip the PHY setting when value is 0xdeadbeef
					if (DDR_CONFIG_BYPASS_MAGIC == phy_value) {
						continue;
					}
				}

				// configuration in IO table has higher priority
				if (need_io_check && (io_table[k1].offset == reg_offset)) {
					phy_value = io_table[k1++].value;
					if ((io_table[k1].offset & ~0x7FFF) != reg_base) {
						need_io_check = false;
					}
				}

				phy_reg[reg_offset] = phy_value;
			}
		}
	}
}

static const uint32_t tx_impedance_array[] = { 0x10040, 0x10042, 0x10043, 0x10044, 0x10045 };
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
static const uint32_t odt_impedance_array[] = { 0x10048, 0x1004a, 0x1004b, 0x1004c, 0x1004d };
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

static int add_ddr_pdds_config(ddr_phy_reg_config* reg_table, uint32_t max_item,
	uint32_t pdds)
{
	uint16_t value;

	if (max_item < 2) {
		pr_err("Must have at least 2 items for ddr pdds config");
		return 0;
	}

	value = pdds | (pdds << 8);
	reg_table[0].offset = 0x58031;
	reg_table[0].value = value;
	reg_table[1].offset = 0x58032;
	reg_table[1].value = value;
	return 2;
}

static int add_ddr_odt_config(ddr_phy_reg_config* reg_table, uint32_t max_item,
	uint32_t dq_odt, uint32_t ca_odt, uint32_t nt_odt)
{
	int enable = nt_odt == R_OFF ? 0 : 1;
	uint16_t value;

	if (max_item < 2) {
		pr_err("Must have at least 2 items for ddr odt config");
		return 0;
	}

	value = dq_odt | (ca_odt << 4) | (enable << 3);
	value |= value << 8;
	reg_table[0].offset = 0x58035;
	reg_table[0].value = value;
	reg_table[1].offset = 0x58036;
	reg_table[1].value = value;

	return 2;
}

static int add_ddr_socodt_config(ddr_phy_reg_config* reg_table, uint32_t max_item,
	uint32_t soc_odt, uint32_t x8_mode)
{
	uint16_t value;

	if (max_item < 2) {
		pr_err("Must have at least 2 items for ddr soc odt config");
		return 0;
	}

	value = BIT(3) | BIT(5);
	value |= soc_odt | (soc_odt << 8);
	value |= (x8_mode << 7) | (x8_mode << 15);
	reg_table[0].offset = 0x58041;
	reg_table[0].value = value;
	reg_table[1].offset = 0x58042;
	reg_table[1].value = value;

	return 2;
}

static int add_ddr_ntodt_config(ddr_phy_reg_config* reg_table, uint32_t max_item,
	uint32_t nt_odt)
{
	uint16_t value;

	if (max_item < 2) {
		pr_err("Must have at least 2 items for ddr ntodt config");
		return 0;
	}

	value = nt_odt << 5;
	value |= value << 8;
	reg_table[0].offset = 0x58065;
	reg_table[0].value = value;
	reg_table[1].offset = 0x58066;
	reg_table[1].value = value;

	return 2;
}

static int add_ddr_2dtraining_config(ddr_phy_reg_config* reg_table, uint32_t max_item,
	uint32_t enable_2d_training)
{
	if (max_item < 1) {
		pr_err("Must have at least 1 item for ddr 2d training config");
		return 0;
	}

	reg_table[0].offset = 0x58067;
	if (0 != enable_2d_training) {
		// 0: enable 2D training; 1: disable 2D training
		reg_table[0].value = 0;
	} else {
		reg_table[0].value = 1;
	}

	return 1;
}

static void build_lpddr5_io_para(const ddr_config_t* io_para, ddr_part_info* part_info)
{
	int i = 0;

	memset(io_override_table, 0, sizeof(io_override_table));

	// MUST NOT change function sequence below
	i += add_soc_phy_write_ds_config(&io_override_table[i], MAX_MODIFIED_IO_PARA_ITEMS - i,
		io_para->phy_write_ds);
	i += add_soc_phy_rx_odt_config(&io_override_table[i], MAX_MODIFIED_IO_PARA_ITEMS - i,
		io_para->phy_rx_odt);
	i += add_ddr_pdds_config(&io_override_table[i], MAX_MODIFIED_IO_PARA_ITEMS - i,
		io_para->pdds);
	i += add_ddr_odt_config(&io_override_table[i], MAX_MODIFIED_IO_PARA_ITEMS - i,
		io_para->dq_odt, io_para->ca_odt, io_para->nt_odt);
	i += add_ddr_socodt_config(&io_override_table[i], MAX_MODIFIED_IO_PARA_ITEMS - i,
		io_para->soc_odt, part_info->x8_mode);
	i += add_ddr_ntodt_config(&io_override_table[i], MAX_MODIFIED_IO_PARA_ITEMS - i,
		io_para->nt_odt);
	i += add_ddr_2dtraining_config(&io_override_table[i], MAX_MODIFIED_IO_PARA_ITEMS - i,
		io_para->enable_2d_training);

	pr_info("build %d ddr io parameters complete\n", i);
}

static void phyinit_lp5_pre_training(uint32_t ddrc_base, ddr_part_info* part_info)
{
	uint32_t offset = 0;
	unsigned long DPHY_BASE = ddrc_base + 0x800000;
	volatile uint32_t* dphy_reg = (volatile uint32_t*)(size_t)DPHY_BASE;
	const ddr_phy_reg_config* override_table;

	if (1 == part_info->ranks && 0 == part_info->x8_mode) {
		// single rank, x16 DDR
		override_table = phy_override_pre_seq_lp5_4g;
	} else if (part_info->ranks > 1 && 0 != part_info->x8_mode) {
		// dual rank, x8 DDR
		override_table = phy_override_pre_seq_lp5_16g;
	} else {
		override_table = NULL;
		pr_info("Use default pre-training talbe\n");
	}

	lpddr_training_table_init(ddrc_base, lp5_pre_train_table, override_table, io_override_table);

	for (offset = 0x584d2; offset < 0x60000; offset++)
		dphy_reg[offset] = 0x0;
}

static void phyinit_lp5_training(uint32_t ddrc_base, ddr_part_info* part_info)
{
	const ddr_phy_reg_config* override_table;

	if (1 == part_info->ranks && 0 == part_info->x8_mode) {
		// single rank, x16 DDR
		lpddr_training_table_init(ddrc_base, lp5_4g_train_table, NULL, NULL);
	} else {
		if (part_info->ranks > 1 && 0 != part_info->x8_mode) {
			// dual rank, x8 DDR
			override_table = phy_override_seq_lp5_16g;
		} else {
			override_table = NULL;
		}
		lpddr_training_table_init(ddrc_base, lp5_train_table, override_table, NULL);
	}
}

#if TRAINING_DEBUG
void translate_streaming(uint32_t* d)
{
}
#endif

void accept_message(uint32_t dphy_base)
{
	volatile uint32_t* dphy_reg = (volatile uint32_t*)(size_t)dphy_base;
	uint32_t read_data;

	dphy_reg[0x000d0031] = 0x00000000;
	read_data = dphy_reg[0x000d0004];
	while ((read_data & 0x00000001) != 0x00000001) {
		read_data = dphy_reg[0x000d0004];
	}
	dphy_reg[0x000d0031] = 0x00000001;
}

uint32_t major_message_all(uint32_t dphy_base)
{
	volatile uint32_t* dphy_reg = (volatile uint32_t*)(size_t)dphy_base;
	uint32_t read_data;
	uint32_t i;
	uint32_t j;
	uint32_t cnt = 0x1000;
#if TRAINING_DEBUG
	uint32_t read_data1;
	static uint32_t dmsg[50];
#endif

	read_data = dphy_reg[0x000d0004];
	while ((read_data & 0x00000001) != 0x00000000) {
		read_data = dphy_reg[0x000d0004];
	}
	read_data = dphy_reg[0x000d0032];

	while ((read_data & 0x000000ff) != 0x00000007) {
		if ((read_data & 0x000000ff) == 0x00000008) {
			accept_message(dphy_base);

			read_data = dphy_reg[0x000d0004];
			while ((read_data & 0x00000001) != 0x00000000) {
				read_data = dphy_reg[0x000d0004];
			}
			j = dphy_reg[0x000d0032];
			i = 0;
			while (i <= j) {

				read_data = dphy_reg[0x000d0032];
#if TRAINING_DEBUG
				read_data1 = dphy_reg[0x000d0034];
				dmsg[i] = (read_data1 << 16) | read_data;
				// LogMsg(0,"read dmsg 0x%08X\n",dmsg[i]);
				if (i == j) {
					translate_streaming(dmsg);
				}
#endif
				accept_message(dphy_base);
				i++;
				read_data = dphy_reg[0x000d0004];
				while ((read_data & 0x00000001) != 0x00000000) {
					read_data = dphy_reg[0x000d0004];
				}
			}

		} else {
			LogMsg(0, "== Training major message ==\n");
			LogMsg(0, "%02x\n", read_data);

			if (read_data == 0xff) {
				dphy_reg[0xd0099] = 0x1;
				while (cnt--)
					;
				dphy_reg[0xd0000] = 0x0;
				read_data = dphy_reg[0x200c9];
				LogMsg(0, "plllockstatus is %02x\n", read_data);
				return read_data;
			}
			// while(cnt--);
			LogMsg(0, "============================\n");
			accept_message(dphy_base);
			read_data = dphy_reg[0x000d0004];
			while ((read_data & 0x00000001) != 0x00000000) {
				read_data = dphy_reg[0x000d0004];
			}
		}
		read_data = dphy_reg[0x000d0032];
	}

	LogMsg(0, "== Training major message ==\n");
	LogMsg(0, "%02x\n", read_data);
	LogMsg(0, "============================\n");

	accept_message(dphy_base);

	return 0;
}

void config_lp5_addrmap(volatile uint32_t* ddrc_reg, ddr_part_info* part_info)
{
	uint32_t ba0_pos, ba1_pos, ba0_field, ba1_field, i, j, msb, ba_msb, region_blks;
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
	ba0_pos = 9;
	ba1_pos = 16;

	pr_info("Configuring LPDDR5 address map with BA0 position: %d, BA1 position: %d\n",
		ba0_pos, ba1_pos);

	// addrmap_bank_b0 Internal Base=3, addrmap_bank_b1 Internal Base=4
	// field_value = HIF_bit - Internal_Base
	// ba0 base=3, HIF=ba0_pos - 3, field=HIF-3
	ba0_field = ba0_pos - 3 - 3;
	// ba1 base=4, HIF=ba1_pos - 3, field=HIF-4
	ba1_field = ba1_pos - 3 - 4;

	// BA0/BA1/COLUMN4~9 use bit position BIT9~BIT16
	// addrmap_col_bN Internal Base=N
	for (i = 9, j = 4; (i <= ba_msb) && (j < ARRAY_SIZE(col_field)); i++) {
		if (i == ba0_pos || i == ba1_pos || i == 8) {
			continue;
		}

		col_field[j] = i - j - 3;
		j++;
	}

	// BA0/BA1/ROW0~17
	// addrmap_row_bN Internal Base=N+6
	for (j = 0; (i <= ba_msb) && (j < ARRAY_SIZE(row_field)); i++) {
		if (i == ba0_pos || i == ba1_pos) {
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

	// ADDRMAP3: [13:8]=addrmap_ba_b1, [5:0]=addrmap_ba_b0, ba2 is unused
	addrmap3 = (0x3F << 16) | (ba1_field << 8) | ba0_field;
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
	// ADDRMAP4: bank group address mapping (BG0/BG1)
	ddrc_reg[0x00030010 / 4] = 0x00000101;
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

void init_snps_lp5_ddrc(unsigned DDRC_BASE, ddr_part_info* part_info,
	ddr_boot_mode ddr_mode, ddr_training_info_t* training_info)
{
	uint32_t read_data;
	uint32_t CFG_BASE = DDRC_BASE + 0x600000;
	uint32_t DPHY_BASE = DDRC_BASE + 0x800000;
	uint32_t count = 0x100;
	uint32_t rst_code = 22;
	volatile uint32_t* ddrc_reg = (volatile uint32_t*)(size_t)DDRC_BASE;
	volatile uint32_t* cfg_reg = (volatile uint32_t*)(size_t)CFG_BASE;
	volatile uint32_t* dphy_reg = (volatile uint32_t*)(size_t)DPHY_BASE;

	if (part_info->ranks > 1) {
		// dual_rank_mode=1, active_ranks=2CS, burst_rdwr=BL16, lpddr5=1
		ddrc_reg[0x00010000 / 4] = 0x03080008;
	} else {
		// dual_rank_mode=0, active_ranks=1CS, burst_rdwr=BL16, lpddr5=1
		ddrc_reg[0x00010000 / 4] = 0x01080008;
	}
	REG32(0xD4282CE8) &= 0x00ffffff;
	REG32(0xD4282CE8) |= ((ddrc_reg[0x00010000 / 4] & 0xff) << 24);

	ddrc_reg[0x00010008 / 4] = 0x00000000;
	ddrc_reg[0x00010510 / 4] = 0x00010005;
	ddrc_reg[0x00010518 / 4] = 0x70000000;
	ddrc_reg[0x00010208 / 4] = 0x00000000;
	if (1 == part_info->ranks)
		ddrc_reg[0x00010200 / 4] = 0x010003f3;
	else
		ddrc_reg[0x00010200 / 4] = 0x00000361;

	ddrc_reg[0x00010280 / 4] = 0x00000000;
	ddrc_reg[0x00010220 / 4] = 0x0a000100;
	ddrc_reg[0x00010224 / 4] = 0x00000000;
	ddrc_reg[0x00010288 / 4] = 0x00000000;
	ddrc_reg[0x00010380 / 4] = 0x80012014;
	ddrc_reg[0x00010100 / 4] = 0x00000020;
	if (0 != part_info->x8_mode) {
		ddrc_reg[0x00010104 / 4] = 0x0000000f;
		ddrc_reg[0x00010108 / 4] = 0x0000000f;
	} else {
		ddrc_reg[0x00010104 / 4] = 0x00000005;
		ddrc_reg[0x00010108 / 4] = 0x00000005;
	}
	ddrc_reg[0x00010118 / 4] = 0x00000000;
	ddrc_reg[0x00010c90 / 4] = 0x0000000f;
	ddrc_reg[0x00010b80 / 4] = 0x00000000;
	ddrc_reg[0x00010300 / 4] = 0x00400040;
	ddrc_reg[0x00010384 / 4] = 0x00002000;
	ddrc_reg[0x0001038c / 4] = 0x04040208;
	ddrc_reg[0x00010390 / 4] = 0x08400810;
	ddrc_reg[0x00010500 / 4] = 0x00100000;
	ddrc_reg[0x00010508 / 4] = 0xc0000000;
	ddrc_reg[0x00010ca0 / 4] = 0x00000000;
	ddrc_reg[0x00010c80 / 4] = 0x00000001;
	ddrc_reg[0x00010f00 / 4] = 0x80008200;
	ddrc_reg[0x00010580 / 4] = 0x00110011;
	ddrc_reg[0x00010ca4 / 4] = 0x00000000;
	ddrc_reg[0x00010308 / 4] = 0x00000000;
	ddrc_reg[0x0001018c / 4] = 0x0000003f;
	ddrc_reg[0x00010184 / 4] = 0x00000003;
	ddrc_reg[0x00010114 / 4] = 0x00000001;
	ddrc_reg[0x00010128 / 4] = 0x00000000;

	ddrc_reg[0x00010c94 / 4] = 0x00000001;

	ddrc_reg[0x00010b84 / 4] = 0x00000003;
	ddrc_reg[0x00010180 / 4] = 0x00000011;
	ddrc_reg[0x00010d00 / 4] = 0x00020002;
	ddrc_reg[0x00010010 / 4] = 0x00000100;
	ddrc_reg[0x00010084 / 4] = 0x00000000;
	ddrc_reg[0x00010284 / 4] = 0x00000000;
	ddrc_reg[0x00010b8c / 4] = 0x00000000;
	ddrc_reg[0x00010b98 / 4] = 0x00000000;
	ddrc_reg[0x000005a8 / 4] = 0x71a4000d;
	ddrc_reg[0x000005a0 / 4] = 0x00000000;
	ddrc_reg[0x000005a4 / 4] = 0x00000300;
	ddrc_reg[0x000005b0 / 4] = 0x00000004;
	ddrc_reg[0x00000d00 / 4] = 0x00000001;
	ddrc_reg[0x000005b4 / 4] = 0xe000012c;

	if (0 != part_info->x8_mode) {
		ddrc_reg[0x00000000 / 4] = 0x29103622;
		ddrc_reg[0x00000004 / 4] = 0x00100630;
		ddrc_reg[0x00000008 / 4] = 0x09121219;
		ddrc_reg[0x0000000c / 4] = 0x000c2230;
		ddrc_reg[0x00000010 / 4] = 0x0f04040f;
		ddrc_reg[0x00000014 / 4] = 0x02040c09;
		ddrc_reg[0x00000018 / 4] = 0x00000012;
		ddrc_reg[0x0000001c / 4] = 0x00000003;
		ddrc_reg[0x00000024 / 4] = 0x00020412;
		ddrc_reg[0x00000030 / 4] = 0x00030000;
		ddrc_reg[0x00000034 / 4] = 0x0c100002;
		ddrc_reg[0x00000038 / 4] = 0x002000e6;
		ddrc_reg[0x00000060 / 4] = 0x0010170e;
		ddrc_reg[0x00000064 / 4] = 0x00002906;
		ddrc_reg[0x00000078 / 4] = 0x001a1419;
		ddrc_reg[0x00000080 / 4] = 0x00030408;

		ddrc_reg[0x00000600 / 4] = 0xc03d0c34;
		ddrc_reg[0x00000604 / 4] = 0x00e00070;
		ddrc_reg[0x00000608 / 4] = 0x06480000;
		ddrc_reg[0x0000060c / 4] = 0x3f000000;
		ddrc_reg[0x00000610 / 4] = 0x00000000;

		ddrc_reg[0x00000800 / 4] = 0x001804d7;
		ddrc_reg[0x00000804 / 4] = 0x02800100;
		ddrc_reg[0x00000d0c / 4] = 0x00400010;
		ddrc_reg[0x00000c84 / 4] = 0x0f00007f;
		ddrc_reg[0x00000b80 / 4] = 0x00000000;
		ddrc_reg[0x00000b04 / 4] = 0x1024100a;
		ddrc_reg[0x00000b08 / 4] = 0x00000033;
		ddrc_reg[0x00000b00 / 4] = 0x00800000;
		ddrc_reg[0x00000d04 / 4] = 0x00000e12;
		ddrc_reg[0x00000580 / 4] = 0x0343021f;
		ddrc_reg[0x00000584 / 4] = 0x00080303;
		ddrc_reg[0x00000588 / 4] = 0x0018431f;
		ddrc_reg[0x00000590 / 4] = 0x1c0c0403;
		ddrc_reg[0x00000594 / 4] = 0x0410000f;
		ddrc_reg[0x00000500 / 4] = 0x00000000;
		ddrc_reg[0x00000504 / 4] = 0x00000000;
		ddrc_reg[0x00000508 / 4] = 0x00000000;
		ddrc_reg[0x0000050c / 4] = 0x00000000;
		ddrc_reg[0x0000005c / 4] = 0x009d0009;
		ddrc_reg[0x00000c00 / 4] = 0x00000000;
		ddrc_reg[0x000005ac / 4] = 0x00010001;
		ddrc_reg[0x000005b8 / 4] = 0x00000147;
		ddrc_reg[0x00000a80 / 4] = 0x00000070;
		ddrc_reg[0x00000d08 / 4] = 0x0000160a;
	} else {
		ddrc_reg[0x00000000 / 4] = 0x28103622;
		ddrc_reg[0x00000004 / 4] = 0x00100630;
		ddrc_reg[0x00000008 / 4] = 0x09111117;
		ddrc_reg[0x0000000c / 4] = 0x000c212f;
		ddrc_reg[0x00000010 / 4] = 0x0f04040f;
		ddrc_reg[0x00000014 / 4] = 0x02040c09;
		ddrc_reg[0x00000018 / 4] = 0x00000012;
		ddrc_reg[0x0000001c / 4] = 0x00000003;
		ddrc_reg[0x00000024 / 4] = 0x00020410;
		ddrc_reg[0x00000030 / 4] = 0x00030000;
		ddrc_reg[0x00000034 / 4] = 0x0c100002;
		ddrc_reg[0x00000038 / 4] = 0x002000e6;
		ddrc_reg[0x00000060 / 4] = 0x000f160e;
		ddrc_reg[0x00000064 / 4] = 0x00002806;
		ddrc_reg[0x00000078 / 4] = 0x00191318;
		ddrc_reg[0x00000080 / 4] = 0x00030408;

		ddrc_reg[0x00000600 / 4] = 0xc03d0c34;
		ddrc_reg[0x00000604 / 4] = 0x00e00070;
		ddrc_reg[0x00000608 / 4] = 0x06480000;
		ddrc_reg[0x0000060c / 4] = 0x3f000000;
		ddrc_reg[0x00000610 / 4] = 0x00000000;

		ddrc_reg[0x00000800 / 4] = 0x001804d7;
		ddrc_reg[0x00000804 / 4] = 0x02800100;
		ddrc_reg[0x00000d0c / 4] = 0x00400010;
		ddrc_reg[0x00000c84 / 4] = 0x0f00007f;
		ddrc_reg[0x00000b80 / 4] = 0x00000000;
		ddrc_reg[0x00000b04 / 4] = 0x1024100a;
		ddrc_reg[0x00000b08 / 4] = 0x00000033;
		ddrc_reg[0x00000b00 / 4] = 0x00800000;
		ddrc_reg[0x00000d04 / 4] = 0x00000e12;
		ddrc_reg[0x00000580 / 4] = 0x033f021f;
		ddrc_reg[0x00000584 / 4] = 0x00080303;
		ddrc_reg[0x00000588 / 4] = 0x00183f1f;
		ddrc_reg[0x00000590 / 4] = 0x180c0403;
		ddrc_reg[0x00000594 / 4] = 0x0410000f;
		ddrc_reg[0x00000500 / 4] = 0x00000000;
		ddrc_reg[0x00000504 / 4] = 0x00000000;
		ddrc_reg[0x00000508 / 4] = 0x00000000;
		ddrc_reg[0x0000050c / 4] = 0x00000000;
		ddrc_reg[0x0000005c / 4] = 0x009d0009;
		ddrc_reg[0x00000c00 / 4] = 0x00000000;
		ddrc_reg[0x000005ac / 4] = 0x00010001;
		ddrc_reg[0x000005b8 / 4] = 0x00000147;
		ddrc_reg[0x00000a80 / 4] = 0x00000070;
		ddrc_reg[0x00000d08 / 4] = 0x0000150b;
	}

	ddrc_reg[0x00000c80 / 4] = 0x0f000001;
	ddrc_reg[0x00000c88 / 4] = 0x0f00007f;
	ddrc_reg[0x00000650 / 4] = 0x00000098;
	ddrc_reg[0x00000d30 / 4] = 0x002faf09;
	ddrc_reg[0x00000d34 / 4] = 0x180009c5;
	ddrc_reg[0x00020000 / 4] = 0x00000000;
	ddrc_reg[0x00020004 / 4] = 0x0000501f;
	ddrc_reg[0x00021004 / 4] = 0x0000501f;
	ddrc_reg[0x00022004 / 4] = 0x0000501f;
	ddrc_reg[0x00023004 / 4] = 0x0000501f;
	ddrc_reg[0x00024004 / 4] = 0x0000501f;
	ddrc_reg[0x00020008 / 4] = 0x0000501f;
	ddrc_reg[0x00021008 / 4] = 0x0000501f;
	ddrc_reg[0x00022008 / 4] = 0x0000501f;
	ddrc_reg[0x00023008 / 4] = 0x0000501f;
	ddrc_reg[0x00024008 / 4] = 0x0000501f;
	ddrc_reg[0x00020090 / 4] = 0x00000000;
	ddrc_reg[0x00021090 / 4] = 0x00000000;
	ddrc_reg[0x00022090 / 4] = 0x00000000;
	ddrc_reg[0x00023090 / 4] = 0x00000000;
	ddrc_reg[0x00024090 / 4] = 0x00000000;
	ddrc_reg[0x00020094 / 4] = 0x00000000;
	ddrc_reg[0x00021094 / 4] = 0x00000000;
	ddrc_reg[0x00022094 / 4] = 0x00000000;
	ddrc_reg[0x00023094 / 4] = 0x00000000;
	ddrc_reg[0x00024094 / 4] = 0x00000000;
	ddrc_reg[0x00020098 / 4] = 0x00000000;
	ddrc_reg[0x00021098 / 4] = 0x00000000;
	ddrc_reg[0x00022098 / 4] = 0x00000000;
	ddrc_reg[0x00023098 / 4] = 0x00000000;
	ddrc_reg[0x00024098 / 4] = 0x00000000;
	ddrc_reg[0x0002009c / 4] = 0x00000e00;
	ddrc_reg[0x0002109c / 4] = 0x00000e00;
	ddrc_reg[0x0002209c / 4] = 0x00000e00;
	ddrc_reg[0x0002309c / 4] = 0x00000e00;
	ddrc_reg[0x0002409c / 4] = 0x00000e00;
	ddrc_reg[0x000200a0 / 4] = 0x00000000;
	ddrc_reg[0x000210a0 / 4] = 0x00000000;
	ddrc_reg[0x000220a0 / 4] = 0x00000000;
	ddrc_reg[0x000230a0 / 4] = 0x00000000;
	ddrc_reg[0x000240a0 / 4] = 0x00000000;

	// ---- SAR (System Address Region) and address mapping configuration ----
	config_lp5_addrmap(ddrc_reg, part_info);

	ddrc_reg[0x00010b84 / 4] = 0x00000002;
	ddrc_reg[0x00010d00 / 4] = 0xc0020002;
	ddrc_reg[0x00010180 / 4] = 0x00000811;
	ddrc_reg[0x00010180 / 4] = 0x00000800;
	ddrc_reg[0x00010208 / 4] = 0x00000001;
	cfg_reg[0x18 / 4] |= (1 << rst_code); // RELEASE FOR DCLK
	cfg_reg[0x18 / 4] |= (1 << 1);
	ddrc_reg[0x00010280 / 4] = 0x80000000;
	ddrc_reg[0x000005b4 / 4] = 0xc000012c;
	ddrc_reg[0x00010c80 / 4] = 0x00000000;
	ddrc_reg[0x00010288 / 4] = 0x00000001;
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
		phyinit_lp5_pre_training(DDRC_BASE, part_info);

		ddrc_reg[0x00010180 / 4] |= (0x1 << 11);

		dphy_reg[0xd0000] = 0x1;
		dphy_reg[0xd0099] = 0x9;
		dphy_reg[0xd0099] = 0x1;
		dphy_reg[0xd0099] = 0x0;
		read_data = major_message_all(DPHY_BASE);
		if (read_data == 0xff)
			return;
		dphy_reg[0xd0099] = 0x1;
		while (count--)
			;
		dphy_reg[0xd0000] = 0x0;

		phyinit_lp5_training(DDRC_BASE, part_info);

		// save DDR training info
		save_snps_ddrc_training_result(DDRC_BASE, training_info);
	} else {
		init_snps_ddrc_quick(DDRC_BASE, DDR_TYPE_LPDDR5, training_info);
	}

	if (0 == part_info->x8_mode) {
		ddrc_reg[0x00010c80 / 4] = 0x00000000;
		ddrc_reg[0x00000060 / 4] = 0x0010160e;
		ddrc_reg[0x00000024 / 4] = 0x00020410;
		ddrc_reg[0x00010c80 / 4] = 0x00000001;
		read_data = ddrc_reg[0x00010c84 / 4];
		while ((read_data & 0x00000001) != 0x00000001) {
			read_data = ddrc_reg[0x00010c84 / 4];
		}
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
	ddrc_reg[0x00010510 / 4] = 0x00010015;
	ddrc_reg[0x00010c80 / 4] = 0x00000001;
	read_data = ddrc_reg[0x00010c84 / 4];
	while ((read_data & 0x00000001) != 0x00000001) {
		read_data = ddrc_reg[0x00010c84 / 4];
	}
	ddrc_reg[0x00010180 / 4] = 0x00000000;
	read_data = ddrc_reg[0x00010014 / 4];
	while ((read_data & 0x00000003) != 0x00000001) {
		read_data = ddrc_reg[0x00010014 / 4];
	}
	ddrc_reg[0x00010c80 / 4] = 0x00000000;
	ddrc_reg[0x00010508 / 4] = 0xc0000000;
	ddrc_reg[0x00010c80 / 4] = 0x00000001;
	read_data = ddrc_reg[0x00010c84 / 4];
	while ((read_data & 0x00000001) != 0x00000001) {
		read_data = ddrc_reg[0x00010c84 / 4];
	}
	ddrc_reg[0x00010280 / 4] = 0x00000000;
	ddrc_reg[0x00010c80 / 4] = 0x00000000;
	ddrc_reg[0x00010288 / 4] = 0x00000000;
	ddrc_reg[0x00010c80 / 4] = 0x00000001;
	read_data = ddrc_reg[0x00010c84 / 4];
	while ((read_data & 0x00000001) != 0x00000001) {
		read_data = ddrc_reg[0x00010c84 / 4];
	}
	ddrc_reg[0x00010208 / 4] = 0x00000000;
	ddrc_reg[0x00010180 / 4] = 0x00000010;
	ddrc_reg[0x000005b4 / 4] = 0xe000012c;
	ddrc_reg[0x00010b84 / 4] = 0x00000000;
	ddrc_reg[0x00020090 / 4] = 0x00000001;
	ddrc_reg[0x00021090 / 4] = 0x00000001;
	ddrc_reg[0x00022090 / 4] = 0x00000001;
	ddrc_reg[0x00023090 / 4] = 0x00000001;
	ddrc_reg[0x00024090 / 4] = 0x00000001;
}

static void init_ddr_clock(uint32_t DDRC_BASE, uint32_t data_rate_mtps)
{
	uint32_t read_data;
	uint32_t CFG_BASE = DDRC_BASE + 0x600000;
	volatile uint32_t* cfg_reg = (volatile uint32_t*)(size_t)CFG_BASE;

	if (5500 == data_rate_mtps) {
		/* DPLL 2750MHz*/
		cfg_reg[0x8 / 4] = 0x0b3912aa;
		cfg_reg[0x10 / 4] = 0xa0558b8b;
		cfg_reg[0xc / 4] |= (0x1 << 22) | (0x1 << 16) | (0xff) | (0xab << 8);
	} else if (6000 == data_rate_mtps) {
		/* DPLL 3000MHz*/
		cfg_reg[0x8 / 4] = 0x0b3e2000;
		cfg_reg[0x10 / 4] = 0xa0558c8c;
		cfg_reg[0xc / 4] |= (0x1 << 22) | (0x1 << 16) | (0xff) | (0x00 << 8);
	} else {
		/* DPLL 3200MHz*/
		cfg_reg[0xc / 4] |= (0x1 << 22) | (0x1 << 16) | (0xff);
	}

	read_data = cfg_reg[0x1c / 4];
	while ((read_data & 0x00000001) != 0x1) {
		read_data = cfg_reg[0x1c / 4];
	}
	// clear frequency divider
	cfg_reg[0x18 / 4] &= ~(0x3f << 16);

	if (1066 == data_rate_mtps) {
		cfg_reg[0x18 / 4] |= (0x1 << 19) | (0x7 << 16); // sel 2, div 8
	} else if (4266 == data_rate_mtps) {
		cfg_reg[0x18 / 4] |= (0x1 << 19) | (0x1 << 16); // sel 2, div 2
		// cfg_reg[0x18 / 4] |= (0x2 << 19) | (0x1 << 16); // sel 2, div 2 3200mbps
	} else if (5120 == data_rate_mtps) {
		cfg_reg[0x18 / 4] |= (0x7 << 19) | (0x0 << 16); // sel 3, div 1 5120mbps
	} else {
		cfg_reg[0x18 / 4] |= (0x2 << 19) | (0x0 << 16); // sel 3, div 1 6400mbps
		// cfg_reg[0x18 / 4] |= (0x1 << 19) | (0x1 << 16); // sel 3, div 1
	}

	// initial frequency change
	cfg_reg[0x18 / 4] |= (1 << 25);
	LogMsg(0, "read 6400 reg 0x%08X 0x%08X\n", CFG_BASE + 0x18, cfg_reg[0x18 / 4]);
	cfg_reg[0x18 / 4] = cfg_reg[0x18 / 4];
	LogMsg(0, "check setting reg 0x%08X 0x%08X\n", 0xD4282CE8, cfg_reg[0x18 / 4]);
	read_data = cfg_reg[0x18 / 4];
	while ((read_data & 0x2000000) != 0x0) {
		read_data = cfg_reg[0x18 / 4];
	}
	cfg_reg[0x18 / 4] |= 0x1;
}

static void init_snps_lp45(unsigned DDRC_BASE, ddr_part_info* part_info,
	ddr_boot_mode ddr_mode, ddr_training_info_t* training_info)
{
	init_ddr_clock(DDRC_BASE, part_info->data_rate_mtps);

	if (DDR_TYPE_LPDDR5 == part_info->type) {
		init_snps_lp5_ddrc(DDRC_BASE, part_info, ddr_mode, training_info);
	} else if (DDR_TYPE_LPDDR4X == part_info->type) {
		init_snps_lp4x_ddrc(DDRC_BASE, part_info, ddr_mode, training_info);
	}
}

void lpddr_init_prepare(ddr_part_info* part_info, ddr_boot_mode ddr_mode)
{
	// ddr para and training firmware need to be initialized before training
	if (DDR_TYPE_LPDDR5 == part_info->type) {
		build_lpddr5_io_para(get_ddr_default_io_para(DDR_TYPE_LPDDR5), part_info);
		if (DDR_QUICKBOOT_MODE != ddr_mode) {
			// during first boot, MUST do fully training
			lp5_training_prepare();
		}
	} else if (DDR_TYPE_LPDDR4X == part_info->type) {
		build_lpddr4x_io_para(get_ddr_default_io_para(DDR_TYPE_LPDDR4X), part_info);
		if (DDR_QUICKBOOT_MODE != ddr_mode) {
			// during first boot, MUST do fully training
			lp4x_training_prepare();
		}
	}
}

void lpddr_silicon_init(uint64_t ddrc_reg_base, ddr_part_info* part_info,
	ddr_boot_mode ddr_mode, ddr_training_info_t* training_info)
{
	LogMsg(0, "=== start init_lpddr() ===\n");
	init_snps_lp45(ddrc_reg_base, part_info, ddr_mode, training_info);
	LogMsg(0, "=== finish init_lpddr() ===\n");
}
