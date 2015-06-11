/*
 * Copyright (C) 2009 Atmark Techno, Inc. All Rights Reserved.
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
#ifndef __ASM_ARCH_BOARD_ARMADILLO400_H__
#define __ASM_ARCH_BOARD_ARMADILLO400_H__

#include <linux/gpio.h>
#include <asm/arch/mxc_uart.h>

#include "devices.h"

//#define ARMADILLO_400_BOARD_REV_A is not supported
#define ARMADILLO_400_BOARD_REV_B         0x0200
#define ARMADILLO_400_BOARD_REV_C         0x0300
#define ARMADILLO_400_BOARD_REV_C1        0x0301
#define ARMADILLO_400_BOARD_REV_MAJOR_MASK 0xff00

/*!
 * @name MXC UART board-level configurations
 */

/* UART 1 configuration */
/*!
 * This define specifies if the UART port is configured to be in DTE or
 * DCE mode. There exists a define like this for each UART port. Valid
 * values that can be used are \b MODE_DTE or \b MODE_DCE.
 */
#define UART1_MODE              MODE_DTE
/*!
 * This define specifies if the UART is to be used for IRDA. There exists a
 * define like this for each UART port. Valid values that can be used are
 * \b IRDA or \b NO_IRDA.
 */
#define UART1_IR                NO_IRDA
/*!
 * This define is used to determine the regulator name for the UART port.
 */
#define UART1_REGULATOR_NAME    NULL

/* UART 2 configuration */
#define UART2_MODE              MODE_DTE
#define UART2_IR                NO_IRDA
#define UART2_REGULATOR_NAME    NULL

/* UART 3 configuration */
#define UART3_MODE              MODE_DTE
#define UART3_IR                NO_IRDA
#define UART3_REGULATOR_NAME    "REG5"

/* UART 4 configuration */
#define UART4_MODE              MODE_DTE
#define UART4_IR                NO_IRDA
#define UART4_REGULATOR_NAME    NULL

/* UART 5 configuration */
#define UART5_MODE              MODE_DTE
#define UART5_IR                NO_IRDA
#define UART5_REGULATOR_NAME    "REG5"

#define MXC_LL_UART_PADDR	UART2_BASE_ADDR
#define MXC_LL_UART_VADDR	AIPS1_IO_ADDRESS(UART2_BASE_ADDR)

#define UART2_FORCEOFF_GPIO	GPIO(4, 31)
#define UART2_DSR_GPIO		GPIO(4, 23)

#define PHY_RST_GPIO		GPIO(3, 18)
#define LINK_LED_GPIO		GPIO(3, 16)

#define USB_PWRSEL_GPIO		GPIO(3, 26)
#define USB_PWRSRC_5V		1
#define USB_PWRSRC_VIN		0
#define USB_PWRSRC		USB_PWRSRC_VIN

#define SDHC1_CD_GPIO		GPIO(3, 31)
#define SDHC1_WP_GPIO		-1
#define SDHC1_WP_GPIO_A460	GPIO(4, 4)
#define SDHC1_PWRE_GPIO_REVB	GPIO(3, 17)
#define SDHC1_PWRE_GPIO_REVC	GPIO(3, 27)
#define SDHC2_CD_GPIO		GPIO(4, 21)
#define SDHC2_WP_GPIO		GPIO(1, 7)

extern int sdhc1_wp_gpio;

enum mxc_ext_gpio_direction {
	MXC_EXT_GPIO_DIRECTION_OUTPUT,
	MXC_EXT_GPIO_DIRECTION_INPUT,
};

struct mxc_ext_gpio {
	const char *name;
	u32 gpio;
	enum mxc_ext_gpio_direction default_direction;
	int default_value;
};

extern void gpio_activate_ext_gpio(struct mxc_ext_gpio *ext_gpios, int nr_ext_gpios);

enum armadillo460_i2c_id {
	ARMADILLO460_EXT_I2C_ID = MX25_NR_I2C_IDS,
};

extern void mxc_cpu_init(void) __init;
extern void mxc_cpu_common_init(void);
extern int mxc_clocks_init(void);
extern void __init early_console_setup(char *);

#endif				/* __ASM_ARCH_BOARD_ARMADILLO400__ */
