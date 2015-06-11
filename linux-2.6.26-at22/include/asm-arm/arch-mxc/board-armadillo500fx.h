/*
 *  Copyright 2008 Atmark Techno, Inc. All Rights Reserved.
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

#ifndef __ASM_ARCH_MXC_BOARD_ARMADILLO500FX_H__
#define __ASM_ARCH_MXC_BOARD_ARMADILLO500FX_H__

#define MXC_INT_GPIO_P1(x) MXC_GPIO_TO_IRQ(GPIO_NUM_PIN * 0 + (x))
#define MXC_INT_GPIO_P2(x) MXC_GPIO_TO_IRQ(GPIO_NUM_PIN * 1 + (x))
#define MXC_INT_GPIO_P3(x) MXC_GPIO_TO_IRQ(GPIO_NUM_PIN * 2 + (x))

#ifndef __ASSEMBLY__

struct armadillo5x0_gpio_port {
	char *name;
	__u32 pin;
	int irq;
	int dir_ro;
};

struct armadillo5x0_gpio_private {
	int nr_gpio;
	struct armadillo5x0_gpio_port *ports;
};

#endif /* __ASSEMBLY__ */

#endif /* __ASM_ARCH_MXC_BOARD_ARMADILLO500FX_H__ */
