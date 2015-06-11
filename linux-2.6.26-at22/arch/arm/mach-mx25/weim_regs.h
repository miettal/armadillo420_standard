/*
 * Copyright 2011 Atmark Techno, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#ifndef __ARCH_ARM_MACH_MX25_WEIM_REGS_H__
#define __ARCH_ARM_MACH_MX25_WEIM_REGS_H__

#include <asm/arch/hardware.h>

#define CSCR_U(n)	(IO_ADDRESS(WEIM_BASE_ADDR + (n) * 0x10))
#define CSCR_L(n)	(IO_ADDRESS(WEIM_BASE_ADDR + (n) * 0x10 + 0x4))
#define CSCR_A(n)	(IO_ADDRESS(WEIM_BASE_ADDR + (n) * 0x10 + 0x8))
#define WCR		(IO_ADDRESS(WEIM_BASE_ADDR + 0x60))
#define WCR_AUS(n)	(1 << (8 + (n)))
#endif
