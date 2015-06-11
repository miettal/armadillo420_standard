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

#ifndef __ARCH_MACH_MX3_BOARD_ARMADILLO5X0_H__
#define __ARCH_MACH_MX3_BOARD_ARMADILLO5X0_H__

/*
 * Include Files
 */
#include <asm/arch/mxc_uart.h>

/*!
 * Specifies if the Irda transmit path is inverting
 */
#define MXC_IRDA_TX_INV         0
/*!
 * Specifies if the Irda receive path is inverting
 */
#define MXC_IRDA_RX_INV         0

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

/* UART 2 configuration */
#define UART2_MODE              MODE_DTE
#define UART2_IR                NO_IRDA

/* UART 3 configuration */
#define UART3_MODE              MODE_DTE
#define UART3_IR                NO_IRDA

/* UART 4 configuration */
#define UART4_MODE              MODE_DTE
#define UART4_IR                NO_IRDA

/* UART 5 configuration */
#define UART5_MODE              MODE_DTE
#define UART5_IR                NO_IRDA

#endif /* __ASM_ARCH_MXC_BOARD_ARMADILLO5X0_H__ */
