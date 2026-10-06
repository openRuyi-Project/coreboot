// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2025 Spacemit
 */

#ifndef _K3_DDR_H_
#define _K3_DDR_H_

#include "coreboot.h"

#include "lpddr_msg_block.h"

#define DDR_CONFIG_BYPASS_MAGIC	(0xdeadbeef)

// support 4266MT/s, 5500MT/s, 6000MT/s, 6400MT/s
#define CONFIG_DDR_DATARATE	(6400)

// 2D training configuration
#define DISABLE_DDR_2D_TRAINING	(0)

#define MAX_MODIFIED_IO_PARA_ITEMS	(256)

#define DDR_TRAINING_FIRMWARE_TABLE_ADDR	(DDR_TRAINING_INFO_BUFF)

#define DDR_QUICKBOOT_FIRMWARE_FW_ADDR		(DDR_TRAINING_INFO_BUFF + sizeof(struct ddr_info_t))
#define DDR_QUICKBOOT_FIRMWARE_FW_MAX_SIZE	(0x10000 - sizeof(struct ddr_info_t))

#if (CONFIG_DDR_DATARATE != 6400) && (CONFIG_DDR_DATARATE != 6000) \
	&& (CONFIG_DDR_DATARATE != 5500) && (CONFIG_DDR_DATARATE != 4266)
#error "Unsupported DDR datarate"
#endif

#ifndef REG32
#define REG32(x) (*((volatile uint32_t*)((uintptr_t)(x))))
#endif

#define TRAINING_DEBUG	0
#define LOGLEVEL	0

#define UCT_WRITE_ONLY			(0xC0032)
#define UCT_WRITE_PROT			(0xC0033)
#define UCCLK_HCLK_ENABLES		(0xC0080)
#define MICRO_CONT_MUX_SEL		(0xD0000)
#define UCT_SHADOW			(0xD0004)
#define DCT_WRITE_ONLY			(0xD0030)
#define DCT_WRITE_PROT			(0xD0031)
#define UCT_WRITE_ONLY_SHADOW		(0xD0032)
#define UCT_DATA_WRITE_ONLY_SHADOW	(0xD0034)
#define MICRO_RESET			(0xD0099)

#define DDRPHY_IMEM_BASE_ADDR		(0x50000)
#define DDRPHY_DMEM_BASE_ADDR		(0x58000)
#define ACSM_SRAM_BASE_ADDR		(0x41000)
#define PSTATE_SRAM_BASE_ADDR		(0xA0000)

#define DDR_TRAINING_MESSAGE_HWORDS	(0x200)
#define DDR_TRAINING_PHYPARA_HWORDS	(5170)
#define DDR_TRAINING_ACSMSRAM_HWORDS	(4096)
#define DDR_TRAINING_PSSRAM_HWORDS	(1 << 14)

#define LogMsg(level, format, args...)		\
	do {					\
		if (level < LOGLEVEL)		\
			printf(format, ##args);	\
	} while (0)

typedef enum {
	DDR_TYPE_LPDDR4X = 0,
	DDR_TYPE_LPDDR5,
	DDR_TYPE_UNKNOWN
} ddr_part_type;

typedef enum {
	DDR_TRAINING_MODE = 0,
	DDR_QUICKBOOT_MODE
} ddr_boot_mode;

typedef struct {
	uint16_t offset;
	uint16_t value;
} discrete_sequence;

typedef struct {
	uint16_t value0;
	uint16_t value1;
} linear_sequence;

typedef struct {
	uint32_t base;
	uint16_t count;
	uint16_t is_linear_increase;

	union {
		discrete_sequence a;
		linear_sequence b;
	} sequence[];
} phy_init_config;

typedef struct {
	uint32_t offset;
	uint32_t value;
} ddr_phy_reg_config;

typedef struct {
	const char *part_number;
	uint32_t crc32_value;
	uint8_t type;
	uint8_t ranks;
	uint8_t x8_mode;
	uint32_t size_mb;
	uint32_t data_rate_mtps;
} ddr_part_info;

typedef enum {
	PHY_R_OFF = 0,
	PHY_R_120 = 0x08,
	PHY_R_60 = 0x0C,
	PHY_R_40 = 0x0E,
	PHY_R_30 = 0x0F,
} phy_odt_e;

typedef enum {
	R_OFF = 0,
	R_240,
	R_120,
	R_80,
	R_60,
	R_48,
	R_40,
} ddr_odt_e;

typedef struct {
	uint8_t ddr_type;
	uint8_t phy_write_ds;
	uint8_t phy_rx_odt;
	uint8_t dq_odt;
	uint8_t ca_odt;
	uint8_t nt_odt;
	uint8_t soc_odt;
	uint8_t pdds;
	uint8_t enable_2d_training;
} ddr_config_t;

typedef struct {
	// make sure all ddr type has the same length of message
	PMU_SMB_LPDDR5_1D_t msg;
	uint16_t phypara[DDR_TRAINING_PHYPARA_HWORDS];
	uint16_t acsm[DDR_TRAINING_ACSMSRAM_HWORDS];
} ddr_training_info_t;

extern const uint8_t lp5_training_fw[], lp4x_training_fw[];
extern const phy_init_config *lp5_pre_train_table[];
extern const phy_init_config *lp5_4g_train_table[], *lp5_train_table[];
extern const ddr_phy_reg_config phy_override_pre_seq_lp5_4g[], phy_override_pre_seq_lp5_16g[];
extern const ddr_phy_reg_config phy_override_seq_lp5_16g[];

extern const phy_init_config *lp4x_pre_train_table[];
extern const phy_init_config *lp4x_4g_train_table[], *lp4x_8g_train_table[], *lp4x_16g_train_table[];
extern const ddr_phy_reg_config phy_override_seq_lp4x_8g[], phy_override_seq_lp4x_16g[];

extern ddr_phy_reg_config io_override_table[MAX_MODIFIED_IO_PARA_ITEMS];

extern void lpddr_init_prepare(ddr_part_info* part_info, ddr_boot_mode ddr_mode);
extern int lp5_training_prepare(void);
extern int lp4x_training_prepare(void);

extern void fpga_ddr_init(void);
extern void lpddr_silicon_init(uint64_t ddrc_reg_base, ddr_part_info* part_info,
	ddr_boot_mode ddr_mode, ddr_training_info_t* training_info);
extern int get_tlvinfo(uint8_t id, uint8_t *buffer, int max_size);

extern uint32_t major_message_all(unsigned int dphy_base);
extern void lpddr_training_table_init(unsigned int ddrc_base, const phy_init_config* train_table[],
	const ddr_phy_reg_config* override_table, ddr_phy_reg_config* io_table);
extern void init_snps_lp4x_ddrc(unsigned DDRC_BASE, ddr_part_info* part_info,
	ddr_boot_mode ddr_mode, ddr_training_info_t* training_info);

extern void save_snps_ddrc_training_result(uint32_t ddrc_base, ddr_training_info_t* training_info);
extern void init_snps_ddrc_quick(uint32_t ddrc_base, ddr_part_type type,
	ddr_training_info_t* training_info);

extern const ddr_config_t* get_ddr_default_io_para(ddr_part_type type);
extern void build_lpddr4x_io_para(const ddr_config_t* io_para, ddr_part_info* part_info);

extern void load_lp5_quickboot_firmware(uint32_t dphy_base);
extern void load_lp5_quickboot_dmem(uint32_t dphy_base);
extern void load_lp4x_quickboot_firmware(uint32_t dphy_base);
extern void load_lp4x_quickboot_dmem(uint32_t dphy_base);

extern void load_decompressed_quickboot_firmware(uint32_t dphy_base,
	const uint8_t *fw_data, size_t fw_size);
extern void load_decompressed_quickboot_dmem(uint32_t dphy_base,
	const uint8_t *dmem_data, size_t dmem_size);
/* Declarations required by coreboot's prototype checks. */
void accept_message(uint32_t dphy_base);
void config_lp4x_addrmap(volatile uint32_t *ddrc_reg, ddr_part_info *part_info);
void config_lp5_addrmap(volatile uint32_t *ddrc_reg, ddr_part_info *part_info);
void init_snps_lp5_ddrc(unsigned int base, ddr_part_info *part_info,
	ddr_boot_mode mode, ddr_training_info_t *training_info);
uint32_t ddr_get_density(void);
#endif /* _K3_DDR_H_ */
