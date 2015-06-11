/*
 * Copyright (C) 2011 Atmark Techno, Inc. All Rights Reserved.
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

#ifndef __ASM_ARCH_ARMADILLO460_CPLD_H__
#define __ASM_ARCH_ARMADILLO460_CPLD_H__

#include <linux/io.h>
#include <asm/arch/gpio.h>

#define cpld_base		CS1_BASE_ADDR_VIRT
#define cpld_irq_base		MXC_EXT_INT_BASE
#define cpld_irq_master		GPIO_TO_IRQ(GPIO(3, 23))

#define cpld_readb(a)		readb(cpld_base + (a))
#define cpld_readw(a)		readw(cpld_base + (a))
#define cpld_writeb(v, a)	writeb(v, cpld_base + (a))
#define cpld_writew(v, a)	writew(v, cpld_base + (a))

/* CPLD register offset */
#define PISR0		(0x00)	/* Ext Interrupt Status0 */
#define PISR1		(0x01)	/* Ext Interrupt Status1 */
#define PISR		(PISR0)
#define PIMR0		(0x02)	/* Ext Interrupt Mask0 */
#define PIMR1		(0x03)	/* Ext Interrupt Mask1 */
#define PIMR		(PIMR0)
#define PIPT0		(0x04)	/* Ext Interrupt Polarity Type0 */
#define PIPT1		(0x05)	/* Ext Interrupt Polarity Type1 */
#define PIPT		(PIPT0)
#define PIDT0		(0x06)	/* Ext Interrupt Detect Type0 */
#define PIDT1		(0x07)	/* Ext Interrupt Detect Type1 */
#define PIDT		(PIDT0)
#define PBCR		(0x08)	/* Ext Bus Control */
#define PICR		(0x09)	/* Ext I/F Control */
#define PRTC		(0x0a)	/* RTC Control */
#define PVER		(0x0f)	/* CPLD Version */

#define PIRQ_MASK	0xdef8
#define PISR_MASK	PIRQ_MASK
#define PIMR_MASK	PIRQ_MASK
#define PIPT_RESET_VAL	0x0ef8
#define PIDT_RESET_VAL	0x0000

/* PBCR: Ext Bus Control Register definitions */
#define PBCR_RST	(1 << 0)
#define PBCR_PC104	(0 << 1)
#define PBCR_ASYNC	(1 << 1)
#define PBCR_SYNC	(3 << 1)
#define PBCR_CLK_R	(1 << 3)

/* PICR: Ext I/F Control Register definitions */
#define PICR_IF_EN	(1 << 0)
#define PICR_SEL1_C11	(1 << 1)
#define PICR_SEL1_C19	(0 << 1)
#define PICR_SEL2_C11	(1 << 2)
#define PICR_SEL2_C19	(0 << 2)
#define PICR_SEL_CON11	(3 << 1)
#define PICR_SEL_CON19	(0 << 1)

#endif
