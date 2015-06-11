/*
 * arch/arm/plat-mxc/include/mach/iomux-mx25.h
 *
 * Copyright (C) 2009 by Lothar Wassmann <LW@KARO-electronics.de>
 *
 * based on arch/arm/mach-mx25/mx25_pins.h
 *    Copyright 2008 Freescale Semiconductor, Inc. All Rights Reserved.
 * and
 * arch/arm/plat-mxc/include/mach/iomux-mx35.h
 *    Copyright (C, NO_PAD_CTRL) 2009 by Jan Weitzel Phytec Messtechnik GmbH <armlinux@phytec.de>
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#ifndef __IOMUX_MX25_H__
#define __IOMUX_MX25_H__

#include <asm/arch/iomux-v3.h>
#include <asm/arch/mx25.h>

/* #ifndef GPIO_PORTA */
/* #error Please include mach/iomux.h */
/* #endif */

/*
 *
 * @brief MX25 I/O Pin List
 *
 * @ingroup GPIO_MX25
 */

#ifndef __ASSEMBLY__

#define IOMUXGPR	IO_ADDRESS(IOMUXC_BASE_ADDR)
/*
 * IOMUX/PAD Bit field definitions
 */

#define MX25_PAD_A10__A10				IOMUX_PAD(0x000, 0x008, 0x00, 0, 0, NO_PAD_CTRL)
#define MX25_PAD_A10__GPIO_4_0				IOMUX_PAD(0x000, 0x008, 0x05, 0, 0, NO_PAD_CTRL)

#define MX25_PAD_A13__A13(pad_ctrl)			IOMUX_PAD(0x22C, 0x00c, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_A13__GPIO_4_1(pad_ctrl)		IOMUX_PAD(0x22C, 0x00c, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_A14__A14(pad_ctrl)			IOMUX_PAD(0x230, 0x010, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_A14__GPIO_2_0(pad_ctrl)		IOMUX_PAD(0x230, 0x010, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_A15__A15(pad_ctrl)			IOMUX_PAD(0x234, 0x014, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_A15__GPIO_2_1(pad_ctrl)		IOMUX_PAD(0x234, 0x014, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_A16__A16				IOMUX_PAD(0x000, 0x018, 0x00, 0, 0, NO_PAD_CTRL)
#define MX25_PAD_A16__GPIO_2_2				IOMUX_PAD(0x000, 0x018, 0x05, 0, 0, NO_PAD_CTRL)

#define MX25_PAD_A17__A17(pad_ctrl)			IOMUX_PAD(0x238, 0x01c, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_A17__GPIO_2_3(pad_ctrl)		IOMUX_PAD(0x238, 0x01c, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_A18__A18(pad_ctrl)			IOMUX_PAD(0x23c, 0x020, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_A18__GPIO_2_4(pad_ctrl)		IOMUX_PAD(0x23c, 0x020, 0x05, 0, 0, pad_ctrl)
#define MX25_PAD_A18__FEC_COL(pad_ctrl)			IOMUX_PAD(0x23c, 0x020, 0x07, 0x504, 0, pad_ctrl)

#define MX25_PAD_A19__A19(pad_ctrl)			IOMUX_PAD(0x240, 0x024, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_A19__FEC_RX_ER(pad_ctrl)		IOMUX_PAD(0x240, 0x024, 0x07, 0x518, 0, pad_ctrl)
#define MX25_PAD_A19__GPIO_2_5(pad_ctrl)		IOMUX_PAD(0x240, 0x024, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_A20__A20(pad_ctrl)			IOMUX_PAD(0x244, 0x028, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_A20__GPIO_2_6(pad_ctrl)		IOMUX_PAD(0x244, 0x028, 0x05, 0, 0, pad_ctrl)
#define MX25_PAD_A20__FEC_RDATA2(pad_ctrl)		IOMUX_PAD(0x244, 0x028, 0x07, 0x50c, 0, pad_ctrl)

#define MX25_PAD_A21__A21(pad_ctrl)			IOMUX_PAD(0x248, 0x02c, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_A21__GPIO_2_7(pad_ctrl)		IOMUX_PAD(0x248, 0x02c, 0x05, 0, 0, pad_ctrl)
#define MX25_PAD_A21__FEC_RDATA3(pad_ctrl)		IOMUX_PAD(0x248, 0x02c, 0x07, 0x510, 0, pad_ctrl)

#define MX25_PAD_A22__A22				IOMUX_PAD(0x000, 0x030, 0x00, 0, 0, NO_PAD_CTRL)
#define MX25_PAD_A22__GPIO_2_8				IOMUX_PAD(0x000, 0x030, 0x05, 0, 0, NO_PAD_CTRL)

#define MX25_PAD_A23__A23(pad_ctrl)			IOMUX_PAD(0x24c, 0x034, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_A23__GPIO_2_9(pad_ctrl)		IOMUX_PAD(0x24c, 0x034, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_A24__A24(pad_ctrl)			IOMUX_PAD(0x250, 0x038, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_A24__GPIO_2_10(pad_ctrl)		IOMUX_PAD(0x250, 0x038, 0x05, 0, 0, pad_ctrl)
#define MX25_PAD_A24__FEC_RX_CLK(pad_ctrl)		IOMUX_PAD(0x250, 0x038, 0x07, 0x514, 0, pad_ctrl)

#define MX25_PAD_A25__A25(pad_ctrl)			IOMUX_PAD(0x254, 0x03c, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_A25__GPIO_2_11(pad_ctrl)		IOMUX_PAD(0x254, 0x03c, 0x05, 0, 0, pad_ctrl)
#define MX25_PAD_A25__FEC_CRS(pad_ctrl)			IOMUX_PAD(0x254, 0x03c, 0x07, 0x508, 0, pad_ctrl)

#define MX25_PAD_EB0__EB0(pad_ctrl)			IOMUX_PAD(0x258, 0x040, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_EB0__AUD4_TXD(pad_ctrl)		IOMUX_PAD(0x258, 0x040, 0x04, 0x464, 0, pad_ctrl)
#define MX25_PAD_EB0__GPIO_2_12(pad_ctrl)		IOMUX_PAD(0x258, 0x040, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_EB1__EB1(pad_ctrl)			IOMUX_PAD(0x25c, 0x044, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_EB1__AUD4_RXD(pad_ctrl)		IOMUX_PAD(0x25c, 0x044, 0x04, 0x460, 0, pad_ctrl)
#define MX25_PAD_EB1__GPIO_2_13(pad_ctrl)		IOMUX_PAD(0x25c, 0x044, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_OE__OE(pad_ctrl)			IOMUX_PAD(0x260, 0x048, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_OE__AUD4_TXC(pad_ctrl)			IOMUX_PAD(0x260, 0x048, 0x04, 0, 0, pad_ctrl)
#define MX25_PAD_OE__GPIO_2_14(pad_ctrl)		IOMUX_PAD(0x260, 0x048, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_CS0__CS0				IOMUX_PAD(0x000, 0x04c, 0x00, 0, 0, NO_PAD_CTRL)
#define MX25_PAD_CS0__GPIO_4_2				IOMUX_PAD(0x000, 0x04c, 0x05, 0, 0, NO_PAD_CTRL)

#define MX25_PAD_CS1__CS1				IOMUX_PAD(0x000, 0x050, 0x00, 0, 0, NO_PAD_CTRL)
#define MX25_PAD_CS1__GPIO_4_3				IOMUX_PAD(0x000, 0x050, 0x05, 0, 0, NO_PAD_CTRL)

#define MX25_PAD_CS4__CS4(pad_ctrl)			IOMUX_PAD(0x264, 0x054, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_CS4__UART5_CTS(pad_ctrl)		IOMUX_PAD(0x264, 0x054, 0x03, 0, 0, pad_ctrl)
#define MX25_PAD_CS4__GPIO_3_20(pad_ctrl)		IOMUX_PAD(0x264, 0x054, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_CS5__CS5(pad_ctrl)			IOMUX_PAD(0x268, 0x058, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_CS5__DTACK_B(pad_ctrl)			IOMUX_PAD(0x268, 0x058, 0x02, 0, 0, pad_ctrl)
#define MX25_PAD_CS5__UART5_RTS(pad_ctrl)		IOMUX_PAD(0x268, 0x058, 0x03, 0x574, 0, pad_ctrl)
#define MX25_PAD_CS5__GPIO_3_21(pad_ctrl)		IOMUX_PAD(0x268, 0x058, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_NF_CE0__NF_CE0(pad_ctrl)		IOMUX_PAD(0x26c, 0x05c, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_NF_CE0__GPIO_3_22(pad_ctrl)		IOMUX_PAD(0x26c, 0x05c, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_ECB__ECB(pad_ctrl)			IOMUX_PAD(0x270, 0x060, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_ECB__UART5_TXD_MUX(pad_ctrl)		IOMUX_PAD(0x270, 0x060, 0x03, 0, 0, pad_ctrl)
#define MX25_PAD_ECB__GPIO_3_23(pad_ctrl)		IOMUX_PAD(0x270, 0x060, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_LBA__LBA(pad_ctrl)			IOMUX_PAD(0x274, 0x064, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_LBA__UART5_RXD_MUX(pad_ctrl)		IOMUX_PAD(0x274, 0x064, 0x03, 0x578, 0, pad_ctrl)
#define MX25_PAD_LBA__GPIO_3_24(pad_ctrl)		IOMUX_PAD(0x274, 0x064, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_BCLK__BCLK				IOMUX_PAD(0x000, 0x068, 0x00, 0, 0, NO_PAD_CTRL)
#define MX25_PAD_BCLK__GPIO_4_4				IOMUX_PAD(0x000, 0x068, 0x05, 0, 0, NO_PAD_CTRL)

#define MX25_PAD_RW__RW(pad_ctrl)			IOMUX_PAD(0x278, 0x06c, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_RW__AUD4_TXFS(pad_ctrl)		IOMUX_PAD(0x278, 0x06c, 0x04, 0x474, 0, pad_ctrl)
#define MX25_PAD_RW__GPIO_3_25(pad_ctrl)		IOMUX_PAD(0x278, 0x06c, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_NFWE_B__NFWE_B				IOMUX_PAD(0x000, 0x070, 0x00, 0, 0, NO_PAD_CTRL)
#define MX25_PAD_NFWE_B__GPIO_3_26			IOMUX_PAD(0x000, 0x070, 0x05, 0, 0, NO_PAD_CTRL)

#define MX25_PAD_NFRE_B__NFRE_B				IOMUX_PAD(0x000, 0x074, 0x00, 0, 0, NO_PAD_CTRL)
#define MX25_PAD_NFRE_B__GPIO_3_27			IOMUX_PAD(0x000, 0x074, 0x05, 0, 0, NO_PAD_CTRL)

#define MX25_PAD_NFALE__NFALE				IOMUX_PAD(0x000, 0x078, 0x00, 0, 0, NO_PAD_CTRL)
#define MX25_PAD_NFALE__GPIO_3_28			IOMUX_PAD(0x000, 0x078, 0x05, 0, 0, NO_PAD_CTRL)

#define MX25_PAD_NFCLE__NFCLE				IOMUX_PAD(0x000, 0x07c, 0x00, 0, 0, NO_PAD_CTRL)
#define MX25_PAD_NFCLE__GPIO_3_29			IOMUX_PAD(0x000, 0x07c, 0x05, 0, 0, NO_PAD_CTRL)

#define MX25_PAD_NFWP_B__NFWP_B				IOMUX_PAD(0x000, 0x080, 0x00, 0, 0, NO_PAD_CTRL)
#define MX25_PAD_NFWP_B__GPIO_3_30			IOMUX_PAD(0x000, 0x080, 0x05, 0, 0, NO_PAD_CTRL)

#define MX25_PAD_NFRB__NFRB(pad_ctrl)			IOMUX_PAD(0x27c, 0x084, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_NFRB__GPIO_3_31(pad_ctrl)		IOMUX_PAD(0x27c, 0x084, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_D15__D15(pad_ctrl)			IOMUX_PAD(0x280, 0x088, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_D15__LD16(pad_ctrl)			IOMUX_PAD(0x280, 0x088, 0x01, 0, 0, pad_ctrl)
#define MX25_PAD_D15__GPIO_4_5(pad_ctrl)		IOMUX_PAD(0x280, 0x088, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_D14__D14(pad_ctrl)			IOMUX_PAD(0x284, 0x08c, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_D14__LD17(pad_ctrl)			IOMUX_PAD(0x284, 0x08c, 0x01, 0, 0, pad_ctrl)
#define MX25_PAD_D14__GPIO_4_6(pad_ctrl)		IOMUX_PAD(0x284, 0x08c, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_D13__D13(pad_ctrl)			IOMUX_PAD(0x288, 0x090, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_D13__LD18(pad_ctrl)			IOMUX_PAD(0x288, 0x090, 0x01, 0, 0, pad_ctrl)
#define MX25_PAD_D13__GPIO_4_7(pad_ctrl)		IOMUX_PAD(0x288, 0x090, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_D12__D12(pad_ctrl)			IOMUX_PAD(0x28c, 0x094, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_D12__GPIO_4_8(pad_ctrl)		IOMUX_PAD(0x28c, 0x094, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_D11__D11(pad_ctrl)			IOMUX_PAD(0x290, 0x098, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_D11__GPIO_4_9(pad_ctrl)		IOMUX_PAD(0x290, 0x098, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_D10__D10(pad_ctrl)			IOMUX_PAD(0x294, 0x09c, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_D10__GPIO_4_10(pad_ctrl)		IOMUX_PAD(0x294, 0x09c, 0x05, 0, 0, pad_ctrl)
#define MX25_PAD_D10__USBOTG_OC(pad_ctrl)		IOMUX_PAD(0x294, 0x09c, 0x06, 0x57c, 0, pad_ctrl)

#define MX25_PAD_D9__D9	(pad_ctrl)			IOMUX_PAD(0x298, 0x0a0, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_D9__GPIO_4_11(pad_ctrl)		IOMUX_PAD(0x298, 0x0a0, 0x05, 0, 0, pad_ctrl)
#define MX25_PAD_D9__USBH2_PWR(pad_ctrl)		IOMUX_PAD(0x298, 0x0a0, 0x06, 0, 0, pad_ctrl)

#define MX25_PAD_D8__D8(pad_ctrl)			IOMUX_PAD(0x29c, 0x0a4, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_D8__GPIO_4_12(pad_ctrl)		IOMUX_PAD(0x29c, 0x0a4, 0x05, 0, 0, pad_ctrl)
#define MX25_PAD_D8__USBH2_OC(pad_ctrl)			IOMUX_PAD(0x29c, 0x0a4, 0x06, 0x580, 0, pad_ctrl)

#define MX25_PAD_D7__D7(pad_ctrl)			IOMUX_PAD(0x2a0, 0x0a8, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_D7__GPIO_4_13(pad_ctrl)		IOMUX_PAD(0x2a0, 0x0a8, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_D6__D6(pad_ctrl)			IOMUX_PAD(0x2a4, 0x0ac, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_D6__GPIO_4_14(pad_ctrl)		IOMUX_PAD(0x2a4, 0x0ac, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_D5__D5(pad_ctrl)			IOMUX_PAD(0x2a8, 0x0b0, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_D5__GPIO_4_15(pad_ctrl)		IOMUX_PAD(0x2a8, 0x0b0, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_D4__D4(pad_ctrl)			IOMUX_PAD(0x2ac, 0x0b4, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_D4__GPIO_4_16(pad_ctrl)		IOMUX_PAD(0x2ac, 0x0b4, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_D3__D3(pad_ctrl)			IOMUX_PAD(0x2b0, 0x0b8, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_D3__GPIO_4_17(pad_ctrl)		IOMUX_PAD(0x2b0, 0x0b8, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_D2__D2(pad_ctrl)			IOMUX_PAD(0x2b4, 0x0bc, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_D2__GPIO_4_18(pad_ctrl)		IOMUX_PAD(0x2b4, 0x0bc, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_D1__D1(pad_ctrl)			IOMUX_PAD(0x2b8, 0x0c0, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_D1__GPIO_4_19(pad_ctrl)		IOMUX_PAD(0x2b8, 0x0c0, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_D0__D0(pad_ctrl)			IOMUX_PAD(0x2bc, 0x0c4, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_D0__GPIO_4_20(pad_ctrl)		IOMUX_PAD(0x2bc, 0x0c4, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_LD0__LD0(pad_ctrl)			IOMUX_PAD(0x2c0, 0x0c8, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_LD0__CSI_D0(pad_ctrl)			IOMUX_PAD(0x2c0, 0x0c8, 0x02, 0x488, 0, pad_ctrl)
#define MX25_PAD_LD0__GPIO_2_15(pad_ctrl)		IOMUX_PAD(0x2c0, 0x0c8, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_LD1__LD1(pad_ctrl)			IOMUX_PAD(0x2c4, 0x0cc, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_LD1__CSI_D1(pad_ctrl)			IOMUX_PAD(0x2c4, 0x0cc, 0x02, 0x48c, 0, pad_ctrl)
#define MX25_PAD_LD1__GPIO_2_16(pad_ctrl)		IOMUX_PAD(0x2c4, 0x0cc, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_LD2__LD2(pad_ctrl)			IOMUX_PAD(0x2c8, 0x0d0, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_LD2__GPIO_2_17(pad_ctrl)		IOMUX_PAD(0x2c8, 0x0d0, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_LD3__LD3(pad_ctrl)			IOMUX_PAD(0x2cc, 0x0d4, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_LD3__GPIO_2_18(pad_ctrl)		IOMUX_PAD(0x2cc, 0x0d4, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_LD4__LD4(pad_ctrl)			IOMUX_PAD(0x2d0, 0x0d8, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_LD4__GPIO_2_19(pad_ctrl)		IOMUX_PAD(0x2d0, 0x0d8, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_LD5__LD5(pad_ctrl)			IOMUX_PAD(0x2d4, 0x0dc, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_LD5__GPIO_1_19(pad_ctrl)		IOMUX_PAD(0x2d4, 0x0dc, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_LD6__LD6(pad_ctrl)			IOMUX_PAD(0x2d8, 0x0e0, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_LD6__GPIO_1_20(pad_ctrl)		IOMUX_PAD(0x2d8, 0x0e0, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_LD7__LD7(pad_ctrl)			IOMUX_PAD(0x2dc, 0x0e4, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_LD7__GPIO_1_21(pad_ctrl)		IOMUX_PAD(0x2dc, 0x0e4, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_LD8__LD8(pad_ctrl)			IOMUX_PAD(0x2e0, 0x0e8, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_LD8__FEC_TX_ERR(pad_ctrl)		IOMUX_PAD(0x2e0, 0x0e8, 0x05, 0, 0, pad_ctrl)
#define MX25_PAD_LD8__UART4_RXD(pad_ctrl)		IOMUX_PAD(0x2e0, 0x0e8, 0x02, 0x570, 0, pad_ctrl)

#define MX25_PAD_LD9__LD9(pad_ctrl)			IOMUX_PAD(0x2e4, 0x0ec, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_LD9__FEC_COL(pad_ctrl)			IOMUX_PAD(0x2e4, 0x0ec, 0x05, 0x504, 1, pad_ctrl)
#define MX25_PAD_LD9__UART4_TXD(pad_ctrl)		IOMUX_PAD(0x2e4, 0x0ec, 0x02, 0, 0, pad_ctrl)

#define MX25_PAD_LD10__LD10(pad_ctrl)			IOMUX_PAD(0x2e8, 0x0f0, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_LD10__FEC_RX_ER(pad_ctrl)		IOMUX_PAD(0x2e8, 0x0f0, 0x05, 0x518, 1, pad_ctrl)
#define MX25_PAD_LD10__UART4_RTS(pad_ctrl)		IOMUX_PAD(0x2e8, 0x0f0, 0x02, 0x56c, 0, pad_ctrl)

#define MX25_PAD_LD11__LD11(pad_ctrl)			IOMUX_PAD(0x2ec, 0x0f4, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_LD11__FEC_RDATA2(pad_ctrl)		IOMUX_PAD(0x2ec, 0x0f4, 0x05, 0x50c, 1, pad_ctrl)
#define MX25_PAD_LD11__UART4_CTS(pad_ctrl)		IOMUX_PAD(0x2ec, 0x0f4, 0x02, 0, 0, pad_ctrl)

#define MX25_PAD_LD12__LD12(pad_ctrl)			IOMUX_PAD(0x2f0, 0x0f8, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_LD12__FEC_RDATA3(pad_ctrl)		IOMUX_PAD(0x2f0, 0x0f8, 0x05, 0x510, 1, pad_ctrl)
#define MX25_PAD_LD12__CSPI2_MOSI(pad_ctrl)		IOMUX_PAD(0x2f0, 0x0f8, 0x02, 0x4a0, 0, pad_ctrl)

#define MX25_PAD_LD13__LD13(pad_ctrl)			IOMUX_PAD(0x2f4, 0x0fc, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_LD13__FEC_TDATA2(pad_ctrl)		IOMUX_PAD(0x2f4, 0x0fc, 0x05, 0, 0, pad_ctrl)
#define MX25_PAD_LD13__CSPI2_MISO(pad_ctrl)		IOMUX_PAD(0x2f4, 0x0fc, 0x02, 0x49c, 0, pad_ctrl)

#define MX25_PAD_LD14__LD14(pad_ctrl)			IOMUX_PAD(0x2f8, 0x100, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_LD14__FEC_TDATA3(pad_ctrl)		IOMUX_PAD(0x2f8, 0x100, 0x05, 0, 0, pad_ctrl)
#define MX25_PAD_LD14__CSPI2_SCLK(pad_ctrl)		IOMUX_PAD(0x2f8, 0x100, 0x02, 0x494, 0, pad_ctrl)

#define MX25_PAD_LD15__LD15(pad_ctrl)			IOMUX_PAD(0x2fc, 0x104, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_LD15__FEC_RX_CLK(pad_ctrl)		IOMUX_PAD(0x2fc, 0x104, 0x05, 0x514, 1, pad_ctrl)
#define MX25_PAD_LD15__CSPI2_RDY(pad_ctrl)		IOMUX_PAD(0x2fc, 0x104, 0x02, 0x498, 0, pad_ctrl)

#define MX25_PAD_HSYNC__HSYNC(pad_ctrl)			IOMUX_PAD(0x300, 0x108, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_HSYNC__GPIO_1_22(pad_ctrl)		IOMUX_PAD(0x300, 0x108, 0x05, 0, 0, pad_ctrl)
#define MX25_PAD_HSYNC__I2C3_SCL(pad_ctrl)		IOMUX_PAD(0x300, 0x108, 0x02, 0x524, 0, pad_ctrl)

#define MX25_PAD_VSYNC__VSYNC(pad_ctrl)			IOMUX_PAD(0x304, 0x10c, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_VSYNC__GPIO_1_23(pad_ctrl)		IOMUX_PAD(0x304, 0x10c, 0x05, 0, 0, pad_ctrl)
#define MX25_PAD_VSYNC__I2C3_SDA(pad_ctrl)		IOMUX_PAD(0x304, 0x10c, 0x02, 0x528, 0, pad_ctrl)

#define MX25_PAD_LSCLK__LSCLK(pad_ctrl)			IOMUX_PAD(0x308, 0x110, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_LSCLK__GPIO_1_24(pad_ctrl)		IOMUX_PAD(0x308, 0x110, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_OE_ACD__OE_ACD(pad_ctrl)		IOMUX_PAD(0x30c, 0x114, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_OE_ACD__GPIO_1_25(pad_ctrl)		IOMUX_PAD(0x30c, 0x114, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_CONTRAST__CONTRAST(pad_ctrl)		IOMUX_PAD(0x310, 0x118, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_CONTRAST__FEC_CRS(pad_ctrl)		IOMUX_PAD(0x310, 0x118, 0x05, 0x508, 1, pad_ctrl)

#define MX25_PAD_PWM__PWM(pad_ctrl)			IOMUX_PAD(0x314, 0x11c, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_PWM__GPIO_1_26(pad_ctrl)		IOMUX_PAD(0x314, 0x11c, 0x05, 0, 0, pad_ctrl)
#define MX25_PAD_PWM__USBH2_OC(pad_ctrl)		IOMUX_PAD(0x314, 0x11c, 0x06, 0x580, 1, pad_ctrl)

#define MX25_PAD_CSI_D2__CSI_D2(pad_ctrl)		IOMUX_PAD(0x318, 0x120, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_D2__UART5_RXD(pad_ctrl)		IOMUX_PAD(0x318, 0x120, 0x01, 0x578, 1, pad_ctrl)
#define MX25_PAD_CSI_D2__GPIO_1_27(pad_ctrl)		IOMUX_PAD(0x318, 0x120, 0x05, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_D2__CSPI3_MOSI(pad_ctrl)		IOMUX_PAD(0x318, 0x120, 0x07, 0x4b8, 1, pad_ctrl)

#define MX25_PAD_CSI_D3__CSI_D3(pad_ctrl)		IOMUX_PAD(0x31c, 0x124, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_D3__UART5_TXD(pad_ctrl)		IOMUX_PAD(0x31c, 0x124, 0x01, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_D3__GPIO_1_28(pad_ctrl)		IOMUX_PAD(0x31c, 0x124, 0x05, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_D3__CSPI3_MISO(pad_ctrl)		IOMUX_PAD(0x31c, 0x124, 0x07, 0x4b4, 1, pad_ctrl)

#define MX25_PAD_CSI_D4__CSI_D4(pad_ctrl)		IOMUX_PAD(0x320, 0x128, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_D4__UART5_RTS(pad_ctrl)		IOMUX_PAD(0x320, 0x128, 0x01, 0x574, 1, pad_ctrl)
#define MX25_PAD_CSI_D4__GPIO_1_29(pad_ctrl)		IOMUX_PAD(0x320, 0x128, 0x05, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_D4__CSPI3_SCLK(pad_ctrl)		IOMUX_PAD(0x320, 0x128, 0x07, 0x4ac, 1, pad_ctrl)

#define MX25_PAD_CSI_D5__CSI_D5(pad_ctrl)		IOMUX_PAD(0x324, 0x12c, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_D5__UART5_CTS(pad_ctrl)		IOMUX_PAD(0x324, 0x12c, 0x01, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_D5__GPIO_1_30(pad_ctrl)		IOMUX_PAD(0x324, 0x12c, 0x05, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_D5__CSPI3_RDY(pad_ctrl)		IOMUX_PAD(0x324, 0x12c, 0x07, 0x4b0, 1, pad_ctrl)

#define MX25_PAD_CSI_D6__CSI_D6(pad_ctrl)		IOMUX_PAD(0x328, 0x130, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_D6__SD2_CMD(pad_ctrl)		IOMUX_PAD(0x328, 0x130, 0x12, 0x4e0, 1, pad_ctrl)
#define MX25_PAD_CSI_D6__GPIO_1_31(pad_ctrl)		IOMUX_PAD(0x328, 0x130, 0x05, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_D6__CSPI3_SS0(pad_ctrl)		IOMUX_PAD(0x328, 0x130, 0x07, 0x4bc, 1, pad_ctrl)

#define MX25_PAD_CSI_D7__CSI_D7(pad_ctrl)		IOMUX_PAD(0x32c, 0x134, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_D7__SD2_CLK(pad_ctrl)		IOMUX_PAD(0x32c, 0x134, 0x12, 0x4dc, 1, pad_ctrl)
#define MX25_PAD_CSI_D7__GPIO_1_6(pad_ctrl)		IOMUX_PAD(0x32c, 0x134, 0x05, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_D7__CSPI3_SS1(pad_ctrl)		IOMUX_PAD(0x32c, 0x134, 0x07, 0x4c0, 1, pad_ctrl)

#define MX25_PAD_CSI_D8__CSI_D8(pad_ctrl)		IOMUX_PAD(0x330, 0x138, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_D8__AUD6_RXC(pad_ctrl)		IOMUX_PAD(0x330, 0x138, 0x02, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_D8__GPIO_1_7(pad_ctrl)		IOMUX_PAD(0x330, 0x138, 0x05, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_D8__CSPI3_SS2(pad_ctrl)		IOMUX_PAD(0x330, 0x138, 0x07, 0x4c4, 0, pad_ctrl)

#define MX25_PAD_CSI_D9__CSI_D9(pad_ctrl)		IOMUX_PAD(0x334, 0x13c, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_D9__AUD6_RXFS(pad_ctrl)		IOMUX_PAD(0x334, 0x13c, 0x02, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_D9__GPIO_4_21(pad_ctrl)		IOMUX_PAD(0x334, 0x13c, 0x05, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_D9__CSPI3_SS3(pad_ctrl)		IOMUX_PAD(0x334, 0x13c, 0x07, 0x4c8, 0, pad_ctrl)

#define MX25_PAD_CSI_MCLK__CSI_MCLK(pad_ctrl)		IOMUX_PAD(0x338, 0x140, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_MCLK__AUD6_TXD(pad_ctrl)		IOMUX_PAD(0x338, 0x140, 0x01, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_MCLK__SD2_DATA0(pad_ctrl)		IOMUX_PAD(0x338, 0x140, 0x02, 0x4e4, 1, pad_ctrl)
#define MX25_PAD_CSI_MCLK__GPIO_1_8(pad_ctrl)		IOMUX_PAD(0x338, 0x140, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_CSI_VSYNC__CSI_VSYNC(pad_ctrl)		IOMUX_PAD(0x33c, 0x144, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_VSYNC__AUD6_RXD(pad_ctrl)		IOMUX_PAD(0x33c, 0x144, 0x01, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_VSYNC__SD2_DATA1(pad_ctrl)		IOMUX_PAD(0x33c, 0x144, 0x02, 0x4e8, 1, pad_ctrl)
#define MX25_PAD_CSI_VSYNC__GPIO_1_9(pad_ctrl)		IOMUX_PAD(0x33c, 0x144, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_CSI_HSYNC__CSI_HSYNC(pad_ctrl)		IOMUX_PAD(0x340, 0x148, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_HSYNC__AUD6_TXC(pad_ctrl)		IOMUX_PAD(0x340, 0x148, 0x01, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_HSYNC__SD2_DATA2(pad_ctrl)		IOMUX_PAD(0x340, 0x148, 0x02, 0x4ec, 1, pad_ctrl)
#define MX25_PAD_CSI_HSYNC__GPIO_1_10(pad_ctrl)		IOMUX_PAD(0x340, 0x148, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_CSI_PIXCLK__CSI_PIXCLK(pad_ctrl)	IOMUX_PAD(0x344, 0x14c, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_PIXCLK__AUD6_TXFS(pad_ctrl)	IOMUX_PAD(0x344, 0x14c, 0x01, 0, 0, pad_ctrl)
#define MX25_PAD_CSI_PIXCLK__SD2_DATA3(pad_ctrl)	IOMUX_PAD(0x344, 0x14c, 0x02, 0x4f0, 1, pad_ctrl)
#define MX25_PAD_CSI_PIXCLK__GPIO_1_11(pad_ctrl)	IOMUX_PAD(0x344, 0x14c, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_I2C1_CLK__I2C1_CLK(pad_ctrl)		IOMUX_PAD(0x348, 0x150, 0x10, 0, 0, pad_ctrl)
#define MX25_PAD_I2C1_CLK__GPIO_1_12(pad_ctrl)		IOMUX_PAD(0x348, 0x150, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_I2C1_DAT__I2C1_DAT(pad_ctrl)		IOMUX_PAD(0x34c, 0x154, 0x10, 0, 0, pad_ctrl)
#define MX25_PAD_I2C1_DAT__GPIO_1_13(pad_ctrl)		IOMUX_PAD(0x34c, 0x154, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_CSPI1_MOSI__CSPI1_MOSI(pad_ctrl)	IOMUX_PAD(0x350, 0x158, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_CSPI1_MOSI__UART3_RXD(pad_ctrl)	IOMUX_PAD(0x350, 0x158, 0x02, 0x568, 0, pad_ctrl)
#define MX25_PAD_CSPI1_MOSI__GPIO_1_14(pad_ctrl)	IOMUX_PAD(0x350, 0x158, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_CSPI1_MISO__CSPI1_MISO(pad_ctrl)	IOMUX_PAD(0x354, 0x15c, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_CSPI1_MISO__UART3_TXD(pad_ctrl)	IOMUX_PAD(0x354, 0x15c, 0x02, 0, 0, pad_ctrl)
#define MX25_PAD_CSPI1_MISO__GPIO_1_15(pad_ctrl)	IOMUX_PAD(0x354, 0x15c, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_CSPI1_SS0__CSPI1_SS0(pad_ctrl)		IOMUX_PAD(0x358, 0x160, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_CSPI1_SS0__PWM2(pad_ctrl)		IOMUX_PAD(0x358, 0x160, 0x02, 0, 0, pad_ctrl)
#define MX25_PAD_CSPI1_SS0__GPIO_1_16(pad_ctrl)		IOMUX_PAD(0x358, 0x160, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_CSPI1_SS1__CSPI1_SS1(pad_ctrl)		IOMUX_PAD(0x35c, 0x164, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_CSPI1_SS1__UART3_RTS(pad_ctrl)		IOMUX_PAD(0x35c, 0x164, 0x02, 0x564, 0, pad_ctrl)
#define MX25_PAD_CSPI1_SS1__GPIO_1_17(pad_ctrl)		IOMUX_PAD(0x35c, 0x164, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_CSPI1_SCLK__CSPI1_SCLK(pad_ctrl)	IOMUX_PAD(0x360, 0x168, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_CSPI1_SCLK__UART3_CTS(pad_ctrl)	IOMUX_PAD(0x360, 0x168, 0x02, 0, 0, pad_ctrl)
#define MX25_PAD_CSPI1_SCLK__GPIO_1_18(pad_ctrl)	IOMUX_PAD(0x360, 0x168, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_CSPI1_RDY__CSPI1_RDY(pad_ctrl)		IOMUX_PAD(0x364, 0x16c, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_CSPI1_RDY__GPIO_2_22(pad_ctrl)		IOMUX_PAD(0x364, 0x16c, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_UART1_RXD__UART1_RXD(pad_ctrl)		IOMUX_PAD(0x368, 0x170, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_UART1_RXD__UART2_DTR(pad_ctrl)		IOMUX_PAD(0x368, 0x170, 0x03, 0, 0, pad_ctrl)
#define MX25_PAD_UART1_RXD__GPIO_4_22(pad_ctrl)		IOMUX_PAD(0x368, 0x170, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_UART1_TXD__UART1_TXD(pad_ctrl)		IOMUX_PAD(0x36c, 0x174, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_UART1_TXD__UART2_DSR(pad_ctrl)		IOMUX_PAD(0x36c, 0x174, 0x03, 0, 0, pad_ctrl)
#define MX25_PAD_UART1_TXD__GPIO_4_23(pad_ctrl)		IOMUX_PAD(0x36c, 0x174, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_UART1_RTS__UART1_RTS(pad_ctrl)		IOMUX_PAD(0x370, 0x178, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_UART1_RTS__CSI_D0(pad_ctrl)		IOMUX_PAD(0x370, 0x178, 0x01, 0x488, 1, pad_ctrl)
#define MX25_PAD_UART1_RTS__UART2_DCD(pad_ctrl)		IOMUX_PAD(0x370, 0x178, 0x03, 0, 0, pad_ctrl)
#define MX25_PAD_UART1_RTS__GPIO_4_24(pad_ctrl)		IOMUX_PAD(0x370, 0x178, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_UART1_CTS__UART1_CTS(pad_ctrl)		IOMUX_PAD(0x374, 0x17c, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_UART1_CTS__CSI_D1(pad_ctrl)		IOMUX_PAD(0x374, 0x17c, 0x01, 0x48c, 1, pad_ctrl)
#define MX25_PAD_UART1_CTS__UART2_RI(pad_ctrl)		IOMUX_PAD(0x374, 0x17c, 0x03, 0, 0, pad_ctrl)
#define MX25_PAD_UART1_CTS__GPIO_4_25(pad_ctrl)		IOMUX_PAD(0x374, 0x17c, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_UART2_RXD__UART2_RXD(pad_ctrl)		IOMUX_PAD(0x378, 0x180, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_UART2_RXD__GPIO_4_26(pad_ctrl)		IOMUX_PAD(0x378, 0x180, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_UART2_TXD__UART2_TXD(pad_ctrl)		IOMUX_PAD(0x37c, 0x184, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_UART2_TXD__GPIO_4_27(pad_ctrl)		IOMUX_PAD(0x37c, 0x184, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_UART2_RTS__UART2_RTS(pad_ctrl)		IOMUX_PAD(0x380, 0x188, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_UART2_RTS__FEC_COL(pad_ctrl)		IOMUX_PAD(0x380, 0x188, 0x02, 0x504, 2, pad_ctrl)
#define MX25_PAD_UART2_RTS__GPIO_4_28(pad_ctrl)		IOMUX_PAD(0x380, 0x188, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_UART2_CTS__FEC_RX_ER(pad_ctrl)		IOMUX_PAD(0x384, 0x18c, 0x02, 0x518, 2, pad_ctrl)
#define MX25_PAD_UART2_CTS__UART2_CTS(pad_ctrl)		IOMUX_PAD(0x384, 0x18c, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_UART2_CTS__GPIO_4_29(pad_ctrl)		IOMUX_PAD(0x384, 0x18c, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_SD1_CMD__SD1_CMD(pad_ctrl)		IOMUX_PAD(0x388, 0x190, 0x10, 0, 0, pad_ctrl)
#define MX25_PAD_SD1_CMD__FEC_RDATA2(pad_ctrl)		IOMUX_PAD(0x388, 0x190, 0x02, 0x50c, 2, pad_ctrl)
#define MX25_PAD_SD1_CMD__GPIO_2_23(pad_ctrl)		IOMUX_PAD(0x388, 0x190, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_SD1_CLK__SD1_CLK(pad_ctrl)		IOMUX_PAD(0x38c, 0x194, 0x10, 0, 0, pad_ctrl)
#define MX25_PAD_SD1_CLK__FEC_RDATA3(pad_ctrl)		IOMUX_PAD(0x38c, 0x194, 0x02, 0x510, 2, pad_ctrl)
#define MX25_PAD_SD1_CLK__GPIO_2_24(pad_ctrl)		IOMUX_PAD(0x38c, 0x194, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_SD1_DATA0__SD1_DATA0(pad_ctrl)		IOMUX_PAD(0x390, 0x198, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_SD1_DATA0__GPIO_2_25(pad_ctrl)		IOMUX_PAD(0x390, 0x198, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_SD1_DATA1__SD1_DATA1(pad_ctrl)		IOMUX_PAD(0x394, 0x19c, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_SD1_DATA1__AUD7_RXD(pad_ctrl)		IOMUX_PAD(0x394, 0x19c, 0x03, 0x478, 0, pad_ctrl)
#define MX25_PAD_SD1_DATA1__GPIO_2_26(pad_ctrl)		IOMUX_PAD(0x394, 0x19c, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_SD1_DATA2__SD1_DATA2(pad_ctrl)		IOMUX_PAD(0x398, 0x1a0, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_SD1_DATA2__FEC_RX_CLK(pad_ctrl)	IOMUX_PAD(0x398, 0x1a0, 0x05, 0x514, 2, pad_ctrl)
#define MX25_PAD_SD1_DATA2__GPIO_2_27(pad_ctrl)		IOMUX_PAD(0x398, 0x1a0, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_SD1_DATA3__SD1_DATA3(pad_ctrl)		IOMUX_PAD(0x39c, 0x1a4, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_SD1_DATA3__FEC_CRS(pad_ctrl)		IOMUX_PAD(0x39c, 0x1a4, 0x00, 0x508, 2, pad_ctrl)
#define MX25_PAD_SD1_DATA3__GPIO_2_28(pad_ctrl)		IOMUX_PAD(0x39c, 0x1a4, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_KPP_ROW0__KPP_ROW0(pad_ctrl)		IOMUX_PAD(0x3a0, 0x1a8, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_KPP_ROW0__UART3_RXD(pad_ctrl)		IOMUX_PAD(0x3a0, 0x1a8, 0x01, 0x568, 1, pad_ctrl)
#define MX25_PAD_KPP_ROW0__GPIO_2_29(pad_ctrl)		IOMUX_PAD(0x3a0, 0x1a8, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_KPP_ROW1__KPP_ROW1(pad_ctrl)		IOMUX_PAD(0x3a4, 0x1ac, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_KPP_ROW1__UART3_TXD(pad_ctrl)		IOMUX_PAD(0x3a4, 0x1ac, 0x01, 0, 0, pad_ctrl)
#define MX25_PAD_KPP_ROW1__GPIO_2_30(pad_ctrl)		IOMUX_PAD(0x3a4, 0x1ac, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_KPP_ROW2__KPP_ROW2(pad_ctrl)		IOMUX_PAD(0x3a8, 0x1b0, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_KPP_ROW2__UART3_RTS(pad_ctrl)		IOMUX_PAD(0x3a8, 0x1b0, 0x01, 0x564, 1, pad_ctrl)
#define MX25_PAD_KPP_ROW2__AUD5_RXC(pad_ctrl)		IOMUX_PAD(0x3a8, 0x1b0, 0x02, 0, 0, pad_ctrl)
#define MX25_PAD_KPP_ROW2__CSI_D0(pad_ctrl)		IOMUX_PAD(0x3a8, 0x1b0, 0x03, 0x488, 2, pad_ctrl)
#define MX25_PAD_KPP_ROW2__GPIO_2_31(pad_ctrl)		IOMUX_PAD(0x3a8, 0x1b0, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_KPP_ROW3__KPP_ROW3(pad_ctrl)		IOMUX_PAD(0x3ac, 0x1b4, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_KPP_ROW3__UART3_CTS(pad_ctrl)		IOMUX_PAD(0x3ac, 0x1b4, 0x01, 0, 0, pad_ctrl)
#define MX25_PAD_KPP_ROW3__AUD5_RXFS(pad_ctrl)		IOMUX_PAD(0x3ac, 0x1b4, 0x02, 0, 0, pad_ctrl)
#define MX25_PAD_KPP_ROW3__CSI_LD1(pad_ctrl)		IOMUX_PAD(0x3ac, 0x1b4, 0x03, 0x48c, 2, pad_ctrl)
#define MX25_PAD_KPP_ROW3__GPIO_3_0(pad_ctrl)		IOMUX_PAD(0x3ac, 0x1b4, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_KPP_COL0__KPP_COL0(pad_ctrl)		IOMUX_PAD(0x3b0, 0x1b8, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_KPP_COL0__UART4_RXD(pad_ctrl)		IOMUX_PAD(0x3b0, 0x1b8, 0x01, 0x570, 1, pad_ctrl)
#define MX25_PAD_KPP_COL0__AUD5_TXD(pad_ctrl)		IOMUX_PAD(0x3b0, 0x1b8, 0x02, 0, 0, pad_ctrl)
#define MX25_PAD_KPP_COL0__GPIO_3_1(pad_ctrl)		IOMUX_PAD(0x3b0, 0x1b8, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_KPP_COL1__KPP_COL1(pad_ctrl)		IOMUX_PAD(0x3b4, 0x1bc, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_KPP_COL1__UART4_TXD(pad_ctrl)		IOMUX_PAD(0x3b4, 0x1bc, 0x01, 0, 0, pad_ctrl)
#define MX25_PAD_KPP_COL1__AUD5_RXD(pad_ctrl)		IOMUX_PAD(0x3b4, 0x1bc, 0x02, 0, 0, pad_ctrl)
#define MX25_PAD_KPP_COL1__GPIO_3_2(pad_ctrl)		IOMUX_PAD(0x3b4, 0x1bc, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_KPP_COL2__KPP_COL2(pad_ctrl)		IOMUX_PAD(0x3b8, 0x1c0, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_KPP_COL2__UART4_RTS(pad_ctrl)		IOMUX_PAD(0x3b8, 0x1c0, 0x01, 0x56c, 1, pad_ctrl)
#define MX25_PAD_KPP_COL2__AUD5_TXC(pad_ctrl)		IOMUX_PAD(0x3b8, 0x1c0, 0x02, 0, 0, pad_ctrl)
#define MX25_PAD_KPP_COL2__GPIO_3_3(pad_ctrl)		IOMUX_PAD(0x3b8, 0x1c0, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_KPP_COL3__KPP_COL3(pad_ctrl)		IOMUX_PAD(0x3bc, 0x1c4, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_KPP_COL3__UART4_CTS(pad_ctrl)		IOMUX_PAD(0x3bc, 0x1c4, 0x01, 0, 0, pad_ctrl)
#define MX25_PAD_KPP_COL3__AUD5_TXFS(pad_ctrl)		IOMUX_PAD(0x3bc, 0x1c4, 0x02, 0, 0, pad_ctrl)
#define MX25_PAD_KPP_COL3__GPIO_3_4(pad_ctrl)		IOMUX_PAD(0x3bc, 0x1c4, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_FEC_MDC__FEC_MDC(pad_ctrl)		IOMUX_PAD(0x3c0, 0x1c8, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_FEC_MDC__AUD4_TXD(pad_ctrl)		IOMUX_PAD(0x3c0, 0x1c8, 0x02, 0x464, 1, pad_ctrl)
#define MX25_PAD_FEC_MDC__GPIO_3_5(pad_ctrl)		IOMUX_PAD(0x3c0, 0x1c8, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_FEC_MDIO__FEC_MDIO(pad_ctrl)		IOMUX_PAD(0x3c4, 0x1cc, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_FEC_MDIO__AUD4_RXD(pad_ctrl)		IOMUX_PAD(0x3c4, 0x1cc, 0x02, 0x460, 1, pad_ctrl)
#define MX25_PAD_FEC_MDIO__GPIO_3_6(pad_ctrl)		IOMUX_PAD(0x3c4, 0x1cc, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_FEC_TDATA0__FEC_TDATA0(pad_ctrl)	IOMUX_PAD(0x3c8, 0x1d0, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_FEC_TDATA0__GPIO_3_7(pad_ctrl)		IOMUX_PAD(0x3c8, 0x1d0, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_FEC_TDATA1__FEC_TDATA1(pad_ctrl)	IOMUX_PAD(0x3cc, 0x1d4, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_FEC_TDATA1__AUD4_TXFS(pad_ctrl)	IOMUX_PAD(0x3cc, 0x1d4, 0x02, 0x474, 1, pad_ctrl)
#define MX25_PAD_FEC_TDATA1__GPIO_3_8(pad_ctrl)		IOMUX_PAD(0x3cc, 0x1d4, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_FEC_TX_EN__FEC_TX_EN(pad_ctrl)		IOMUX_PAD(0x3d0, 0x1d8, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_FEC_TX_EN__GPIO_3_9(pad_ctrl)		IOMUX_PAD(0x3d0, 0x1d8, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_FEC_RDATA0__FEC_RDATA0(pad_ctrl)	IOMUX_PAD(0x3d4, 0x1dc, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_FEC_RDATA0__GPIO_3_10(pad_ctrl)	IOMUX_PAD(0x3d4, 0x1dc, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_FEC_RDATA1__FEC_RDATA1(pad_ctrl)	IOMUX_PAD(0x3d8, 0x1e0, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_FEC_RDATA1__GPIO_3_11(pad_ctrl)	IOMUX_PAD(0x3d8, 0x1e0, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_FEC_RX_DV__FEC_RX_DV(pad_ctrl)		IOMUX_PAD(0x3dc, 0x1e4, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_FEC_RX_DV__CAN2_RX(pad_ctrl)		IOMUX_PAD(0x3dc, 0x1e4, 0x04, 0x484, 0, pad_ctrl)
#define MX25_PAD_FEC_RX_DV__GPIO_3_12(pad_ctrl)		IOMUX_PAD(0x3dc, 0x1e4, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_FEC_TX_CLK__FEC_TX_CLK(pad_ctrl)	IOMUX_PAD(0x3e0, 0x1e8, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_FEC_TX_CLK__GPIO_3_13(pad_ctrl)	IOMUX_PAD(0x3e0, 0x1e8, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_RTCK__RTCK(pad_ctrl)			IOMUX_PAD(0x3e4, 0x1ec, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_RTCK__OWIRE(pad_ctrl)			IOMUX_PAD(0x3e4, 0x1ec, 0x11, 0, 0, pad_ctrl)
#define MX25_PAD_RTCK__GPIO_3_14(pad_ctrl)		IOMUX_PAD(0x3e4, 0x1ec, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_DE_B__DE_B(pad_ctrl)			IOMUX_PAD(0x3ec, 0x1f0, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_DE_B__GPIO_2_20(pad_ctrl)		IOMUX_PAD(0x3ec, 0x1f0, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_TDO__TDO(pad_ctrl)			IOMUX_PAD(0x3e8, 0x000, 0x00, 0, 0, pad_ctrl)

#define MX25_PAD_GPIO_A__GPIO_A(pad_ctrl)		IOMUX_PAD(0x3f0, 0x1f4, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_GPIO_A__USBOTG_PWR(pad_ctrl)		IOMUX_PAD(0x3f0, 0x1f4, 0x02, 0, 0, pad_ctrl)
#define MX25_PAD_GPIO_A__KPP_ROW4(pad_ctrl)		IOMUX_PAD(0x3f0, 0x1f4, 0x03, 0x53c, 1, pad_ctrl)
#define MX25_PAD_GPIO_A__I2C3_SCL(pad_ctrl)		IOMUX_PAD(0x3f0, 0x1f4, 0x14, 0x524, 1, pad_ctrl)
#define MX25_PAD_GPIO_A__CAN1_TX(pad_ctrl)		IOMUX_PAD(0x3f0, 0x1f4, 0x06, 0, 0, pad_ctrl)
#define MX25_PAD_GPIO_A__PWM2(pad_ctrl)		IOMUX_PAD(0x3f0, 0x1f4, 0x01, 0, 0, pad_ctrl)

#define MX25_PAD_GPIO_B__GPIO_B(pad_ctrl)		IOMUX_PAD(0x3f4, 0x1f8, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_GPIO_B__USBOTG_OC(pad_ctrl)		IOMUX_PAD(0x3f4, 0x1f8, 0x02, 0x57c, 1, pad_ctrl)
#define MX25_PAD_GPIO_B__KPP_ROW5(pad_ctrl)		IOMUX_PAD(0x3f4, 0x1f8, 0x03, 0x540, 1, pad_ctrl)
#define MX25_PAD_GPIO_B__I2C3_SDA(pad_ctrl)		IOMUX_PAD(0x3f4, 0x1f8, 0x14, 0x528, 2, pad_ctrl)
#define MX25_PAD_GPIO_B__CAN1_RX(pad_ctrl)		IOMUX_PAD(0x3f4, 0x1f8, 0x06, 0x480, 1, pad_ctrl)
#define MX25_PAD_GPIO_B__PWM3(pad_ctrl)		IOMUX_PAD(0x3f4, 0x1f8, 0x01, 0, 0, pad_ctrl)

#define MX25_PAD_GPIO_C__GPIO_C(pad_ctrl)		IOMUX_PAD(0x3f8, 0x1fc, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_GPIO_C__PWM4(pad_ctrl)			IOMUX_PAD(0x3f8, 0x1fc, 0x01, 0, 0, pad_ctrl)
#define MX25_PAD_GPIO_C__I2C2_SCL(pad_ctrl)		IOMUX_PAD(0x3f8, 0x1fc, 0x12, 0x51c, 1, pad_ctrl)
#define MX25_PAD_GPIO_C__CAN2_TX(pad_ctrl)		IOMUX_PAD(0x3f8, 0x1fc, 0x06, 0, 0, pad_ctrl)

#define MX25_PAD_GPIO_D__GPIO_D(pad_ctrl)		IOMUX_PAD(0x3fc, 0x200, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_GPIO_D__I2C2_SDA(pad_ctrl)		IOMUX_PAD(0x3fc, 0x200, 0x12, 0x520, 1, pad_ctrl)
#define MX25_PAD_GPIO_D__CAN2_RX(pad_ctrl)		IOMUX_PAD(0x3fc, 0x200, 0x06, 0x484, 1, pad_ctrl)

#define MX25_PAD_GPIO_E__GPIO_E(pad_ctrl)		IOMUX_PAD(0x400, 0x204, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_GPIO_E__LD16(pad_ctrl)			IOMUX_PAD(0x400, 0x204, 0x02, 0, 0, pad_ctrl)
#define MX25_PAD_GPIO_E__AUD7_TXD(pad_ctrl)		IOMUX_PAD(0x400, 0x204, 0x04, 0, 0, pad_ctrl)

#define MX25_PAD_GPIO_F__GPIO_F(pad_ctrl)		IOMUX_PAD(0x404, 0x208, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_GPIO_F__LD17(pad_ctrl)			IOMUX_PAD(0x404, 0x208, 0x02, 0, 0, pad_ctrl)
#define MX25_PAD_GPIO_F__AUD7_TXC(pad_ctrl)		IOMUX_PAD(0x404, 0x208, 0x04, 0, 0, pad_ctrl)

#define MX25_PAD_EXT_ARMCLK__EXT_ARMCLK			IOMUX_PAD(0x000, 0x20c, 0x00, 0, 0, NO_PAD_CTRL)
#define MX25_PAD_EXT_ARMCLK__GPIO_3_15			IOMUX_PAD(0x000, 0x20c, 0x05, 0, 0, NO_PAD_CTRL)

#define MX25_PAD_UPLL_BYPCLK__UPLL_BYPCLK		IOMUX_PAD(0x000, 0x210, 0x00, 0, 0, NO_PAD_CTRL)
#define MX25_PAD_UPLL_BYPCLK__GPIO_3_16			IOMUX_PAD(0x000, 0x210, 0x05, 0, 0, NO_PAD_CTRL)

#define MX25_PAD_VSTBY_REQ__VSTBY_REQ(pad_ctrl)		IOMUX_PAD(0x408, 0x214, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_VSTBY_REQ__AUD7_TXFS(pad_ctrl)		IOMUX_PAD(0x408, 0x214, 0x04, 0, 0, pad_ctrl)
#define MX25_PAD_VSTBY_REQ__GPIO_3_17(pad_ctrl)		IOMUX_PAD(0x408, 0x214, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_VSTBY_ACK__VSTBY_ACK(pad_ctrl)		IOMUX_PAD(0x40c, 0x218, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_VSTBY_ACK__GPIO_3_18(pad_ctrl)		IOMUX_PAD(0x40c, 0x218, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_POWER_FAIL__POWER_FAIL(pad_ctrl)	IOMUX_PAD(0x410, 0x21c, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_POWER_FAIL__AUD7_RXD(pad_ctrl)		IOMUX_PAD(0x410, 0x21c, 0x04, 0x478, 1, pad_ctrl)
#define MX25_PAD_POWER_FAIL__GPIO_3_19(pad_ctrl)	IOMUX_PAD(0x410, 0x21c, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_CLKO__CLKO(pad_ctrl)			IOMUX_PAD(0x414, 0x220, 0x00, 0, 0, pad_ctrl)
#define MX25_PAD_CLKO__GPIO_2_21(pad_ctrl)		IOMUX_PAD(0x414, 0x220, 0x05, 0, 0, pad_ctrl)

#define MX25_PAD_BOOT_MODE0__BOOT_MODE0			IOMUX_PAD(0x000, 0x224, 0x00, 0, 0, NO_PAD_CTRL)
#define MX25_PAD_BOOT_MODE0__GPIO_4_30			IOMUX_PAD(0x000, 0x224, 0x05, 0, 0, NO_PAD_CTRL)
#define MX25_PAD_BOOT_MODE1__BOOT_MODE1			IOMUX_PAD(0x000, 0x228, 0x00, 0, 0, NO_PAD_CTRL)
#define MX25_PAD_BOOT_MODE1__GPIO_4_31			IOMUX_PAD(0x000, 0x228, 0x05, 0, 0, NO_PAD_CTRL)

#define MX25_PAD_CTL_GRP_DVS_MISC(pad_ctrl)		IOMUX_PAD(0x418, 0x000, 0, 0, 0, pad_ctrl)
#define MX25_PAD_CTL_GRP_DSE_FEC(pad_ctrl)		IOMUX_PAD(0x41c, 0x000, 0, 0, 0, pad_ctrl)
#define MX25_PAD_CTL_GRP_DVS_JTAG(pad_ctrl)		IOMUX_PAD(0x420, 0x000, 0, 0, 0, pad_ctrl)
#define MX25_PAD_CTL_GRP_DSE_NFC(pad_ctrl)		IOMUX_PAD(0x424, 0x000, 0, 0, 0, pad_ctrl)
#define MX25_PAD_CTL_GRP_DSE_CSI(pad_ctrl)		IOMUX_PAD(0x428, 0x000, 0, 0, 0, pad_ctrl)
#define MX25_PAD_CTL_GRP_DSE_WEIM(pad_ctrl)		IOMUX_PAD(0x42c, 0x000, 0, 0, 0, pad_ctrl)
#define MX25_PAD_CTL_GRP_DSE_DDR(pad_ctrl)		IOMUX_PAD(0x430, 0x000, 0, 0, 0, pad_ctrl)
#define MX25_PAD_CTL_GRP_DVS_CRM(pad_ctrl)		IOMUX_PAD(0x434, 0x000, 0, 0, 0, pad_ctrl)
#define MX25_PAD_CTL_GRP_DSE_KPP(pad_ctrl)		IOMUX_PAD(0x438, 0x000, 0, 0, 0, pad_ctrl)
#define MX25_PAD_CTL_GRP_DSE_SDHC1(pad_ctrl)		IOMUX_PAD(0x43c, 0x000, 0, 0, 0, pad_ctrl)
#define MX25_PAD_CTL_GRP_DSE_LCD(pad_ctrl)		IOMUX_PAD(0x440, 0x000, 0, 0, 0, pad_ctrl)
#define MX25_PAD_CTL_GRP_DSE_UART(pad_ctrl)		IOMUX_PAD(0x444, 0x000, 0, 0, 0, pad_ctrl)
#define MX25_PAD_CTL_GRP_DVS_NFC(pad_ctrl)		IOMUX_PAD(0x448, 0x000, 0, 0, 0, pad_ctrl)
#define MX25_PAD_CTL_GRP_DVS_CSI(pad_ctrl)		IOMUX_PAD(0x44c, 0x000, 0, 0, 0, pad_ctrl)
#define MX25_PAD_CTL_GRP_DSE_CSPI1(pad_ctrl)		IOMUX_PAD(0x450, 0x000, 0, 0, 0, pad_ctrl)
#define MX25_PAD_CTL_GRP_DDRTYPE(pad_ctrl)		IOMUX_PAD(0x454, 0x000, 0, 0, 0, pad_ctrl)
#define MX25_PAD_CTL_GRP_DVS_SDHC1(pad_ctrl)		IOMUX_PAD(0x458, 0x000, 0, 0, 0, pad_ctrl)
#define MX25_PAD_CTL_GRP_DVS_LCD(pad_ctrl)		IOMUX_PAD(0x45c, 0x000, 0, 0, 0, pad_ctrl)

#endif // __ASSEMBLY__
#endif // __IOMUX_MX25_H__
