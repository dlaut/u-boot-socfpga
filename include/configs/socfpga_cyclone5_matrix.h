/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2014 Marek Vasut <marex@denx.de>
 */
#ifndef __CONFIG_SOCFPGA_CYCLONE5_H__
#define __CONFIG_SOCFPGA_CYCLONE5_H__

#include <asm/arch/base_addr_ac5.h>

/* Memory configurations */
#define PHYS_SDRAM_1_SIZE		0x40000000	/* 1GiB on SoCDK */

/* Booting Linux */
#define CONFIG_LOADADDR		0x01000000
#define CONFIG_SYS_LOAD_ADDR	CONFIG_LOADADDR

/* QSPI boot */
#define FDT_SIZE		__stringify(0x00010000)
#define KERNEL_SIZE		__stringify(0x005d0000)
#define QSPI_FDT_ADDR		__stringify(0x00220000)
#define QSPI_KERNEL_ADDR	__stringify(0x00230000)

#define SOCFPGA_BOOT_SETTINGS \
	"fdt_size=" FDT_SIZE "\0" \
	"kernel_size=" KERNEL_SIZE "\0" \
	"qspi_fdt_addr=" QSPI_FDT_ADDR "\0" \
	"qspi_kernel_addr=" QSPI_KERNEL_ADDR "\0" \
	"qspiboot=setenv bootargs earlycon " \
		"root=/dev/mtdblock1 rw rootfstype=jffs2; " \
		"bootz ${kernel_addr_r} - ${fdt_addr_r}\0" \
	"qspiload=sf probe; " \
		"sf read ${kernel_addr_r} ${qspi_kernel_addr} ${kernel_size}; " \
		"sf read ${fdt_addr_r} ${qspi_fdt_addr} ${fdt_size}\0"

// TEMP
#define DEVELOPMENT_MODE 1
#define DEVMODE_SERIAL            "eserial0"    // Aux
#define BUTTON_ACTIVE_LEVEL       "high"
// -----

#define DEVMODE_USER_RUN_LEVEL    "4"
#define DEVMODE_BOOTARGS_DEBUG    "debug earlyprintk=serial,ttyS0," __stringify(CONFIG_BAUDRATE) " console=ttyS0," __stringify(CONFIG_BAUDRATE) " " DEVMODE_USER_RUN_LEVEL
#define DEVMODE_DEBUG_SERIAL      DEVMODE_SERIAL
#define PRDMODE_USER_RUN_LEVEL    "5"
#define PRDMODE_BOOTARGS_DEBUG    PRDMODE_USER_RUN_LEVEL
#define PRDMODE_DEBUG_SERIAL      "nulldev"

#ifdef DEVELOPMENT_MODE
	#warning "!!!Development mode enabled!!!"
	#define DEFAULT_USER_RUN_LEVEL    DEVMODE_USER_RUN_LEVEL
	#define DEFAULT_BOOTARGS_DEBUG    DEVMODE_BOOTARGS_DEBUG
	#define DEFAULT_DEBUG_SERIAL      DEVMODE_DEBUG_SERIAL
#else
	#define DEFAULT_USER_RUN_LEVEL    PRDMODE_USER_RUN_LEVEL
	#define DEFAULT_BOOTARGS_DEBUG    PRDMODE_BOOTARGS_DEBUG
	#define DEFAULT_DEBUG_SERIAL      PRDMODE_DEBUG_SERIAL
	#define CONFIG_SILENT_CONSOLE     1
#endif

#define CONFIG_BOOTARGS " " DEFAULT_BOOTARGS_DEBUG " "

#define CONFIG_EXTRA_ENV_SETTINGS \
	"dlaboot=" \
		"run checkmac; " \
		"setenv boot_part boot; " \
		"setenv boot_path Current; " \
		"setenv boot_target default; " \
		"run fpgaload; " \
		"run checkbutton; " \
		"run checkmotorrurnin; " \
		"run checkpts; " \
		"run checkupdate; " \
		"run mmcload; " \
		"dlablinkall; " \
		"run mmcboot; " \
		"run recoverdevice\0" \
	"userrunlevel=" DEFAULT_USER_RUN_LEVEL "\0" \
	"bootargs_debug=" DEFAULT_BOOTARGS_DEBUG "\0" \
	"ipaddr=192.168.3.100\0" \
	"serverip=192.168.3.101\0" \
	"userrestoredefault=false\0" \
	"userbootloader=false\0" \
	"buttonactlevel=" BUTTON_ACTIVE_LEVEL "\0" \
	"bootm_size=0xa000000\0" \
	"kernel_addr_r="__stringify(CONFIG_SYS_LOAD_ADDR)"\0" \
	"fdt_addr_r=0x02000000\0" \
	"ramdisk_addr_r=0x02300000\0" \
	"ethaddr=AA:BB:CC:DD:EE:FF\0" \
	"filekernel=zImage\0" \
	"filedtb=socfpga.dtb\0" \
	"filerootfs=rootfs.cpio.lz4.u-boot\0" \
	"filenetwork=Config/cfg/network.ini\0" \
	"initrd_high=0x08000000\0" \
	"mmcloadcmd=ext4load\0" \
	"mmcloadp=1\0" \
	"mmcbootp=1\0" \
	"mmcrecp=5\0" \
	"mmcuserp=6\0" \
	"mmcload=mmc rescan;" \
		"${mmcloadcmd} mmc 0:${mmcloadp} ${kernel_addr_r} ${boot_path}/${filekernel};" \
		"${mmcloadcmd} mmc 0:${mmcloadp} ${fdt_addr_r} ${boot_path}/${filedtb};" \
		"${mmcloadcmd} mmc 0:${mmcloadp} ${ramdisk_addr_r} ${boot_path}/${filerootfs};\0" \
	"mmcboot=" \
		"run fdt_append_bootinfo; " \
		"setenv bootargs ${bootargs_debug} rootfstype=ramfs root=/dev/ram0;" \
		"bootz ${kernel_addr_r} ${ramdisk_addr_r} ${fdt_addr_r};\0" \
	"netload=" \
		"tftp ${fdt_addr_r} ${boot_path}/${filedtb};" \
		"tftp ${kernel_addr_r} ${boot_path}/${filekernel};" \
		"tftp ${ramdisk_addr_r} ${boot_path}/${filerootfs};\0" \
	"checkmac=" \
		"echo Reading MAC;" \
		"${mmcloadcmd} mmc 0:${mmcrecp} ${loadaddr} ${filenetwork};" \
		"dlamac mmc 0:${mmcrecp} ${filenetwork} ${filesize};\0" \
	"checkmotorrurnin=" \
			"if ${mmcloadcmd} mmc 0:${mmcuserp} ${loadaddr} AppData/RunIn.ini; then " \
				"echo Boot PTS from flash with RurnIn test;" \
				"setenv boot_part recovery; " \
				"setenv boot_path PTS;" \
				"setenv boot_target pts; " \
				"setenv mmcloadp ${mmcrecp};" \
					"run fpgaload;" \
					"run mmcload;" \
					"run mmcboot;" \
					"echo Boot from PTS failed!;" \
					"dlaledset 16 1;" \
					"setenv boot_part boot; " \
					"setenv boot_path Current;" \
					"setenv boot_target default; " \
					"setenv mmcloadp ${mmcbootp};" \
					"run fpgaload;" \
			"fi\0" \
	"checkpts=" \
		"if ${mmcloadcmd} mmc 0:${mmcuserp} ${loadaddr} AppData/BootInfo.txt; then " \
			"dlaledset 24 1;" \
			"echo Boot PTS;" \
			"setenv boot_part net; " \
			"setenv boot_path matrix-soc-pts;" \
			"setenv boot_target pts; " \
			"setenv netretry no;" \
			"run fpganetload;" \
			"run netload;" \
			"dlaledset 24 0;" \
			"run mmcboot;" \
			"dlaledset 16 1;" \
			"setenv boot_part boot; " \
			"setenv boot_path Current;" \
			"setenv boot_target default; " \
			"run fpgaload;" \
		"fi\0" \
	"checkbutton=" \
		"dlabuttonmng;" \
		"if ${userrestoredefault}; then " \
			"echo Restore defaults;" \
			"ext4write mmc 0:${mmcuserp} /RestoreDefaults.txt $loadaddr 0x32;" \
			"setenv userrestoredefault false;" \
			"dlabeep;" \
		"fi;" \
		"if ${userbootloader}; then " \
			"if dlals mmc 0:${mmcbootp} /Loader; then " \
				"echo Boot Loader;" \
				"setenv boot_part boot; " \
				"setenv boot_path Loader;" \
				"setenv boot_target recovery; " \
				"setenv mmcloadp ${mmcbootp};" \
				"run fpgaload;" \
				"run mmcload;" \
				"run mmcboot;" \
				"setenv boot_part boot; " \
				"setenv boot_path Current;" \
				"setenv boot_target default; " \
				"run fpgaload;" \
			"fi;" \
			"echo Loader folder not present: boot from Current;" \
			"setenv mmcloadp ${mmcbootp};" \
			"setenv userbootloader false;" \
		"fi\0" \
	"checkupdate=" \
		"if ${mmcloadcmd} mmc 0:${mmcuserp} ${loadaddr} /UpdateBoot.txt; then " \
			"echo Booting update flag is set!;" \
		"else " \
			"if ${mmcloadcmd} mmc 0:${mmcloadp} ${loadaddr} /Update/Update.txt; then " \
				"dlaledset 24 1;" \
				"setenv boot_part boot; " \
				"setenv boot_path Update;" \
				"setenv boot_target default; " \
				"run fpgaload;" \
				"run mmcload;" \
				"dlaledset 18 1;" \
				"ext4write mmc 0:${mmcuserp} /UpdateBoot.txt $loadaddr 0x32;" \
				"dlaledset 24 0;" \
				"dlaledset 18 0;" \
				"dlablinkall;run mmcboot;" \
				"echo Update operation failed!;" \
				"dlaledset 16 1;" \
				"ext4write mmc 0:${mmcuserp} /UpdateFail.txt $loadaddr 0x32;" \
				"setenv boot_part boot; " \
				"setenv boot_path Current;" \
				"setenv boot_target default; " \
				"run fpgaload;" \
			"fi;" \
		"fi\0" \
	"fpga=0\0" \
	"fpgadata=0x2000000\0" \
	"fpgadatasize=0x700000\0" \
	"fpgaimage=soc_system.rbf\0" \
	"fpgaload=if ${mmcloadcmd} mmc 0:${mmcloadp} $fpgadata ${boot_path}/$fpgaimage;" \
		"then " \
			"fpga load 0 $fpgadata $filesize;" \
			"bridge enable; " \
		"else " \
			"echo FPGA binary not loaded; " \
		"fi;\0" \
	"fpganetload=" \
		"tftp ${fpgadata} ${boot_path}/${fpgaimage};"\
		"fpga load 0 ${fpgadata} ${filesize};" \
		"bridge enable;\0" \
	"updateinprogress=false\0" \
	"userbootloader=false\0" \
	"userrestoredefault=false\0" \
	"recoverdevice=" \
		"echo Boot from Current failed!;" \
		"setenv boot_part boot; " \
		"setenv boot_path Rollback;" \
		"setenv boot_target default; " \
		"setenv mmcloadp ${mmcbootp};" \
		"run fpgaload;" \
		"run mmcload;" \
		"run mmcboot;" \
		"echo Boot from Rollback failed!;" \
		"setenv boot_part recovery; " \
		"setenv boot_path Factory;" \
		"setenv boot_target default; " \
		"setenv mmcloadp ${mmcrecp};" \
		"run fpgaload;" \
		"run mmcload;" \
		"run mmcboot;" \
		"echo Boot from Factory failed!;" \
		"dlaledset 24 1;" \
		"setenv boot_part net; " \
		"setenv boot_path matrix-soc-recovery;" \
		"setenv boot_target default; " \
		"setenv netretry no;" \
		"run fpganetload;" \
		"run netload;" \
		"dlaledset 24 0;" \
		"run mmcboot;" \
		"echo Boot from network failed! Nothing more to do...;" \
		"dlaledset 16 1;" \
		"\0" \
	"fdt_append_bootinfo=" \
		"fdt addr ${fdt_addr_r}; " \
		"fdt resize; " \
		"fdt mknode / chosen; " \
		"fdt mknode /chosen bootinfo; " \
		"fdt set /chosen/bootinfo boot_part ${boot_part}; " \
		"fdt set /chosen/bootinfo boot_path ${boot_path}; " \
		"fdt set /chosen/bootinfo boot_target ${boot_target}; " \
		"\0"

/* The rest of the configuration is shared */
#include <configs/socfpga_common.h>

#endif	/* __CONFIG_SOCFPGA_CYCLONE5_H__ */
