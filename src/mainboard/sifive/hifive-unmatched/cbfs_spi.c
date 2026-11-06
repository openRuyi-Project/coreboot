/* SPDX-License-Identifier: GPL-2.0-only */

/* Select the CBFS boot device from the FU740 boot straps. */

#include <boot_device.h>
#include <cbfs.h>
#include <commonlib/region.h>
#include <console/console.h>
#include <device/mmio.h>
#include <lib.h>
#include <soc/addressmap.h>
#include <soc/spi.h>
#include <spi_flash.h>
#include <spi_sdcard.h>
#include <stdint.h>

/* BootROM MSEL values identify the controller and transfer mode. */
#define MSEL_SPI0x1(m)	(((m) == 5) || ((m) == 14))
#define MSEL_SPI0x4(m)	(((m) == 6) || ((m) == 10) || ((m) == 15))
#define MSEL_SPI1x1(m)	((m) == 12)
#define MSEL_SPI1x4(m)	(((m) == 7) || ((m) == 13))
#define MSEL_SPI1SD(m)	((m) ==  8)
#define MSEL_SPI2SD(m)	((m) == 11)

static struct spi_sdcard spi2_sdcard;
static struct spi_flash spi_flash;
static bool spi_flash_init_done;

/* IS25WP256D: READ4 covers 32 MiB without changing the flash address
 * mode expected by the mask ROM. */
static const struct fu740_spi_ffmt_config qspi0_format = {
	.cmd_en = 1,
	.addr_len = 4,
	.cmd_code = 0x13,
};

static const struct mem_region_device qspi0_mdev =
	MEM_REGION_DEV_RO_INIT(FU740_QSPI0FLASH, CONFIG_ROM_SIZE);

static ssize_t sd_readat(const struct region_device *rdev, void *dest,
					size_t offset, size_t count)
{
	spi_sdcard_read(&spi2_sdcard, dest, offset, count);
	return count;
}

static ssize_t spi_readat(const struct region_device *rd, void *b, size_t offset, size_t size)
{
	if (spi_flash_read(&spi_flash, offset, size, b))
		return -1;
	return size;
}

static ssize_t spi_writeat(const struct region_device *rd, const void *b,
				size_t offset, size_t size)
{
	if (spi_flash_write(&spi_flash, offset, size, b))
		return -1;
	return size;
}

static ssize_t spi_eraseat(const struct region_device *rd, size_t offset, size_t size)
{
	if (spi_flash_erase(&spi_flash, offset, size))
		return -1;
	return size;
}

static const struct region_device_ops sd_ops = {
	.mmap   = mmap_helper_rdev_mmap,
	.munmap = mmap_helper_rdev_munmap,
	.readat = sd_readat,
};

static const struct region_device_ops spi_ops = {
	.mmap = mmap_helper_rdev_mmap,
	.munmap = mmap_helper_rdev_munmap,
	.readat = spi_readat,
	.writeat = spi_writeat,
	.eraseat = spi_eraseat,
};

static struct mmap_helper_region_device spi_mdev =
	MMAP_HELPER_DEV_INIT(&spi_ops, 0, CONFIG_ROM_SIZE, &cbfs_cache);

static struct mmap_helper_region_device sd_mdev =
	MMAP_HELPER_DEV_INIT(&sd_ops, 0, CONFIG_ROM_SIZE, &cbfs_cache);

void boot_device_init(void)
{
	if (spi_flash_init_done)
		return;

	uint32_t m = read32((uint32_t *)FU740_MSEL) & 0xf;
	printk(BIOS_DEBUG, "%s MSEL %#x\n", __func__, m);
	if (MSEL_SPI0x1(m) || MSEL_SPI0x4(m)) {
		if (fu740_spi_mmap(0, 25000000, &qspi0_format))
			die("Unable to map QSPI0 flash");
		spi_flash_init_done = true;
		return;
	}

	if (MSEL_SPI1x4(m)) {
		fu740_spi_configs[1].ffmt_config.data_proto = FU740_SPI_PROTO_Q;
		fu740_spi_configs[1].ffmt_config.cmd_code = 0x6b; // quad
		if (spi_flash_probe(1, 0, &spi_flash)) {
			printk(BIOS_EMERG, "SPI1x4 failed to init SPI flash\n");
			return;
		}
		if (fu740_spi_setup(&spi_flash.spi) == -1) {
			printk(BIOS_EMERG, "SPI1x4 failed to configure mmap for SPI flash\n");
		}
	} else if (MSEL_SPI1x1(m)) {
		fu740_spi_configs[1].ffmt_config.data_proto = FU740_SPI_PROTO_S;
		fu740_spi_configs[1].ffmt_config.cmd_code = 0x03; // Normal read
		/* SPI1 reads use programmed I/O. */
		if (spi_flash_probe(1, 0, &spi_flash)) {
			printk(BIOS_EMERG, "failed to init SPI flash\n");
			return;
		}
	} else if (MSEL_SPI2SD(m)) {
		spi_sdcard_init(&spi2_sdcard, 2, 0);
	} else {
		printk(BIOS_EMERG, "MSEL: %#02x: ", m);
		die("unsupported configuration of MSEL\n");
	}

	spi_flash_init_done = true;
}

/* Return the CBFS boot device. */
const struct region_device *boot_device_ro(void)
{
	if (!spi_flash_init_done)
		return NULL;

	uint32_t m = read32((uint32_t *)FU740_MSEL);
	m &= 0xf;
	if (MSEL_SPI0x1(m) || MSEL_SPI0x4(m))
		return &qspi0_mdev.rdev;

	if (MSEL_SPI2SD(m) || MSEL_SPI1SD(m))
		return &sd_mdev.rdev;
	return &spi_mdev.rdev;
}
