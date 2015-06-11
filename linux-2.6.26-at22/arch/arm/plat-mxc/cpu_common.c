/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/init.h>
#include <linux/module.h>
#include <asm/setup.h>
extern int mxc_early_serial_console_init(char *);

static int cpu_rev = 0;

/*!
 * @file plat-mxc/cpu_common.c
 *
 * @brief This file contains the common CPU initialization code.
 *
 * @ingroup MSL_MX31
 */

static void __init cpu_rev_setup(char **p)
{
	cpu_rev = simple_strtoul(*p, NULL, 16);
}

__early_param("cpu_rev=", cpu_rev_setup);

#define mxc_cpu_rev()		(cpu_rev & 0xFF)
#define mxc_cpu_rev_major()	((cpu_rev >> 4) & 0xF)
#define mxc_cpu_rev_minor()	(cpu_rev & 0xF)
#define mxc_cpu_is_rev(rev) \
	(mxc_cpu_rev() == rev) ? 1 : ((mxc_cpu_rev() < rev) ? -1 : 2)

int mxc_cpu_type(void)
{
	return (cpu_rev >> 12);
}

void mxc_set_cpu_rev(unsigned int part, unsigned int rev)
{
	cpu_rev = (part << 12) | rev;
}

int mxc_is_cpu(int part)
{
	return (mxc_cpu_type() == part) ? 1 : 0;
}
EXPORT_SYMBOL(mxc_is_cpu);

/*
 * Create functions to test for cpu revision
 *
 * Returns:
 *	 0 - not the cpu queried
 *	 1 - cpu and revision match
 *	 2 - cpu matches, but cpu revision is greater than queried rev
 *	-1 - cpu matches, but cpu revision is less than queried rev
 */
#define DEFINE_CPU_IS_REV(type)                                 \
int cpu_is_##type##_rev (unsigned int rev)                   \
{                                                            \
       return (cpu_is_##type() ? mxc_cpu_is_rev(rev) : 0); \
}

DEFINE_CPU_IS_REV(mx27)
DEFINE_CPU_IS_REV(mx31)
DEFINE_CPU_IS_REV(mx32)
DEFINE_CPU_IS_REV(mx35)
DEFINE_CPU_IS_REV(mx51)
DEFINE_CPU_IS_REV(mx53)

int mxc_jtag_type = 0; /* OFF: 0 (default), JTAG: 1, ETM8: 2, ETM16: 3 */
EXPORT_SYMBOL(mxc_jtag_type);

/*
 * Here are the JTAG options from the command line. By default JTAG
 * is OFF which means JTAG is not connected and WFI is enabled
 *
 *       "on"     JTAG is connected, so WFI is disabled
 *       "off"    JTAG is disconnected, so WFI is enabled
 *       "etm8"   ETM(8bits) is connected, so WFI is disabled
 *       "etm16"  ETM(16bits) is connected, so WFI is disabled
 */

static void __init jtag_wfi_setup(char **p)
{
	if (memcmp(*p, "on", 2) == 0) {
		mxc_jtag_type = 1;
		*p += 2;
	} else if (memcmp(*p, "off", 3) == 0) {
		mxc_jtag_type = 0;
		*p += 3;
	} else if (memcmp(*p, "etm8", 4) == 0) {
		mxc_jtag_type = 2;
		*p += 4;
	} else if (memcmp(*p, "etm16", 5) == 0) {
		mxc_jtag_type = 3;
		*p += 5;
	}
}

__early_param("jtag=", jtag_wfi_setup);

void __init mxc_cpu_common_init(void)
{
	pr_info("CPU is %s%x Revision %u.%u\n",
		(mxc_cpu_type() < 0x100) ? "i.MX" : "MXC",
		mxc_cpu_type(), mxc_cpu_rev_major(), mxc_cpu_rev_minor());
}

/**
 * early_console_setup - setup debugging console
 *
 * Consoles started here require little enough setup that we can start using
 * them very early in the boot process, either right after the machine
 * vector initialization, or even before if the drivers can detect their hw.
 *
 * Returns non-zero if a console couldn't be setup.
 * This function is developed based on 
 * early_console_setup function as defined in arch/ia64/kernel/setup.c
 */
int __init early_console_setup(char *cmdline)
{
	int earlycons = 0;

#ifdef CONFIG_SERIAL_MXC_CONSOLE
	if (!mxc_early_serial_console_init(cmdline))
		earlycons++;
#endif

	return (earlycons) ? 0 : -1;
}
