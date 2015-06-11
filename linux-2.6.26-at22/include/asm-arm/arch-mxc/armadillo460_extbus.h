/*
 *  Copyright 2011 Atmark Techno, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _LINUX_ARMADILLO460_EXTBUS_H_
#define _LINUX_ARMADILLO460_EXTBUS_H_

#if defined(CONFIG_MACH_ARMADILLO460)
#include <asm/mach-types.h>
#include <asm/arch/hardware.h>

/*
 * PC104 memory map:
 *
 * Virt     Phys        Size    What
 * ------------------------------------------------------
 * F2000000 B2000000    64K     PC104 I/O area (8bit)
 * F2010000 B2010000            reserved
 * F3000000 B3000000    16M     PC104 Memory area (8bit)
 * F4000000 B4000000    64K     PC104 I/O area (16bit)
 * F4010000 B4010000            reserved
 * F5000000 B5000000    16M     PC104 Memory area (16bit)
 */

#define A460_IO_ADDR_BASE	(CS3_BASE_ADDR_VIRT)
#define A460_IO_SIZE		(0x00010000)
#define A460_IO16_OFFSET	(0x02000000)

#define A460_MEM_ADDR_BASE	(CS3_BASE_ADDR_VIRT + 0x01000000)
#define A460_MEM_SIZE		(0x01000000)
#define A460_MEM16_OFFSET	(0x02000000)

#ifdef __io

#define is_io_addr(a) \
	((A460_IO_ADDR_BASE <= (__u32)a) && \
	 ((__u32)a < A460_IO_ADDR_BASE + A460_IO_SIZE))

static inline void mach_outw(__u16 d, void __iomem *a)
{
	if (is_io_addr(a))
		a += A460_IO16_OFFSET;

	__raw_writew(cpu_to_le16(d), a);
}

static inline void mach_outl(__u32 d, void __iomem *a)
{
	if (is_io_addr(a))
		a += A460_IO16_OFFSET;

	__raw_writel(cpu_to_le32(d), a);
}

static inline __u16 mach_inw(void __iomem *a)
{
	if (is_io_addr(a))
		a += A460_IO16_OFFSET;

	return le16_to_cpu((__force __le16)__raw_readw(a));
}

static inline __u32 mach_inl(void __iomem *a)
{
	if (is_io_addr(a))
		a += A460_IO16_OFFSET;

	return le32_to_cpu((__force __le32)__raw_readl(a));
}

static inline void mach_outsw(void __iomem *a, const void *b, int c)
{
	if (is_io_addr(a))
		a += A460_IO16_OFFSET;

	__raw_writesw(a, b, c);
}

static inline void mach_outsl(void __iomem *a, const void *b, int c)
{
	if (is_io_addr(a))
		a += A460_IO16_OFFSET;

	__raw_writesl(a, b, c);
}

static inline void mach_insw(const void __iomem *a, void *b, int c)
{
	if (is_io_addr(a))
		a += A460_IO16_OFFSET;

	__raw_readsw(a, b, c);
}

static inline void mach_insl(const void __iomem *a, void *b, int c)
{
	if (is_io_addr(a))
		a += A460_IO16_OFFSET;

	__raw_readsl(a, b, c);
}

#undef outw
#define outw(v,p)	mach_outw(v,__io(p))

#undef outl
#define outl(v,p)	mach_outl(v,__io(p))

#undef inw
#define inw(p)		mach_inw(__io(p))

#undef inl
#define inl(p)		mach_inl(__io(p))

#undef outsw
#define outsw(p,d,l)	mach_outsw(__io(p),d,l)

#undef outsl
#define outsl(p,d,l)	mach_outsl(__io(p),d,l)

#undef insw
#define insw(p,d,l)	mach_insw(__io(p),d,l)

#undef insl
#define insl(p,d,l)	mach_insl(__io(p),d,l)

#else
#error you must include this file after "asm/io.h".
#endif

static inline unsigned int convirq_to_isa(unsigned int irq)
{
	return machine_is_armadillo460() ? irq - MXC_EXT_INT_BASE : irq;
}

static inline unsigned int convirq_from_isa(unsigned int irq)
{
	return machine_is_armadillo460() ? irq + MXC_EXT_INT_BASE : irq;
}
#else
#define convirq_to_isa(x) (x)
#define convirq_from_isa(x) (x)
#endif

#endif
