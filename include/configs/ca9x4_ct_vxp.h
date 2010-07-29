/*
 * (C) Copyright 2010 Linaro
 * Matt Waddel, <matt.waddel@linaro.org>
 *
 * Configuration for Versatile Express. Parts were derived from other ARM
 *   configurations.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/* Board info register */
#define SYS_ID				0x10000000
#define CONFIG_REVISION_TAG		1

/* High Level Configuration Options */
#define CONFIG_ARMV7			1

#define CONFIG_SYS_MEMTEST_START	0x60000000
#define CONFIG_SYS_MEMTEST_END		0x40000000
#define CFG_HZ				1000
#define CONFIG_SYS_HZ			CFG_HZ

#define CONFIG_CMDLINE_TAG		1	/* enable passing of ATAGs */
#define CONFIG_SETUP_MEMORY_TAGS	1
#define CONFIG_MISC_INIT_R		1
#define CONFIG_L2_OFF			1

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + 128 * 1024)
#define CONFIG_SYS_GBL_DATA_SIZE	128

#define SCTL_BASE			0x10001000
#define VEXPRESS_SYS_FLASH_OFF		0x4C
#define VEXPRESS_FLASHCTRL		(SCTL_BASE + VEXPRESS_SYS_FLASH_OFF)
#define VEXPRESS_FLASHPROG_FLVPPEN	(1 << 0)

/* SMSC9115 Ethernet from SMSC9118 family */
#define CONFIG_NET_MULTI
#define CONFIG_SMC911X			1
#define CONFIG_SMC911X_32_BIT		1
#define CONFIG_SMC911X_BASE		0x4E000000

/* PL011 Serial Configuration */
#define CONFIG_PL011_SERIAL
#define CONFIG_PL011_CLOCK		24000000
#define CONFIG_PL01x_PORTS		{(void *)CONFIG_SYS_SERIAL0, \
					 (void *)CONFIG_SYS_SERIAL1}
#define CONFIG_CONS_INDEX		0

#define CONFIG_BAUDRATE			38400
#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }
#define CONFIG_SYS_SERIAL0		0x10009000
#define CONFIG_SYS_SERIAL1		0x1000A000

/* Command line configuration */
#define CONFIG_CMD_BDI
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_ELF
#define CONFIG_CMD_ENV
#define CONFIG_CMD_FLASH
#define CONFIG_CMD_IMI
#define CONFIG_CMD_MEMORY
#define CONFIG_CMD_NET
#define CONFIG_CMD_PING
#define CONFIG_CMD_SAVEENV
#define CONFIG_NET_MULTI

#define CONFIG_CMD_FAT
#define CONFIG_DOS_PARTITION		1
#define CONFIG_MMC			1
#define CONFIG_CMD_MMC
#define CONFIG_GENERIC_MMC

/* BOOTP options */
#define CONFIG_BOOTP_BOOTFILESIZE
#define CONFIG_BOOTP_BOOTPATH
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_HOSTNAME

/* Miscellaneous configurable options */
#undef	CONFIG_SYS_CLKS_IN_HZ
#define CONFIG_SYS_LOAD_ADDR		0x60008000	/* load address */
#define LINUX_BOOT_PARAM_ADDR		0x60000100
#define CONFIG_BOOTDELAY		2

/* Stack sizes are set up in start.S using the settings below */
#define CONFIG_STACKSIZE		(128 * 1024)	/* regular stack */
#ifdef CONFIG_USE_IRQ
#define CONFIG_STACKSIZE_IRQ		(4 * 1024)	/* IRQ stack */
#define CONFIG_STACKSIZE_FIQ		(4 * 1024)	/* FIQ stack */
#endif

/* Physical Memory Map */
#define CONFIG_NR_DRAM_BANKS		2
#define PHYS_SDRAM_1			0x60000000	/* SDRAM Bank #1 */
#define PHYS_SDRAM_2			0x80000000	/* SDRAM Bank #1 */
#define PHYS_SDRAM_1_SIZE		0x20000000	/* 512 MB */
#define PHYS_SDRAM_2_SIZE		0x20000000	/* 512 MB */
#define PHYS_FLASH_SIZE			0x04000000	/* 64MB */

/* Sample environment settings */
#define CONFIG_BOOTCOMMAND		"bootm"
#define CONFIG_BOOTARGS			"root=/dev/nfs ip=dhcp mem=1024M " \
					"console=ttyAMA0 video=vc:1-2clcdfb: "\
					"nfsroot=10.1.77.36:/work/exports/share"
#define	CONFIG_EXTRA_ENV_SETTINGS	"ethaddr=00:02:F7:00:19:17\0"	\
					"ipaddr=10.1.77.77\0"		\
					"gatewayip=10.1.77.1\0"		\
					"serverip=10.1.77.36\0"
#define CONFIG_BOOTFILE			"/work/exports/vexpress"

/* FLASH and environment organization */
#define CONFIG_SYS_FLASH_CFI		1
#define CONFIG_FLASH_CFI_DRIVER		1
#define CONFIG_SYS_FLASH_SIZE		0x04000000
#define CONFIG_SYS_MAX_FLASH_BANKS	1
#define CONFIG_SYS_FLASH_BASE		0x40000000

/* Timeout values in ticks */
#define CONFIG_SYS_FLASH_ERASE_TOUT	(2 * CFG_HZ)	/* Erase Timeout */
#define CONFIG_SYS_FLASH_WRITE_TOUT	(2 * CFG_HZ)	/* Write Timeout */

/* 255 0x40000 sectors + first or last sector may have 4 erase regions = 259 */
#define CONFIG_SYS_MAX_FLASH_SECT	259		/* Max sectors */
#define FLASH_MAX_SECTOR_SIZE		0x00040000	/* 256 KB sectors */
#define FLASH_MIN_SECTOR_SIZE		0x00010000	/*  64 KB sectors */

/* Room required on the stack for the environment data */
#define CONFIG_ENV_SIZE			8192

/*
 * Amount of flash used for environment:
 * Since we don't know which end has the small erase blocks
 * use the penultimate full sector location
 * for the environment - save a full sector even though
 * the real environment size CONFIG_ENV_SIZE is probably less
 */
#define CONFIG_ENV_SECT_SIZE		FLASH_MAX_SECTOR_SIZE
#define CONFIG_ENV_OVERWRITE		1

/* Store environment at top of flash */
#define CONFIG_ENV_IS_IN_FLASH		1
#define CONFIG_ENV_OFFSET		(PHYS_FLASH_SIZE - \
					(2 * CONFIG_ENV_SECT_SIZE))
#define CONFIG_ENV_ADDR			(CONFIG_SYS_FLASH_BASE + \
					 CONFIG_ENV_OFFSET)
#define CONFIG_SYS_FLASH_PROTECTION	/* The devices have real protection */
#define CONFIG_SYS_FLASH_EMPTY_INFO	/* flinfo indicates empty blocks */
#define CONFIG_SYS_FLASH_BANKS_LIST	{ CONFIG_SYS_FLASH_BASE }

/* Monitor Command Prompt */
#define CONFIG_SYS_CBSIZE		512	/* Console I/O Buffer Size */
#define CONFIG_SYS_PROMPT		"VExpress# "
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + \
					sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE /* Boot args buffer */
#define CONFIG_CMD_SOURCE
#define CONFIG_SYS_LONGHELP
#define CONFIG_CMDLINE_EDITING		1
#define CONFIG_SYS_MAXARGS		16	/* max command args */
#endif
