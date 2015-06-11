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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/sysdev.h>
#include <asm/arch/iomux-mx25.h>
#include "weim_regs.h"
#include "armadillo460_cpld.h"

#define CSCRxU_PC104	0x00015980
#define CSCRxL_PC104_8	0x0f000341
#define CSCRxL_PC104_16	0x0f000541
#define CSCRxA_PC104	0x000f0000
#define CSCRxU_SYNC	0x00014480
#define CSCRxL_SYNC_8	0x04000341
#define CSCRxL_SYNC_16	0x04000541
#define CSCRxA_SYNC	0x00040000

#if defined(CONFIG_ARMADILLO460_EXT_BUS_PC104_MODE)
#define CSCR3U_VAL	CSCRxU_PC104
#define CSCR3L_VAL	CSCRxL_PC104_8
#define CSCR3A_VAL	CSCRxA_PC104
#define CSCR4U_VAL	CSCRxU_PC104
#define CSCR4L_VAL	CSCRxL_PC104_16
#define CSCR4A_VAL	CSCRxA_PC104
#define WCR_ORR_AUS3	0
#define WCR_ORR_AUS4	WCR_AUS(4)
#define PBCR_ORR_MODE	PBCR_PC104
#define PBCR_ORR_CLK_R	0
#endif

#if defined(CONFIG_ARMADILLO460_EXT_BUS_DIRECT_CPU_SYNC_MODE)
#define CSCR3U_VAL	CSCRxU_SYNC
#if defined(CONFIG_ARMADILLO460_EXT_BUS_DIRECT_CPU_CS3_8BIT)
#define CSCR3L_VAL	CSCRxL_SYNC_8
#else
#define CSCR3L_VAL	CSCRxL_SYNC_16
#endif
#define CSCR3A_VAL	CSCRxA_SYNC
#define CSCR4U_VAL	0x00000000
#define CSCR4L_VAL	0x00000000
#define CSCR4A_VAL	0x00000000
#if defined(CONFIG_ARMADILLO460_EXT_BUS_DIRECT_CPU_CS3_8BIT)
#define WCR_ORR_AUS3	0
#else
#define WCR_ORR_AUS3	WCR_AUS(3)
#endif
#define WCR_ORR_AUS4	0
#define PBCR_ORR_MODE	PBCR_SYNC
#if defined(CONFIG_ARMADILLO460_EXT_BUS_DIRECT_CPU_INVERTED_SYSCLK)
#define PBCR_ORR_CLK_R	PBCR_CLK_R
#else
#define PBCR_ORR_CLK_R	0
#endif
#endif

#if defined(CONFIG_ARMADILLO460_EXT_BUS_DIRECT_CPU_ASYNC_MODE)
#define CSCR3U_VAL	CONFIG_ARMADILLO460_EXT_BUS_DIRECT_CPU_CS3_CSCRU
#define CSCR3L_VAL	CONFIG_ARMADILLO460_EXT_BUS_DIRECT_CPU_CS3_CSCRL
#define CSCR3A_VAL	CONFIG_ARMADILLO460_EXT_BUS_DIRECT_CPU_CS3_CSCRA
#define CSCR4U_VAL	CONFIG_ARMADILLO460_EXT_BUS_DIRECT_CPU_CS4_CSCRU
#define CSCR4L_VAL	CONFIG_ARMADILLO460_EXT_BUS_DIRECT_CPU_CS4_CSCRL
#define CSCR4A_VAL	CONFIG_ARMADILLO460_EXT_BUS_DIRECT_CPU_CS4_CSCRA
#if defined(CONFIG_ARMADILLO460_EXT_BUS_DIRECT_CPU_CS3_AUS)
#define WCR_ORR_AUS3	WCR_AUS(3)
#else
#define WCR_ORR_AUS3	0
#endif
#if defined(CONFIG_ARMADILLO460_EXT_BUS_DIRECT_CPU_CS4_AUS)
#define WCR_ORR_AUS4	WCR_AUS(4)
#else
#define WCR_ORR_AUS4	0
#endif
#define PBCR_ORR_MODE	PBCR_ASYNC
#define PBCR_ORR_CLK_R	0
#endif

#define WCR_VAL		(WCR_ORR_AUS3 | WCR_ORR_AUS4)
#define PBCR_VAL	(PBCR_ORR_MODE | PBCR_ORR_CLK_R)

static struct pad_desc __initdata armadillo460_ext_bus_pads[] = {
	MX25_PAD_CS4__CS4(PAD_CTL_DVS | PAD_CTL_SRE_FAST),
	MX25_PAD_CS5__DTACK_B(PAD_CTL_DVS | PAD_CTL_HYS | PAD_CTL_SRE_FAST),
};

static void __init armadillo460_init_ext_bus(void)
{
	u32 val;

	mxc_iomux_v3_setup_multiple_pads(armadillo460_ext_bus_pads,
					 ARRAY_SIZE(armadillo460_ext_bus_pads));

	val = readl(IOMUXGPR) | 0x2;
	writel(val, IOMUXGPR);

	writel(CSCR3U_VAL, CSCR_U(3));
	writel(CSCR3A_VAL, CSCR_A(3));
	writel(CSCR3L_VAL, CSCR_L(3));

	writel(CSCR4U_VAL, CSCR_U(4));
	writel(CSCR4A_VAL, CSCR_A(4));
	writel(CSCR4L_VAL, CSCR_L(4));

	val = readl(WCR) & ~(WCR_AUS(3) | WCR_AUS(4));
	val |= WCR_VAL;
	writel(val, WCR);

	cpld_writeb(PBCR_VAL, PBCR);
}

static void __init armadillo460_init_ext_if(void)
{
	u8 val = PICR_IF_EN | PICR_SEL_CON11;

#if defined(CONFIG_ARMADILLO460_UART4_CON19)
	val &= ~PICR_SEL1_C11;
#if defined(CONFIG_ARMADILLO460_UART4_HW_FLOW_CON19)
	val &= ~PICR_SEL2_C11;
#endif
#endif

	cpld_writeb(val, PICR);
}

static u8 cpld_gpio_reg;

static int ext_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	u8 reg;

	reg = cpld_gpio_reg;
	reg |= (1 << offset);
	cpld_writeb(reg, PRTC);
	cpld_gpio_reg = reg;

	return 0;
}

static int ext_gpio_direction_output(struct gpio_chip *chip, unsigned offset, int val)
{
	u8 reg;

	reg = cpld_gpio_reg;
	if (val)
		reg |= (1 << offset);
	else
		reg &= ~(1 << offset);
	cpld_writeb(reg, PRTC);
	cpld_gpio_reg = reg;

	return 0;
}

static int ext_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	u8 reg;

	reg = cpld_gpio_reg;
	reg |= (1 << offset);
	cpld_writeb(reg, PRTC);
	reg = cpld_readb(PRTC);

	return (reg & (1 << offset)) ? 1 : 0;
}

static void ext_gpio_set(struct gpio_chip *chip, unsigned offset, int val)
{
	ext_gpio_direction_output(chip, offset, val);
}

static struct gpio_chip armadillo460_ext_gpio = {
	.label = "armadillo460_ext_gpio",
	.direction_input = ext_gpio_direction_input,
	.direction_output = ext_gpio_direction_output,
	.get = ext_gpio_get,
	.set = ext_gpio_set,
	.base = A460_CPLD_GPIO_BASE,
	.ngpio = A460_CPLD_GPIO_NUM,
};

static void __init armadillo460_init_ext_gpio(void)
{
	gpiochip_add(&armadillo460_ext_gpio);
}

static void armadillo460_ext_irq_handler(unsigned int irq, struct irq_desc *desc)
{
	unsigned int extirq;
	unsigned short isr, imr;
	unsigned short valid;

	isr = cpld_readw(PISR);
	imr = cpld_readw(PIMR) & PIMR_MASK;

	valid = isr & imr;

	extirq = cpld_irq_base;
	for (; valid != 0; valid >>= 1, extirq++) {
		struct irq_desc *d;

		if ((valid & 1) == 0)
			continue;

		d = irq_desc + extirq;
		d->handle_irq(extirq, d);
	}
}

static void ext_irq_ack(unsigned int _irq)
{
	unsigned int irq = _irq - cpld_irq_base;

	cpld_writew(1 << irq, PISR);
}

static void ext_irq_mask(unsigned int _irq)
{
	unsigned int irq = _irq - cpld_irq_base;
	unsigned short val;

	val = cpld_readw(PIMR);
	val &= ~(1 << irq);
	cpld_writew(val, PIMR);
}

static void ext_irq_unmask(unsigned int _irq)
{
	unsigned int irq = _irq - cpld_irq_base;
	unsigned short val;

	val = cpld_readw(PIMR);
	val |= (1 << irq);
	cpld_writew(val, PIMR);
}

static int ext_irq_set_type(unsigned int irq, unsigned int flow_type)
{
	static unsigned short ipt = PIPT_RESET_VAL;
	static unsigned short idt = PIDT_RESET_VAL;
	irq_flow_handler_t irq_handler;
	unsigned int irqbit = irq - cpld_irq_base;

	if (irqbit == 12 || irqbit == 14 || irqbit == 15) {
		if (unlikely(flow_type != IRQT_HIGH))
			return -EINVAL;
		goto exit;
	}

	switch (flow_type) {
	case IRQT_RISING:
		idt |= (1 << irqbit);
		ipt |= (1 << irqbit);
		irq_handler = handle_edge_irq;
		break;
	case IRQT_FALLING:
		idt |= (1 << irqbit);
		ipt &= ~(1 << irqbit);
		irq_handler = handle_edge_irq;
		break;
	case IRQT_LOW:
		idt &= ~(1 << irqbit);
		ipt &= ~(1 << irqbit);
		irq_handler = handle_level_irq;
		break;
	case IRQT_HIGH:
		idt &= ~(1 << irqbit);
		ipt |= (1 << irqbit);
		irq_handler = handle_level_irq;
		break;
	case IRQT_BOTHEDGE:
		/* fall through */
	default:
		return -EINVAL;
	}

	set_irq_handler(irq, irq_handler);
	cpld_writew(ipt, PIPT);
	cpld_writew(idt, PIDT);

exit:
	/* pending clear */
	ext_irq_ack(irq);

	return 0;
}

static unsigned short _ext_irq_wakeable;
static unsigned short _ext_irq_saved;

static int ext_irq_set_wake(unsigned int irq, unsigned int enable)
{
	unsigned int irqbit = irq - cpld_irq_base;

	if (enable)
		_ext_irq_wakeable |= (1 << irqbit);
	else
		_ext_irq_wakeable &= ~(1 << irqbit);

	return 0;
}

static int ext_irq_suspend(struct sys_device *dev, pm_message_t mesg)
{
	_ext_irq_saved = cpld_readw(PIMR);
	cpld_writew(_ext_irq_wakeable, PIMR);

	return 0;
}

static int ext_irq_resume(struct sys_device *dev)
{
	cpld_writew(_ext_irq_saved, PIMR);

	return 0;
}

static struct irq_chip armadillo460_ext_irq_chip = {
	.name = "EXT_CPLD",
	.ack = ext_irq_ack,
	.mask = ext_irq_mask,
	.unmask = ext_irq_unmask,
	.set_type = ext_irq_set_type,
	.set_wake = ext_irq_set_wake,
};

static struct sysdev_class ext_irq_sysclass = {
	.name = "ext_irq",
	.suspend = ext_irq_suspend,
	.resume = ext_irq_resume,
};

static struct sys_device ext_irq_device = {
	.id = 0,
	.cls = &ext_irq_sysclass,
};

static void armadillo460_init_ext_irq(void)
{
	unsigned int irq;
	int i;

	/* register ext_irq sysdev class for power-management */
	if (!sysdev_class_register(&ext_irq_sysclass))
		sysdev_register(&ext_irq_device);

	/* disable all irq and clear all pending irqs. */
	cpld_writew(0x0000, PIMR);
	cpld_writew(0xffff, PISR);

	for (i = 3; i < 16; i++) {
		if (i == 8 || i == 13)
			/* irq 0,1,2,8,13 are not supported */
			continue;
		irq = cpld_irq_base + i;
		set_irq_chip(irq, &armadillo460_ext_irq_chip);
		set_irq_type(irq, IRQT_HIGH);
		set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
	}

	set_irq_type(cpld_irq_master, IRQT_LOW);
	set_irq_chained_handler(cpld_irq_master, armadillo460_ext_irq_handler);
	enable_irq_wake(cpld_irq_master);
}

static struct pad_desc __initdata armadillo460_cpld_pads[] = {
	MX25_PAD_CS1__CS1,
	MX25_PAD_ECB__GPIO_3_23(PAD_CTL_DVS | PAD_CTL_HYS),
};

void __init armadillo460_init_cpld(void)
{
	mxc_iomux_v3_setup_multiple_pads(armadillo460_cpld_pads,
					 ARRAY_SIZE(armadillo460_cpld_pads));

	writel(0x00010800, CSCR_U(1));
	writel(0x00040000, CSCR_A(1));
	writel(0x44004341, CSCR_L(1));

	printk(KERN_INFO "Initializing CPLD: v%d\n", cpld_readb(PVER));

	armadillo460_init_ext_irq();
	armadillo460_init_ext_gpio();
	armadillo460_init_ext_if();
	armadillo460_init_ext_bus();
}
