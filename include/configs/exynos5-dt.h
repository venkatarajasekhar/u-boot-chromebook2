/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * Configuration settings for the SAMSUNG EXYNOS5
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_EXYNOS5_DT_H
#define __CONFIG_EXYNOS5_DT_H

/* High Level Configuration Options */
#define CONFIG_SAMSUNG			/* in a SAMSUNG core */
#define CONFIG_S5P			/* S5P Family */
#define CONFIG_EXYNOS5			/* which is in a Exynos5 Family */

#include <asm/arch/cpu.h>		/* get chip and board defs */

#define CONFIG_SYS_GENERIC_BOARD
#define CONFIG_ARCH_CPU_INIT
#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO
#define CONFIG_BOARD_COMMON
#define CONFIG_ARCH_EARLY_INIT_R

#define CONFIG_BOOTSTAGE
#define CONFIG_PHYSMEM

/* TPM */
#define CONFIG_INFINEON_TPM_I2C
#define CONFIG_INFINEON_TPM_I2C_BUS 3
#define CONFIG_INFINEON_TPM_I2C_ADDR 0x20
#define CONFIG_CMD_TPM
#define CONFIG_TPM
#define CONFIG_TPM_TIS_I2C

/* Enable fdt support for Exynos5250 */
#define CONFIG_ARCH_DEVICE_TREE		exynos5
#define CONFIG_OF_CONTROL
#define CONFIG_OF_SEPARATE

/* Keep L2 Cache Disabled */
#define CONFIG_SYS_DCACHE_OFF

/* Enable ACE acceleration for SHA1 and SHA256 */
#define CONFIG_EXYNOS_ACE_SHA
#define CONFIG_SHA_HW_ACCEL

/* input clock of PLL: SMDK5250 has 24MHz input clock */
#define CONFIG_SYS_CLK_FREQ		24000000

#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_CMDLINE_TAG
#define CONFIG_INITRD_TAG
#define CONFIG_CMDLINE_EDITING

#define CONFIG_BOARD_REV_GPIO_COUNT	2

/* Power Down Modes */
#define S5P_CHECK_SLEEP			0x00000BAD
#define S5P_CHECK_DIDLE			0xBAD00000
#define S5P_CHECK_LPA			0xABAD0000

/* Offset for inform registers */
#define INFORM0_OFFSET			0x800
#define INFORM1_OFFSET			0x804
#define INFORM2_OFFSET			0x808
#define INFORM3_OFFSET			0x80c

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + (4 << 20))

#define CONFIG_BAUDRATE			115200
#define EXYNOS5_DEFAULT_UART_OFFSET	0x010000

/* Enable keyboard */
#define CONFIG_CROS_EC		/* CROS_EC protocol */
#define CONFIG_CROS_EC_SPI		/* Support CROS_EC over SPI */
#define CONFIG_CROS_EC_I2C		/* Support CROS_EC over I2C */
#define CONFIG_CROS_EC_KEYB	/* CROS_EC keyboard input */
#define CONFIG_CMD_CROS_EC
#define CONFIG_KEYBOARD

/* Console configuration */
#define CONFIG_CONSOLE_MUX
#define CONFIG_SYS_CONSOLE_IS_IN_ENV
#define EXYNOS_DEVICE_SETTINGS \
		"stdin=serial,cros-ec-keyb\0" \
		"stdout=serial,lcd\0" \
		"stderr=serial,lcd\0"

#define CONFIG_SILENT_CONSOLE

#define CONFIG_EXTRA_ENV_SETTINGS \
	EXYNOS_DEVICE_SETTINGS

#define TZPC_BASE_OFFSET		0x10000

/* SD/MMC configuration */
#define CONFIG_GENERIC_MMC
#define CONFIG_MMC
#define CONFIG_SDHCI
#define CONFIG_S5P_SDHCI
#define CONFIG_DWMMC
#define CONFIG_EXYNOS_DWMMC
#define CONFIG_SUPPORT_EMMC_BOOT


#define CONFIG_BOARD_EARLY_INIT_F

/* PWM */
#define CONFIG_PWM

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE

/* Command definition*/
#include <config_cmd_default.h>

#define CONFIG_CMD_PING
#define CONFIG_CMD_ELF
#define CONFIG_CMD_MMC
#define CONFIG_CMD_EXT2
#define CONFIG_CMD_FAT
#define CONFIG_CMD_NET
#define CONFIG_CMD_HASH
#define CONFIG_CMD_DHCP

#define CONFIG_BOOTDELAY		0
#define CONFIG_ZERO_BOOTDELAY_CHECK

/* Thermal Management Unit */
#define CONFIG_EXYNOS_TMU
#define CONFIG_CMD_DTT
#define CONFIG_TMU_CMD_DTT

/* USB */
#define CONFIG_CMD_USB
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_EXYNOS
#define CONFIG_SYS_USB_EHCI_MAX_ROOT_PORTS 3
#define CONFIG_USB_STORAGE

/* USB boot mode */
#define EXYNOS_COPY_USB_FNPTR_ADDR	0x02020070
#define EXYNOS_USB_SECONDARY_BOOT	0xfeed0002
#define EXYNOS_IRAM_SECONDARY_BASE	0x02020018

/* TPM */
#define CONFIG_TPM
#define CONFIG_CMD_TPM
#define CONFIG_INFINEON_TPM_I2C
#define CONFIG_INFINEON_TPM_I2C_BUS 3
#define CONFIG_INFINEON_TPM_I2C_ADDR 0x20

/* MMC SPL */
#define CONFIG_SPL
#define CONFIG_SPL_GPIO_SUPPORT

#define COPY_BL2_FNPTR_ADDR	0x02020030

/* specific .lds file */
#define CONFIG_SPL_LDSCRIPT	"board/samsung/common/exynos-uboot-spl.lds"
#define CONFIG_SPL_MAX_FOOTPRINT	(14 * 1024)

#define CONFIG_BOOTCOMMAND	"mmc read 40007000 451 2000; bootm 40007000"

/* Miscellaneous configurable options */
#define CONFIG_SYS_LONGHELP		/* undef to save memory */
#define CONFIG_SYS_HUSH_PARSER		/* use "hush" command parser	*/
#define CONFIG_SYS_CBSIZE		256	/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE		384	/* Print Buffer Size */
#define CONFIG_SYS_MAXARGS		16	/* max number of command args */
#define CONFIG_DEFAULT_CONSOLE		"console=ttySAC1,115200n8\0"
/* Boot Argument Buffer Size */
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE
/* memtest works on */
#define CONFIG_SYS_MEMTEST_START	CONFIG_SYS_SDRAM_BASE
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_SDRAM_BASE + 0x5E00000)
#define CONFIG_SYS_LOAD_ADDR		(CONFIG_SYS_SDRAM_BASE + 0x3E00000)

#define CONFIG_SYS_HZ			1000

#define CONFIG_RD_LVL

#define CONFIG_NR_DRAM_BANKS	8
#define SDRAM_BANK_SIZE		(256UL << 20UL)	/* 256 MB */
#define PHYS_SDRAM_1		CONFIG_SYS_SDRAM_BASE
#define PHYS_SDRAM_1_SIZE	SDRAM_BANK_SIZE
#define PHYS_SDRAM_2		(CONFIG_SYS_SDRAM_BASE + SDRAM_BANK_SIZE)
#define PHYS_SDRAM_2_SIZE	SDRAM_BANK_SIZE
#define PHYS_SDRAM_3		(CONFIG_SYS_SDRAM_BASE + (2 * SDRAM_BANK_SIZE))
#define PHYS_SDRAM_3_SIZE	SDRAM_BANK_SIZE
#define PHYS_SDRAM_4		(CONFIG_SYS_SDRAM_BASE + (3 * SDRAM_BANK_SIZE))
#define PHYS_SDRAM_4_SIZE	SDRAM_BANK_SIZE
#define PHYS_SDRAM_5		(CONFIG_SYS_SDRAM_BASE + (4 * SDRAM_BANK_SIZE))
#define PHYS_SDRAM_5_SIZE	SDRAM_BANK_SIZE
#define PHYS_SDRAM_6		(CONFIG_SYS_SDRAM_BASE + (5 * SDRAM_BANK_SIZE))
#define PHYS_SDRAM_6_SIZE	SDRAM_BANK_SIZE
#define PHYS_SDRAM_7		(CONFIG_SYS_SDRAM_BASE + (6 * SDRAM_BANK_SIZE))
#define PHYS_SDRAM_7_SIZE	SDRAM_BANK_SIZE
#define PHYS_SDRAM_8		(CONFIG_SYS_SDRAM_BASE + (7 * SDRAM_BANK_SIZE))
#define PHYS_SDRAM_8_SIZE	SDRAM_BANK_SIZE

#define CONFIG_SYS_MONITOR_BASE	0x00000000

/* FLASH and environment organization */
#define CONFIG_SYS_NO_FLASH
#undef CONFIG_CMD_IMLS

#define CONFIG_SYS_MMC_ENV_DEV		0

#define CONFIG_SECURE_BL1_ONLY

/* Secure FW size configuration */
#ifdef	CONFIG_SECURE_BL1_ONLY
#define	CONFIG_SEC_FW_SIZE		(8 << 10)	/* 8KB */
#else
#define	CONFIG_SEC_FW_SIZE		0
#endif

/* Configuration of BL1, BL2, ENV Blocks on mmc */
#define CONFIG_RES_BLOCK_SIZE	(512)
#define CONFIG_BL1_SIZE		(16 << 10) /*16 K reserved for BL1*/
#define	CONFIG_BL2_SIZE		(512UL << 10UL)	/* 512 KB */
#define CONFIG_ENV_SIZE		(16 << 10)	/* 16 KB */

#define CONFIG_BL1_OFFSET	(CONFIG_RES_BLOCK_SIZE + CONFIG_SEC_FW_SIZE)
#define CONFIG_BL2_OFFSET	(CONFIG_BL1_OFFSET + CONFIG_BL1_SIZE)
#define CONFIG_ENV_OFFSET	(0x00400000 - CONFIG_ENV_SIZE)

/* U-boot copy size from boot Media to DRAM.*/
#define BL2_START_OFFSET	(CONFIG_BL2_OFFSET/512)
#define BL2_SIZE_BLOC_COUNT	(CONFIG_BL2_SIZE/512)

#define OM_STAT				(0x1f << 1)
#define EXYNOS_COPY_SPI_FNPTR_ADDR	0x02020058
#define SPI_FLASH_UBOOT_POS		(CONFIG_SEC_FW_SIZE + CONFIG_BL1_SIZE)

#define CONFIG_DOS_PARTITION
#define CONFIG_EFI_PARTITION
#define CONFIG_CMD_PART
#define CONFIG_PARTITION_UUIDS

/*
 * Put the initial stack pointer 1KB below this to allow room for the
 * SPL marker. This value is arbitrary, but gd_t is placed starting here.
 */
#define CONFIG_SYS_INIT_SP_ADDR	(CONFIG_IRAM_TOP - 0x800)

/* The place where we put our SPL marker */
#define CONFIG_SPL_MARKER	(CONFIG_IRAM_TOP - 4)

/* I2C */
#define CONFIG_SYS_I2C_INIT_BOARD
#define CONFIG_HARD_I2C
#define CONFIG_CMD_I2C
#define CONFIG_SYS_I2C_SPEED	100000		/* 100 Kbps */
#define CONFIG_DRIVER_S3C24X0_I2C
#define CONFIG_I2C_MULTI_BUS
#define CONFIG_SYS_I2C_SLAVE    0x0
#define CONFIG_I2C_EDID

/* PMIC */
#define CONFIG_PMIC
#define CONFIG_PMIC_I2C
#define CONFIG_PMIC_MAX77686

/* SPI */
#define CONFIG_ENV_IS_IN_SPI_FLASH
#define CONFIG_SPI_FLASH

#ifdef CONFIG_SPI_FLASH
#define CONFIG_EXYNOS_SPI
#define CONFIG_CMD_SF
#define CONFIG_CMD_SPI
#define CONFIG_SPI_FLASH_WINBOND
#define CONFIG_SPI_FLASH_GIGADEVICE
#define CONFIG_SF_DEFAULT_MODE		SPI_MODE_0
#define CONFIG_SF_DEFAULT_SPEED		50000000
#define EXYNOS5_SPI_NUM_CONTROLLERS	5
#endif

#ifdef CONFIG_ENV_IS_IN_SPI_FLASH
#define CONFIG_ENV_SPI_MODE	SPI_MODE_0
#define CONFIG_ENV_SECT_SIZE	CONFIG_ENV_SIZE
#define CONFIG_ENV_SPI_BUS	1
#define CONFIG_ENV_SPI_MAX_HZ	50000000
#endif

/* PMIC */
#define CONFIG_POWER
#define CONFIG_POWER_I2C
#define CONFIG_POWER_MAX77686
#define CONFIG_POWER_S2MPS11

/* SPI */
#define CONFIG_ENV_IS_IN_SPI_FLASH
#define CONFIG_SPI_FLASH

#ifdef CONFIG_SPI_FLASH
#define CONFIG_EXYNOS_SPI
#define CONFIG_CMD_SF
#define CONFIG_CMD_SPI
#define CONFIG_SPI_FLASH_WINBOND
#define CONFIG_SF_DEFAULT_MODE		SPI_MODE_0
#define CONFIG_SF_DEFAULT_SPEED		50000000
#define EXYNOS5_SPI_NUM_CONTROLLERS	5
#endif

#ifdef CONFIG_ENV_IS_IN_SPI_FLASH
#define CONFIG_ENV_SPI_MODE	SPI_MODE_0
#define CONFIG_ENV_SECT_SIZE	CONFIG_ENV_SIZE
#define CONFIG_ENV_SPI_BUS	1
#define CONFIG_ENV_SPI_BASE	0x12D30000
#define CONFIG_ENV_SPI_MAX_HZ	50000000
#endif

/* Ethernet Controllor Driver */
#ifdef CONFIG_CMD_NET
#define CONFIG_SMC911X
#define CONFIG_SMC911X_BASE		0x5000000
#define CONFIG_SMC911X_16_BIT
#define CONFIG_ENV_SROM_BANK		1
#define CONFIG_USB_HOST_ETHER
#define CONFIG_USB_ETHER_ASIX
#define CONFIG_USB_ETHER_SMSC95XX
#endif /*CONFIG_CMD_NET*/

/* Enable PXE Support */
#ifdef CONFIG_CMD_NET
#define CONFIG_CMD_PXE
#define CONFIG_MENU
#endif

/* Sound */
#define CONFIG_CMD_SOUND
#ifdef CONFIG_CMD_SOUND
#define CONFIG_SOUND
#define CONFIG_I2S
#define CONFIG_SOUND_MAX98095
#define CONFIG_SOUND_WM8994
#endif

/* Enable devicetree support */
#define CONFIG_OF_LIBFDT

/* SHA hashing */
#define CONFIG_CMD_HASH
#define CONFIG_HASH_VERIFY
#define CONFIG_SHA1
#define CONFIG_SHA256

#define CONFIG_TPS65090_POWER

/* Display */
#define CONFIG_LCD
#ifdef CONFIG_LCD
#define CONFIG_EXYNOS_FB
#define CONFIG_EXYNOS_DP
#define LCD_XRES			2560
#define LCD_YRES			1600
#define LCD_BPP			LCD_COLOR16
#define CONFIG_CMD_BMP
#endif

/* Enable Time Command */
#define CONFIG_CMD_TIME

/* Enable booting of fitImage format */
#define CONFIG_FIT
#define CONFIG_FIT_BEST_MATCH

/* We might use LZO compression */
#define CONFIG_LZO

#endif	/* __CONFIG_EXYNOS5_DT_H */