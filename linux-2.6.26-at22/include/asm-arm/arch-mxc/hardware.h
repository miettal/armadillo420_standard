/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_MXC_HARDWARE_H__
#define __ASM_ARCH_MXC_HARDWARE_H__

#include <asm/sizes.h>

#ifdef CONFIG_ARCH_MX3
#include <asm/arch/mx31.h>
#endif

#ifdef CONFIG_ARCH_MX25
#include <asm/arch/mx25.h>
#endif

#ifdef CONFIG_ARCH_MX27
#include <asm/arch/mx27.h>
#endif

/* defines PCIO_BASE (not used but needed for compilation) */
#define PCIO_BASE		0

/* This macro is used to get certain bit field from a number */
#define MXC_GET_FIELD(val, len, sh)          ((val >> sh) & ((1 << len) - 1))

/* This macro is used to set certain bit field inside a number */
#define MXC_SET_FIELD(val, len, sh, nval)    ((val & ~(((1 << len) - 1) << sh)) | nval << sh)

/*****************************************************************************
 * Processor specific defines
 *****************************************************************************/
#define CHIP_REV_1_0		0x10
#define CHIP_REV_1_1		0x11
#define CHIP_REV_1_2		0x12
#define CHIP_REV_1_3		0x13
#define CHIP_REV_2_0		0x20
#define CHIP_REV_2_1		0x21
#define CHIP_REV_2_2		0x22
#define CHIP_REV_2_3		0x23
#define CHIP_REV_3_0		0x30
#define CHIP_REV_3_1		0x31
#define CHIP_REV_3_2		0x32

#ifndef __ASSEMBLY__

#ifdef CONFIG_ARCH_MX3
#define cpu_is_mx31()		(mxc_is_cpu(0x31))
#define cpu_is_mx32()		(mxc_is_cpu(0x32))
#else
#define cpu_is_mx31()		(0)
#define cpu_is_mx32()		(0)
#endif

#ifdef CONFIG_ARCH_MX21
#define cpu_is_mx21()		(1)
#else
#define cpu_is_mx21()		(0)
#endif

#ifdef CONFIG_ARCH_MX25
#define cpu_is_mx25()		(1)
#else
#define cpu_is_mx25()		(0)
#endif

#ifdef CONFIG_ARCH_MX27
#define cpu_is_mx27()		(1)
#else
#define cpu_is_mx27()		(0)
#endif

#ifdef CONFIG_ARCH_MX35
#define cpu_is_mx35()		(1)
#else
#define cpu_is_mx35()		(0)
#endif

#ifdef CONFIG_ARCH_MX51
#define cpu_is_mx51()		(1)
#else
#define cpu_is_mx51()		(0)
#endif

#ifdef CONFIG_ARCH_MX53
#define cpu_is_mx53()		(1)
#else
#define cpu_is_mx53()		(0)
#endif

#ifdef CONFIG_ARCH_MX50
#define cpu_is_mx50()		(1)
#else
#define cpu_is_mx50()		(0)
#endif

extern int mxc_cpu_type(void);
extern void mxc_set_cpu_rev(unsigned int part, unsigned int rev);
extern int mxc_is_cpu(int part);
extern int cpu_is_mx25_rev(unsigned int rev);
extern int cpu_is_mx27_rev(unsigned int rev);
extern int cpu_is_mx31_rev(unsigned int rev);
extern int cpu_is_mx32_rev(unsigned int rev);
extern int cpu_is_mx35_rev(unsigned int rev);
extern int cpu_is_mx51_rev(unsigned int rev);
extern int cpu_is_mx53_rev(unsigned int rev);

#endif /* #ifndef __ASSEMBLY__ */

#include <asm/arch/mxc.h>

/*****************************************************************************
 * Board specific defines
 *****************************************************************************/
#if defined(CONFIG_MACH_MX31ADS)
#include <asm/arch/board-mx31ads.h>
#endif

#if defined(CONFIG_MACH_ARMADILLO500)
#include <asm/arch/board-armadillo5x0.h>
#endif

#if defined(CONFIG_MACH_ARMADILLO500FX)
#include <asm/arch/board-armadillo500fx.h>
#endif

#define MXC_MAX_GPIO_LINES	(GPIO_NUM_PIN * GPIO_PORT_NUM)

#if defined(CONFIG_MACH_ARMADILLO460)
#define MXC_EXT_INT_BASE	(MXC_GPIO_INT_BASE + MXC_MAX_GPIO_LINES)
#define MXC_MAX_EXT_INT_LINES	(16)
#endif

#if !defined(CONFIG_ARCH_MX25)
#define MXC_EXP_IO_BASE         (MXC_GPIO_INT_BASE + MXC_MAX_GPIO_LINES)
#define MXC_MAX_EXP_IO_LINES    16
#endif

#if defined(CONFIG_MXC_PSEUDO_IRQS)
#define MXC_PSEUDO_IO_BASE      (MXC_EXP_IO_BASE + MXC_MAX_EXP_IO_LINES)
#define MXC_MAX_PSEUDO_IO_LINES 16
#else
#define MXC_MAX_PSEUDO_IO_LINES 0
#endif

#ifndef MXC_INT_FORCE
#define MXC_INT_FORCE           -1
#endif

#ifndef MXC_MAX_EXP_IO_LINES
#define MXC_MAX_EXP_IO_LINES 0
#endif
#ifndef MXC_MAX_EXT_INT_LINES
#define MXC_MAX_EXT_INT_LINES 0
#endif

#define MXC_MAX_INTS   (MXC_MAX_INT_LINES +	     \
                        MXC_MAX_GPIO_LINES +         \
                        MXC_MAX_EXT_INT_LINES +      \
                        MXC_MAX_EXP_IO_LINES +	     \
                        MXC_MAX_PSEUDO_IO_LINES)

#endif /* __ASM_ARCH_MXC_HARDWARE_H__ */
