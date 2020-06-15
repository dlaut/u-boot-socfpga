/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2020 Intel Corporation <www.intel.com>
 *
 */

#ifndef _HANDOFF_SOC64_H_
#define _HANDOFF_SOC64_H_
/*
 * Offset for HW handoff from Quartus tools
 */
/* HPS handoff */
#define HANDOFF_MAGIC_BOOT			0x424F4F54
#define HANDOFF_MAGIC_MUX			0x504D5558
#define HANDOFF_MAGIC_IOCTL			0x494F4354
#define HANDOFF_MAGIC_FPGA			0x46504741
#define HANDOFF_MAGIC_DELAY			0x444C4159
#define HANDOFF_MAGIC_CLOCK			0x434C4B53
#define HANDOFF_MAGIC_MISC			0x4D495343

#define HANDOFF_OFFSET_LENGTH			0x4
#define HANDOFF_OFFSET_DATA			0x10
#define HANDOFF_SIZE				4096
#define S10_HANDOFF_SIZE			4096

#define DDR_HANDOFF_BASE			0xFFE5C000

#if defined(CONFIG_TARGET_SOCFPGA_STRATIX10) || \
	defined(CONFIG_TARGET_SOCFPGA_AGILEX)
#define HANDOFF_BASE				0xFFE3F000
#define HANDOFF_MISC				(HANDOFF_BASE + 0x610)
#elif defined(CONFIG_TARGET_SOCFPGA_DM)
#define HANDOFF_BASE				0xFFE5F000
#define HANDOFF_MISC				(HANDOFF_BASE + 0x630)

/* DDR handoff */
#define DDR_HANDOFF_MAGIC			0x48524444
#define DDR_HANDOFF_UMCTL2_MAGIC		0x4C54434D
#define DDR_HANDOFF_UMCTL2_SECTION		(DDR_HANDOFF_BASE + 0x10)
#define DDR_HANDOFF_UMCTL2_BASE			(DDR_HANDOFF_BASE + 0x1C)
#define DDR_HANDOFF_PHY_MAGIC			0x43594850
#define DDR_HANDOFF_PHY_INIT_ENGINE_MAGIC	0x45594850
#define DDR_HANDOFF_PHY_BASE_OFFSET		0x8
#endif

#define HANDOFF_MUX				(HANDOFF_BASE + 0x10)
#define HANDOFF_IOCTL				(HANDOFF_BASE + 0x1A0)
#define HANDOFF_FPGA				(HANDOFF_BASE + 0x330)
#define HANODFF_DELAY				(HANDOFF_BASE + 0x3F0)
#define HANDOFF_CLOCK				(HANDOFF_BASE + 0x580)

#ifdef CONFIG_TARGET_SOCFPGA_STRATIX10
#define HANDOFF_CLOCK_OSC			(HANDOFF_BASE + 0x608)
#define HANDOFF_CLOCK_FPGA			(HANDOFF_BASE + 0x60C)
#else
#define HANDOFF_CLOCK_OSC			(HANDOFF_BASE + 0x5fc)
#define HANDOFF_CLOCK_FPGA			(HANDOFF_BASE + 0x600)
#endif

#if defined(CONFIG_TARGET_SOCFPGA_STRATIX10) || \
	defined(CONFIG_TARGET_SOCFPGA_AGILEX) || \
	defined(CONFIG_TARGET_SOCFPGA_DM)
#define HANDOFF_MUX_LEN				96
#define HANDOFF_IOCTL_LEN			96
#define HANDOFF_FPGA_LEN			40
#define HANODFF_DELAY_LEN			96
#endif

#ifndef __ASSEMBLY__
#include <asm/types.h>
enum endianness {
	little_endian,
	big_endian
};

int get_handoff_size(void *handoff_address, enum endianness endian);
int handoff_read(void *handoff_address, void *table, u32 table_len,
		 enum endianness big_endian);
int sysmgr_pinmux_table(void *handoff_address, void *table, u32 table_len);
#endif
#endif /* _HANDOFF_SOC64_H_ */
