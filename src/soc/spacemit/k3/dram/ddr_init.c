// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2025 Spacemit
 */

#include <soc/raminit.h>
#include "k3_ddr.h"

#define DDR_CHECK_SIZE			(0x4000)
#define DDR_CHECK_STEP			(0x2000)
#define DDR_CHECK_CNT			(0x1000)

// place part_info in .data section to avoid it being cleared during bss clear
__section(".data") static ddr_part_info* part_info;

static const ddr_config_t ddr_default_io_para[] = {
	// type,              WDS     RX ODT   DQODT CAODT NTODT  SOCODT PDDS  2DTraining
	{ DDR_TYPE_LPDDR5, PHY_R_30, PHY_R_60, R_60, R_80, R_OFF, R_OFF, R_40, 1 },
	{ DDR_TYPE_LPDDR4X, PHY_R_40, PHY_R_40, R_60, R_40, R_OFF, R_40, R_40, 1 }
};

ddr_phy_reg_config io_override_table[MAX_MODIFIED_IO_PARA_ITEMS];

const ddr_config_t* get_ddr_default_io_para(ddr_part_type type)
{
	for (int i = 0; i < ARRAY_SIZE(ddr_default_io_para); i++) {
		if (ddr_default_io_para[i].ddr_type == type) {
			return &ddr_default_io_para[i];
		}
	}

	pr_err("NOT supported DDR type %d, using LPDDR5 as default\n", type);
	return &ddr_default_io_para[0];
}

static int test_pattern(fdt_addr_t base, fdt_size_t size)
{
	fdt_addr_t addr;
	fdt_size_t check_size;
	uint32_t offset;
	uint32_t* ddr_data = NULL;
	uint32_t* save_data;
	int err;

	err = 0;

	check_size = (DDR_CHECK_SIZE / DDR_CHECK_STEP) * DDR_CHECK_CNT;
	if (check_size > DDR_QUICKBOOT_FIRMWARE_FW_MAX_SIZE) {
		pr_err("backup size exceeds reserved quickboot area, need 0x%lx (max 0x%lx)\n",
		       (unsigned long)check_size,
		       (unsigned long)DDR_QUICKBOOT_FIRMWARE_FW_MAX_SIZE);
		return -1;
	}

	/*
	 * DDR restore can consume most of SPL malloc_f before full
	 * malloc is available. Reuse the quickboot firmware area after DDR
	 * init, while keeping the training info header intact.
	 */
	ddr_data = (uint32_t *)DDR_QUICKBOOT_FIRMWARE_FW_ADDR;

	save_data = ddr_data;
	/* to avoid overlap important data as image or ramdump  */
	for (addr = base; addr < base + size; addr += DDR_CHECK_STEP) {
		for (offset = 0; offset < DDR_CHECK_CNT; offset += 4) {
			*save_data = readl((void*)addr + offset);
			save_data++;
		}
	}

	for (addr = base; addr < base + size; addr += DDR_CHECK_STEP) {
		for (offset = 0; offset < DDR_CHECK_CNT; offset += 4) {
			writel((uint32_t)(addr + offset), (void*)addr + offset);
		}
	}
	for (addr = base; addr < base + size; addr += DDR_CHECK_STEP) {
		for (offset = 0; offset < DDR_CHECK_CNT; offset += 4) {
			if (readl((void*)addr + offset) != (uint32_t)(addr + offset)) {
				pr_err("ddr check error %x vs %x\n", (uint32_t)(addr + offset), readl((void*)addr + offset));
				err++;
				if (err > 10)
					goto ERR_HANDLE;
			}
		}
	}

	for (addr = base; addr < base + size; addr += DDR_CHECK_STEP) {
		for (offset = 0; offset < DDR_CHECK_CNT; offset += 4) {
			writel((~(uint32_t)(addr + offset)), (void*)addr + offset);
		}
	}
	for (addr = base; addr < base + size; addr += DDR_CHECK_STEP) {
		for (offset = 0; offset < DDR_CHECK_CNT; offset += 4) {
			if (readl((void*)addr + offset) != (~(uint32_t)(addr + offset))) {
				pr_err("ddr check error %x vs %x\n", (uint32_t)(~(addr + offset)), readl((void*)addr + offset));
				err++;
				if (err > 10)
					goto ERR_HANDLE;
			}
		}
	}

ERR_HANDLE:
	save_data = ddr_data;
	for (addr = base; addr < base + size; addr += DDR_CHECK_STEP) {
		for (offset = 0; offset < DDR_CHECK_CNT; offset += 4) {
			writel(*save_data, (void*)addr + offset);
			save_data++;
		}
	}
	if (err == 0)
		pr_info("memory verify pass\n");
	else
		printf("memory verify fail!\n");

	return err;
}

const ddr_part_info ddr_parts_info[] = {
	{ "MT62F1G32D2DS", 0x0FD38DD9, DDR_TYPE_LPDDR5, 1, 0, 4096, CONFIG_DDR_DATARATE },
	{ "MT62F2G32D4DS", 0x85D1F688, DDR_TYPE_LPDDR5, 2, 0, 8192, CONFIG_DDR_DATARATE },
	{ "RS3G32LG5D8FDB", 0xF74C6BFC, DDR_TYPE_LPDDR5, 2, 1, 12288, CONFIG_DDR_DATARATE },
	{ "MT62F4G32D8DV", 0x3ACEF2E4, DDR_TYPE_LPDDR5, 2, 1, 16384, CONFIG_DDR_DATARATE },
	{ "MT53E1G32D2FW", 0x75251AB8, DDR_TYPE_LPDDR4X, 1, 0, 4096, 4266 },
	{ "MT53E2G32D4DE", 0x3EA87223, DDR_TYPE_LPDDR4X, 2, 0, 8192, 4266 },
	{ "MT53E4G32D8CY", 0xAA9D4848, DDR_TYPE_LPDDR4X, 2, 1, 16384, 4266 },
};

static ddr_part_info* find_ddr_info(const char *part_number)
{
	int i;
	uint32_t crc_code = crc32(0, (const uint8_t*)part_number, strlen(part_number));

	for (i = 0; i < ARRAY_SIZE(ddr_parts_info); i++) {
		// use crc32 code to speed up the comparison
		if ((crc_code == ddr_parts_info[i].crc32_value) &&
		(0 == strcmp(ddr_parts_info[i].part_number, part_number))) {
			return (ddr_part_info*)&ddr_parts_info[i];
		}
	}

	// use first element as default
	return (ddr_part_info*)&ddr_parts_info[0];
}

uint32_t ddr_get_density(void)
{
	// there are two ddr parts
	if (NULL != part_info)
		return part_info->size_mb * 2;
	return 0;
}

static int ddr_eeprom_part(const uint8_t *data, size_t size, char *part, size_t limit)
{
	if (!data || size < 11 || memcmp(data, "TlvInfo\0", 8) || data[8] != 1)
		return -1;
	size_t end = 11 + read_be16(data + 9);
	if (end < 17 || end > size || data[end - 6] != 0xfe || data[end - 5] != 4 ||
	    crc32(0, data, end - 4) != read_be32(data + end - 4))
		return -1;
	for (size_t pos = 11; pos + 2 <= end; pos += 2 + data[pos + 1]) {
		size_t length = data[pos + 1];
		if (!data[pos] || data[pos] == 0xff || pos + 2 + length > end)
			return -1;
		if (data[pos] == 0x45) {
			length = MIN(length, limit);
			memcpy(part, data + pos + 2, length);
			return length;
		}
	}
	return -1;
}

uint64_t k3_dram_init(const char *fallback_part, const uint8_t *eeprom, size_t eeprom_size)
{
	int ret;
	char ddr_part_number[32];
	const char *temp;
	uint64_t ddrc0, ddrc1;
	unsigned long timestamp;
	struct ddr_info_t *ddr_info;
	ddr_boot_mode ddr_mode;
	ddr_training_info_t *ddr0_training_info, *ddr1_training_info;

	ddrc0 = 0xcb000000;
	ddrc1 = 0xcc000000;

	ret = ddr_eeprom_part(eeprom, eeprom_size, ddr_part_number, sizeof(ddr_part_number) - 1);
	if (ret <= 0) {
		temp = fallback_part;
		if (NULL == temp)
			die("Missing board DDR part number\n");
		ret = strnlen(temp, sizeof(ddr_part_number) - 1);
		memcpy(ddr_part_number, temp, ret);
		printk(BIOS_INFO, "DDR part source: board default\n");
	} else {
		printk(BIOS_INFO, "DDR part source: EEPROM\n");
	}
	ddr_part_number[ret] = '\0';

	part_info = find_ddr_info((const char*)ddr_part_number);
	printf("DDR Part Number: %s, Size: %dMB, Data Rate: %dMT/s\n",
		part_info->part_number, part_info->size_mb, part_info->data_rate_mtps);
	if ((DDR_TYPE_LPDDR5 != part_info->type) && (DDR_TYPE_LPDDR4X != part_info->type)) {
		pr_err("unsupported ddr type %d\n", part_info->type);
		die("Unsupported DDR type\n");
	}

	/* DDR training info may save and restore from differents space:
	1. write to private partition during uboot stage, restore it during spl stage
	2. update to SPL rodata space and write to FSBL partition during fastboot flash,
		load with spl during bootrom.
	*/
	ddr_info = (struct ddr_info_t*)DDR_TRAINING_INFO_BUFF;
	if ((DDR_TRAINING_INFO_MAGIC == ddr_info->magic)
		&& (ddr_info->type == part_info->type)
		&& (ddr_info->cs_num == part_info->ranks)
		&& (ddr_info->data_rate == part_info->data_rate_mtps)
		&& (ddr_info->crc32
			== crc32(0, (const uint8_t*)&ddr_info->chipid, sizeof(struct ddr_info_t) - 8))) {
		ddr_mode = DDR_QUICKBOOT_MODE;
	} else {
		if (DDR_TYPE_LPDDR5 == part_info->type) {
			ddr_info = (struct ddr_info_t*)lp4x_training_fw;
		} else {
			ddr_info = (struct ddr_info_t*)lp5_training_fw;
		}

		if ((DDR_TRAINING_INFO_MAGIC == ddr_info->magic)
			&& (ddr_info->type == part_info->type)
			&& (ddr_info->cs_num == part_info->ranks)
			&& (ddr_info->data_rate == part_info->data_rate_mtps)
			&& (ddr_info->crc32
				== crc32(0, (const uint8_t*)&ddr_info->chipid, sizeof(struct ddr_info_t) - 8))) {
			ddr_mode = DDR_QUICKBOOT_MODE;
		} else {
			/* reuse memory space of training firmware(compressed) to save training results
			Use it only after firmware has been decompressed to DDR_TRAINING_FIRMWARE_TABLE_ADDR
			*/
			ddr_mode = DDR_TRAINING_MODE;
		}
	}

	ddr0_training_info = (ddr_training_info_t*)&ddr_info->training_info[0];
	ddr1_training_info = (ddr_training_info_t*)&ddr_info->training_info[sizeof(ddr_training_info_t)];

	lpddr_init_prepare(part_info, ddr_mode);

	timestamp = get_timer(0);
	lpddr_silicon_init(ddrc0, part_info, ddr_mode, ddr0_training_info);
	lpddr_silicon_init(ddrc1, part_info, ddr_mode, ddr1_training_info);
	timestamp = get_timer(timestamp);

	// clear DDR training flag
	memset(ddr_info, 0, 128);
	if (DDR_TRAINING_MODE == ddr_mode) {
		ddr_info->magic = DDR_TRAINING_INFO_MAGIC;
		ddr_info->type = part_info->type;
		ddr_info->cs_num = part_info->ranks;
		ddr_info->data_rate = part_info->data_rate_mtps;
		ddr_info->crc32 =
			crc32(0, (const uint8_t*)&ddr_info->chipid, sizeof(struct ddr_info_t) - 8);
		printf("DDR training consume %ldms\n", timestamp);
		// in case need to write training info to local storage
		memcpy((void*)DDR_TRAINING_INFO_BUFF, ddr_info, sizeof(struct ddr_info_t));
	} else {
		printf("DDR quick boot consume %ldms\n", timestamp);
	}
	ret = test_pattern(CONFIG_SYS_SDRAM_BASE, DDR_CHECK_SIZE);
	if (ret < 0) {
		while (1);
	}
	pr_info("init done\n");

	return (uint64_t)ddr_get_density() * MiB;
}
