/*
 * Copyright 2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/mm.h>
#include <linux/init.h>
#include <asm/hardware.h>
#include <asm/pgtable.h>
#include <asm/mach/map.h>
#include <asm/arch/iomux-v3.h>
#include <asm/mach-types.h>

/*!
 * @file mach-mx25/mm.c
 *
 * @brief This file creates static mapping between physical to virtual memory.
 *
 * @ingroup Memory_MX25
 */

/*!
 * This structure defines the MX25 memory map.
 */
static struct map_desc mxc_io_desc[] __initdata = {
	{
		.virtual = IRAM_BASE_ADDR_VIRT,
		.pfn = __phys_to_pfn(IRAM_BASE_ADDR),
		.length = IRAM_SIZE,
		.type = MT_NONSHARED_DEVICE
	},
	{
		.virtual = X_MEMC_BASE_ADDR_VIRT,
		.pfn = __phys_to_pfn(X_MEMC_BASE_ADDR),
		.length = X_MEMC_SIZE,
		.type = MT_DEVICE
	},
	{
		.virtual = NFC_BASE_ADDR_VIRT,
		.pfn = __phys_to_pfn(NFC_BASE_ADDR),
		.length = NFC_SIZE,
		.type = MT_NONSHARED_DEVICE
	},
	{
		.virtual = ROMP_BASE_ADDR_VIRT,
		.pfn = __phys_to_pfn(ROMP_BASE_ADDR),
		.length = ROMP_SIZE,
		.type = MT_NONSHARED_DEVICE
	},
	{
		.virtual = ASIC_BASE_ADDR_VIRT,
		.pfn = __phys_to_pfn(ASIC_BASE_ADDR),
		.length = ASIC_SIZE,
		.type = MT_NONSHARED_DEVICE
	},
	{
		.virtual = AIPS1_BASE_ADDR_VIRT,
		.pfn = __phys_to_pfn(AIPS1_BASE_ADDR),
		.length = AIPS1_SIZE,
		.type = MT_NONSHARED_DEVICE
	},
	{
		.virtual = SPBA0_BASE_ADDR_VIRT,
		.pfn = __phys_to_pfn(SPBA0_BASE_ADDR),
		.length = SPBA0_SIZE,
		.type = MT_NONSHARED_DEVICE
	},
	{
		.virtual = AIPS2_BASE_ADDR_VIRT,
		.pfn = __phys_to_pfn(AIPS2_BASE_ADDR),
		.length = AIPS2_SIZE,
		.type = MT_NONSHARED_DEVICE
	},
	{
		.virtual = CS0_BASE_ADDR_VIRT,
		.pfn = __phys_to_pfn(CS0_BASE_ADDR),
		.length = CS0_SIZE,
		.type = MT_DEVICE
	},
};

#if defined(CONFIG_MACH_ARMADILLO460)
static struct map_desc armadillo460_io_desc[] __initdata = {
	{
		.virtual = CS1_BASE_ADDR_VIRT,
		.pfn = __phys_to_pfn(CS1_BASE_ADDR),
		.length = CS1_SIZE,
		.type = MT_NONSHARED_DEVICE
	},
	{
		.virtual = CS3_BASE_ADDR_VIRT,
		.pfn = __phys_to_pfn(CS3_BASE_ADDR),
		.length = CS3_SIZE,
		.type = MT_NONSHARED_DEVICE
	},
	{
		.virtual = CS4_BASE_ADDR_VIRT,
		.pfn = __phys_to_pfn(CS4_BASE_ADDR),
		.length = CS4_SIZE,
		.type = MT_NONSHARED_DEVICE
	},
};
#endif

/*!
 * This function initializes the memory map. It is called during the
 * system startup to create static physical to virtual memory map for
 * the IO modules.
 */
void __init mxc_map_io(void)
{
	mxc_iomux_v3_init((void *)IO_ADDRESS(IOMUXC_BASE_ADDR));

	iotable_init(mxc_io_desc, ARRAY_SIZE(mxc_io_desc));
#if defined(CONFIG_MACH_ARMADILLO460)
	if (machine_is_armadillo460())
		iotable_init(armadillo460_io_desc,
			     ARRAY_SIZE(armadillo460_io_desc));
#endif
}
