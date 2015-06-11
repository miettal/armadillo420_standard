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

#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <asm/mach-types.h>
#include <asm/arch/iomux-mx25.h>

#include "board.h"

static struct pad_desc armadillo400_gpio_led_pads[] = {
	MX25_PAD_NFALE__GPIO_3_28, /* red led */
	MX25_PAD_NFCLE__GPIO_3_29, /* green led */
	MX25_PAD_BOOT_MODE0__GPIO_4_30, /* yellow led */
};

static struct pad_desc armadillo400_gpio_keys_pads[] = {
#if !defined(CONFIG_ARMADILLO400_SELECT_MUX_AS_A410)
	/* SW1 */
	MX25_PAD_NFWP_B__GPIO_3_30,
#if defined(CONFIG_MACH_ARMADILLO410) || defined(CONFIG_MACH_ARMADILLO440) || defined(CONFIG_MACH_ARMADILLO460)
	/* EXT_GPIO18 */
	MX25_PAD_DE_B__GPIO_2_20(0), /* 47K_UP is always on */
	/* EXT_GPIO19 */
#if defined(CONFIG_ARMADILLO400_CON11_40_GPIO_2_29)
	MX25_PAD_KPP_ROW0__GPIO_2_29(PAD_CTL_PKE | PAD_CTL_PUS_47K_UP),
#endif
	/* EXT_GPIO20 */
#if defined(CONFIG_ARMADILLO400_CON11_41_GPIO_2_30)
	MX25_PAD_KPP_ROW1__GPIO_2_30(PAD_CTL_PKE | PAD_CTL_PUS_47K_UP),
#endif
#endif /* CONFIG_MACH_ARMADILLO410 || CONFIG_MACH_ARMADILLO440 || CONFIG_MACH_ARMADILLO460 */
#endif /* CONFIG_ARMADILLO400_SELECT_MUX_AS_A410 */
};

static struct pad_desc armadillo400_ext_gpio_pads_revb[] = {
	/* EXT_GPIO0 */
#if defined(CONFIG_ARMADILLO400_CON9_21_GPIO1_8)
	MX25_PAD_CSI_MCLK__GPIO_1_8(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_22_GPIO1_9)
	MX25_PAD_CSI_VSYNC__GPIO_1_9(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_23_GPIO1_10)
	MX25_PAD_CSI_HSYNC__GPIO_1_10(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_24_GPIO1_11)
	MX25_PAD_CSI_PIXCLK__GPIO_1_11(PAD_CTL_HYS | PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_25_GPIO1_16)
	MX25_PAD_CSPI1_SS0__GPIO_1_16(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
	/* EXT_GPIO5 */
#if defined(CONFIG_ARMADILLO400_CON9_26_GPIO)
	MX25_PAD_CSPI1_RDY__GPIO_2_22(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#elif defined(CONFIG_ARMADILLO400_CON9_26_W1)
	MX25_PAD_CSPI1_RDY__GPIO_2_22(PAD_CTL_PKE | PAD_CTL_PUS_22K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_27_GPIO2_21)
	MX25_PAD_CLKO__GPIO_2_21(0),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_28_GPIO3_15)
	MX25_PAD_EXT_ARMCLK__GPIO_3_15,
#endif
#if defined(CONFIG_ARMADILLO400_CON9_11_GPIO1_17)
	MX25_PAD_CSPI1_SS1__GPIO_1_17(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_12_GPIO1_29)
	MX25_PAD_CSI_D4__GPIO_1_29(PAD_CTL_HYS | PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
	/* EXT_GPIO10 */
#if defined(CONFIG_ARMADILLO400_CON9_13_GPIO1_18)
	MX25_PAD_CSPI1_SCLK__GPIO_1_18(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_14_GPIO1_30)
	MX25_PAD_CSI_D5__GPIO_1_30(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_15_GPIO1_7)
	MX25_PAD_CSI_D8__GPIO_1_7(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_16_GPIO1_31)
	MX25_PAD_CSI_D6__GPIO_1_31(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_17_GPIO4_21)
	MX25_PAD_CSI_D9__GPIO_4_21(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
	/* EXT_GPIO15 */
#if defined(CONFIG_ARMADILLO400_CON9_18_GPIO1_6)
	MX25_PAD_CSI_D7__GPIO_1_6(PAD_CTL_HYS | PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON14_3_GPIO1_2)
	MX25_PAD_GPIO_C__GPIO_C(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON14_4_GPIO1_3)
	MX25_PAD_GPIO_D__GPIO_D(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_42_GPIO_2_31)
	MX25_PAD_KPP_ROW2__GPIO_2_31(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_43_GPIO_3_0)
	MX25_PAD_KPP_ROW3__GPIO_3_0(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_44_GPIO_3_1)
	MX25_PAD_KPP_COL0__GPIO_3_1(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_45_GPIO_3_2)
	MX25_PAD_KPP_COL1__GPIO_3_2(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_46_GPIO_3_3)
	MX25_PAD_KPP_COL2__GPIO_3_3(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_47_GPIO_3_4)
	MX25_PAD_KPP_COL3__GPIO_3_4(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_48_GPIO1_0)
	MX25_PAD_GPIO_A__GPIO_A(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_49_GPIO1_1)
	MX25_PAD_GPIO_B__GPIO_B(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_3_GPIO1_14)
	MX25_PAD_CSPI1_MOSI__GPIO_1_14(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_4_GPIO1_27)
	MX25_PAD_CSI_D2__GPIO_1_27(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_5_GPIO1_15)
	MX25_PAD_CSPI1_MISO__GPIO_1_15(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_6_GPIO1_28)
	MX25_PAD_CSI_D3__GPIO_1_28(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
};

static struct pad_desc armadillo400_ext_gpio_pads_revc[] = {
	/* EXT_GPIO0 */
#if defined(CONFIG_ARMADILLO400_CON9_21_GPIO1_8) || defined(CONFIG_ARMADILLO410_CON2_73_GPIO1_8)
	MX25_PAD_CSI_MCLK__GPIO_1_8(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_22_GPIO1_9) || defined(CONFIG_ARMADILLO410_CON2_72_GPIO1_9)
	MX25_PAD_CSI_VSYNC__GPIO_1_9(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_23_GPIO1_10) || defined(CONFIG_ARMADILLO410_CON2_71_GPIO1_10)
	MX25_PAD_CSI_HSYNC__GPIO_1_10(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_24_GPIO1_11) || defined(CONFIG_ARMADILLO410_CON2_70_GPIO1_11)
	MX25_PAD_CSI_PIXCLK__GPIO_1_11(PAD_CTL_HYS | PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_25_GPIO1_16) || defined(CONFIG_ARMADILLO410_CON2_69_GPIO1_16)
	MX25_PAD_CSPI1_SS0__GPIO_1_16(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
	/* EXT_GPIO5 */
#if defined(CONFIG_ARMADILLO400_CON9_26_GPIO) || defined(CONFIG_ARMADILLO410_CON2_68_GPIO)
	MX25_PAD_CSPI1_RDY__GPIO_2_22(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#elif defined(CONFIG_ARMADILLO400_CON9_26_W1) || defined(CONFIG_ARMADILLO410_CON2_68_W1)
	MX25_PAD_CSPI1_RDY__GPIO_2_22(PAD_CTL_PKE | PAD_CTL_PUS_22K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_27_GPIO2_21) || defined(CONFIG_ARMADILLO410_CON2_67_GPIO2_21)
	MX25_PAD_CLKO__GPIO_2_21(0), /* no pull up */
#endif
#if defined(CONFIG_ARMADILLO400_CON9_28_GPIO3_15) || defined(CONFIG_ARMADILLO410_CON2_66_GPIO3_15)
	MX25_PAD_EXT_ARMCLK__GPIO_3_15, /* no pull up */
#endif
#if defined(CONFIG_ARMADILLO400_CON9_11_GPIO1_17) || defined(CONFIG_ARMADILLO410_CON2_81_GPIO1_17)
	MX25_PAD_CSPI1_SS1__GPIO_1_17(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_12_GPIO1_29) || defined(CONFIG_ARMADILLO410_CON2_80_GPIO1_29)
	MX25_PAD_CSI_D4__GPIO_1_29(PAD_CTL_HYS | PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
	/* EXT_GPIO10 */
#if defined(CONFIG_ARMADILLO400_CON9_13_GPIO1_18) || defined(CONFIG_ARMADILLO410_CON2_79_GPIO1_18)
	MX25_PAD_CSPI1_SCLK__GPIO_1_18(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_14_GPIO1_30) || defined(CONFIG_ARMADILLO410_CON2_78_GPIO1_30)
	MX25_PAD_CSI_D5__GPIO_1_30(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_15_GPIO1_7) || defined(CONFIG_ARMADILLO410_CON2_77_GPIO1_7)
	MX25_PAD_CSI_D8__GPIO_1_7(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_16_GPIO1_31) || defined(CONFIG_ARMADILLO410_CON2_76_GPIO1_31)
	MX25_PAD_CSI_D6__GPIO_1_31(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_17_GPIO4_21) || defined(CONFIG_ARMADILLO410_CON2_75_GPIO4_21)
	MX25_PAD_CSI_D9__GPIO_4_21(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
	/* EXT_GPIO15 */
#if defined(CONFIG_ARMADILLO400_CON9_18_GPIO1_6) || defined(CONFIG_ARMADILLO410_CON2_74_GPIO1_6) 
	MX25_PAD_CSI_D7__GPIO_1_6(PAD_CTL_HYS | PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_1_GPIO3_17) || defined(CONFIG_ARMADILLO410_CON2_87_GPIO3_17) || defined(CONFIG_ARMADILLO400_CON9_1_SDHC2_PWREN) || defined(CONFIG_ARMADILLO410_CON2_87_SDHC2_PWREN)
	MX25_PAD_VSTBY_REQ__GPIO_3_17(PAD_CTL_PKE),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_2_GPIO3_14) || defined(CONFIG_ARMADILLO410_CON2_86_GPIO3_14)
	MX25_PAD_RTCK__GPIO_3_14(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_42_GPIO_2_31) || defined(CONFIG_ARMADILLO410_CON2_60_GPIO_2_31)
	MX25_PAD_KPP_ROW2__GPIO_2_31(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_43_GPIO_3_0) || defined(CONFIG_ARMADILLO410_CON2_59_GPIO_3_0)
	MX25_PAD_KPP_ROW3__GPIO_3_0(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_44_GPIO_3_1) || defined(CONFIG_ARMADILLO410_CON2_58_GPIO_3_1)
	MX25_PAD_KPP_COL0__GPIO_3_1(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_45_GPIO_3_2) || defined(CONFIG_ARMADILLO410_CON2_57_GPIO_3_2)
	MX25_PAD_KPP_COL1__GPIO_3_2(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_46_GPIO_3_3) || defined(CONFIG_ARMADILLO410_CON2_56_GPIO_3_3)
	MX25_PAD_KPP_COL2__GPIO_3_3(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_47_GPIO_3_4) || defined(CONFIG_ARMADILLO410_CON2_55_GPIO_3_4)
	MX25_PAD_KPP_COL3__GPIO_3_4(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_48_GPIO1_0) || defined(CONFIG_ARMADILLO410_CON2_54_GPIO1_0)
	MX25_PAD_GPIO_A__GPIO_A(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_49_GPIO1_1) || defined(CONFIG_ARMADILLO410_CON2_53_GPIO1_1)
	MX25_PAD_GPIO_B__GPIO_B(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON14_3_GPIO1_2) || defined(CONFIG_ARMADILLO410_CON2_65_GPIO1_2)
	MX25_PAD_GPIO_C__GPIO_C(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON14_4_GPIO1_3) || defined(CONFIG_ARMADILLO410_CON2_64_GPIO1_3)
	MX25_PAD_GPIO_D__GPIO_D(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_3_GPIO1_14) || defined(CONFIG_ARMADILLO410_CON2_85_GPIO1_14)
	MX25_PAD_CSPI1_MOSI__GPIO_1_14(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_4_GPIO1_27) || defined(CONFIG_ARMADILLO410_CON2_84_GPIO1_27)
	MX25_PAD_CSI_D2__GPIO_1_27(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_5_GPIO1_15) || defined(CONFIG_ARMADILLO410_CON2_83_GPIO1_15)
	MX25_PAD_CSPI1_MISO__GPIO_1_15(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_6_GPIO1_28) || defined(CONFIG_ARMADILLO410_CON2_82_GPIO1_28)
	MX25_PAD_CSI_D3__GPIO_1_28(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO410_CON2_63_GPIO_2_20)
	MX25_PAD_DE_B__GPIO_2_20(0), /* 47K_UP is always on */
#endif
#if defined(CONFIG_ARMADILLO410_CON2_62_GPIO_2_29)
	MX25_PAD_KPP_ROW0__GPIO_2_29(PAD_CTL_PKE | PAD_CTL_PUS_47K_UP),
#endif
#if defined(CONFIG_ARMADILLO410_CON2_61_GPIO_2_30)
	MX25_PAD_KPP_ROW1__GPIO_2_30(PAD_CTL_PKE | PAD_CTL_PUS_47K_UP),
#endif
#if defined(CONFIG_ARMADILLO410_CON2_43_GPIO3_30)
	MX25_PAD_NFWP_B__GPIO_3_30,
#endif
};

static struct pad_desc armadillo400_uart_pad =
	MX25_PAD_CTL_GRP_DSE_UART(PAD_CTL_DSE_STANDARD);

static struct pad_desc armadillo400_uart2_pads[] = {
	MX25_PAD_UART2_RXD__UART2_RXD(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
	MX25_PAD_UART2_TXD__UART2_TXD(0),
	MX25_PAD_UART2_RTS__UART2_RTS(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
	MX25_PAD_UART2_CTS__UART2_CTS(0),
	MX25_PAD_UART1_RXD__UART2_DTR(0),
	MX25_PAD_UART1_TXD__GPIO_4_23(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
	MX25_PAD_UART1_RTS__UART2_DCD(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
	MX25_PAD_UART1_CTS__UART2_RI(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
	/* UART2_FORCEOFF */
	MX25_PAD_BOOT_MODE1__GPIO_4_31,
};

static struct pad_desc armadillo400_uart3_pads[] = {
#if defined(CONFIG_ARMADILLO400_CON9_3_UART3_RXD) || defined(CONFIG_ARMADILLO410_CON2_85_UART3_RXD)
	MX25_PAD_CSPI1_MOSI__UART3_RXD(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_40_UART3_RXD) || defined(CONFIG_ARMADILLO410_CON2_62_UART3_RXD)
	MX25_PAD_KPP_ROW0__UART3_RXD(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_5_UART3_TXD) || defined(CONFIG_ARMADILLO410_CON2_83_UART3_TXD)
	MX25_PAD_CSPI1_MISO__UART3_TXD(0),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_41_UART3_TXD) || defined(CONFIG_ARMADILLO410_CON2_61_UART3_TXD)
	MX25_PAD_KPP_ROW1__UART3_TXD(0),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_11_UART3_RTS) || defined(CONFIG_ARMADILLO410_CON2_81_UART3_RTS)
	MX25_PAD_CSPI1_SS1__UART3_RTS(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_42_UART3_RTS) || defined(CONFIG_ARMADILLO410_CON2_60_UART3_RTS)
	MX25_PAD_KPP_ROW2__UART3_RTS(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_13_UART3_CTS) || defined(CONFIG_ARMADILLO410_CON2_79_UART3_CTS)
	MX25_PAD_CSPI1_SCLK__UART3_CTS(0),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_43_UART3_CTS) || defined(CONFIG_ARMADILLO410_CON2_59_UART3_CTS)
	MX25_PAD_KPP_ROW3__UART3_CTS(0),
#endif
};

static struct pad_desc armadillo400_uart4_pads[] = {
#if defined(CONFIG_ARMADILLO400_CON11_44_UART4_RXD) || defined(CONFIG_ARMADILLO410_CON2_58_UART4_RXD)
	MX25_PAD_KPP_COL0__UART4_RXD(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_45_UART4_TXD) || defined(CONFIG_ARMADILLO410_CON2_57_UART4_TXD)
	MX25_PAD_KPP_COL1__UART4_TXD(0),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_46_UART4_RTS) || defined(CONFIG_ARMADILLO410_CON2_56_UART4_RTS)
	MX25_PAD_KPP_COL2__UART4_RTS(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_47_UART4_CTS) || defined(CONFIG_ARMADILLO410_CON2_55_UART4_CTS)
	MX25_PAD_KPP_COL3__UART4_CTS(0),
#endif
};

static struct pad_desc armadillo400_uart5_pads[] = {
#if defined(CONFIG_ARMADILLO400_CON9_4_UART5_RXD) || defined(CONFIG_ARMADILLO410_CON2_84_UART5_RXD)
	MX25_PAD_CSI_D2__UART5_RXD(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_6_UART5_TXD) || defined(CONFIG_ARMADILLO410_CON2_82_UART5_TXD)
	MX25_PAD_CSI_D3__UART5_TXD(0),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_12_UART5_RTS) || defined(CONFIG_ARMADILLO410_CON2_80_UART5_RTS)
	MX25_PAD_CSI_D4__UART5_RTS(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_14_UART5_CTS) || defined(CONFIG_ARMADILLO410_CON2_78_UART5_CTS)
	MX25_PAD_CSI_D5__UART5_CTS(0),
#endif
};

#define I2C_PAD_CTRL (PAD_CTL_HYS | PAD_CTL_PKE | \
		      PAD_CTL_PUE | PAD_CTL_PUS_100K_UP | PAD_CTL_ODE)

static struct pad_desc armadillo400_i2c1_pads[] = {
	MX25_PAD_I2C1_CLK__I2C1_CLK(PAD_CTL_PUS_100K_UP | PAD_CTL_ODE),
	MX25_PAD_I2C1_DAT__I2C1_DAT(PAD_CTL_PUS_100K_UP | PAD_CTL_ODE),
};

static struct pad_desc armadillo400_i2c1_gpio_pads[] = {
	MX25_PAD_I2C1_CLK__GPIO_1_12(PAD_CTL_PUS_100K_UP),
	MX25_PAD_I2C1_DAT__GPIO_1_13(PAD_CTL_PUS_100K_UP),
};
#define I2C1_CLK_GPIO GPIO(1, 12)
#define I2C1_DAT_GPIO GPIO(1, 13)

static struct pad_desc __maybe_unused armadillo400_i2c2_pads[] = {
#if defined(CONFIG_ARMADILLO400_CON14_3_I2C2_SCL) || defined(CONFIG_ARMADILLO410_CON2_65_I2C2_SCL)
	MX25_PAD_GPIO_C__I2C2_SCL(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_22K_UP | PAD_CTL_ODE),
#endif
#if defined(CONFIG_ARMADILLO400_CON14_4_I2C2_SDA) || defined(CONFIG_ARMADILLO410_CON2_64_I2C2_SDA)
	MX25_PAD_GPIO_D__I2C2_SDA(PAD_CTL_PKE | PAD_CTL_PUS_22K_UP | PAD_CTL_ODE),
#endif
};

static struct pad_desc __maybe_unused armadillo400_i2c2_gpio_pads[] = {
#if defined(CONFIG_ARMADILLO400_CON14_3_I2C2_SCL) || defined(CONFIG_ARMADILLO410_CON2_65_I2C2_SCL)
	MX25_PAD_GPIO_C__GPIO_C(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_22K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON14_4_I2C2_SDA) || defined(CONFIG_ARMADILLO410_CON2_64_I2C2_SDA)
	MX25_PAD_GPIO_D__GPIO_D(PAD_CTL_PKE | PAD_CTL_PUS_22K_UP),
#endif
};

#define I2C2_CLK_GPIO GPIO(1, 2)
#define I2C2_DAT_GPIO GPIO(1, 3)

static struct pad_desc __maybe_unused armadillo400_i2c3_pads[] = {
#if defined(CONFIG_ARMADILLO400_CON11_48_I2C3_SCL) || defined(CONFIG_ARMADILLO410_CON2_54_I2C3_SCL)
	MX25_PAD_GPIO_A__I2C3_SCL(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_22K_UP | PAD_CTL_ODE),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_49_I2C3_SDA) || defined(CONFIG_ARMADILLO410_CON2_53_I2C3_SDA)
	MX25_PAD_GPIO_B__I2C3_SDA(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_22K_UP | PAD_CTL_ODE),
#endif
};

static struct pad_desc __maybe_unused armadillo400_i2c3_gpio_pads[] = {
#if defined(CONFIG_ARMADILLO400_CON11_48_I2C3_SCL) || defined(CONFIG_ARMADILLO410_CON2_54_I2C3_SCL)
	MX25_PAD_GPIO_A__GPIO_A(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_22K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_49_I2C3_SDA) || defined(CONFIG_ARMADILLO400_CON2_53_I2C3_SDA)
	MX25_PAD_GPIO_B__GPIO_B(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_22K_UP),
#endif
};
#define I2C3_CLK_GPIO GPIO(1, 0)
#define I2C3_DAT_GPIO GPIO(1, 1)

static struct pad_desc armadillo400_spi1_pads[] = {
#if defined(CONFIG_ARMADILLO400_CON9_3_CSPI1_MOSI) || defined(CONFIG_ARMADILLO410_CON2_85_CSPI1_MOSI)
	MX25_PAD_CSPI1_MOSI__CSPI1_MOSI(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_5_CSPI1_MISO) || defined(CONFIG_ARMADILLO410_CON2_83_CSPI1_MISO)
	MX25_PAD_CSPI1_MISO__CSPI1_MISO(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_11_CSPI1_SS1) || defined(CONFIG_ARMADILLO410_CON2_81_CSPI1_SS1)
	MX25_PAD_CSPI1_SS1__GPIO_1_17(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_13_CSPI1_SCLK) || defined(CONFIG_ARMADILLO410_CON2_79_CSPI1_SCLK)
	MX25_PAD_CSPI1_SCLK__CSPI1_SCLK(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_25_CSPI1_SS0) || defined(CONFIG_ARMADILLO410_CON2_69_CSPI1_SS0)
	MX25_PAD_CSPI1_SS0__GPIO_1_16(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_26_CSPI1_RDY) || defined(CONFIG_ARMADILLO410_CON2_68_CSPI1_RDY)
	MX25_PAD_CSPI1_RDY__CSPI1_RDY(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
};

static struct pad_desc armadillo400_spi2_pads[] = {

};

static struct pad_desc armadillo400_spi3_pads[] = {
#if defined(CONFIG_ARMADILLO400_CON9_4_CSPI3_MOSI) || defined(CONFIG_ARMADILLO410_CON2_84_CSPI3_MOSI)
	MX25_PAD_CSI_D2__CSPI3_MOSI(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_6_CSPI3_MISO) || defined(CONFIG_ARMADILLO410_CON2_82_CSPI3_MISO)
	MX25_PAD_CSI_D3__CSPI3_MISO(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_12_CSPI3_SCLK) || defined(CONFIG_ARMADILLO410_CON2_80_CSPI3_SCLK)
	MX25_PAD_CSI_D4__CSPI3_SCLK(PAD_CTL_HYS | PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_14_CSPI3_RDY) || defined(CONFIG_ARMADILLO410_CON2_78_CSPI3_RDY)
	MX25_PAD_CSI_D5__CSPI3_RDY(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_16_CSPI3_SS0) || defined(CONFIG_ARMADILLO410_CON2_76_CSPI3_SS0)
	MX25_PAD_CSI_D6__GPIO_1_31(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_18_CSPI3_SS1) || defined(CONFIG_ARMADILLO410_CON2_74_CSPI3_SS1)
	MX25_PAD_CSI_D7__GPIO_1_6(PAD_CTL_HYS | PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_15_CSPI3_SS2) || defined(CONFIG_ARMADILLO410_CON2_77_CSPI3_SS2)
	MX25_PAD_CSI_D8__GPIO_1_7(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_17_CSPI3_SS3) || defined(CONFIG_ARMADILLO410_CON2_75_CSPI3_SS3)
	MX25_PAD_CSI_D9__GPIO_4_21(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
};

#define FEC_PAD_CTRL1 (PAD_CTL_HYS | PAD_CTL_PUE | \
		      PAD_CTL_PKE)
#define FEC_PAD_CTRL2 (PAD_CTL_PUE)

static struct pad_desc armadillo400_fec_pads[] = {
	MX25_PAD_FEC_TX_CLK__FEC_TX_CLK(PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
	MX25_PAD_FEC_RX_DV__FEC_RX_DV(PAD_CTL_PUE),
	MX25_PAD_FEC_RDATA0__FEC_RDATA0(PAD_CTL_PUE),
	MX25_PAD_FEC_TDATA0__FEC_TDATA0(PAD_CTL_PUE | PAD_CTL_PUS_100K_UP | PAD_CTL_SRE_FAST),
	MX25_PAD_FEC_TX_EN__FEC_TX_EN(PAD_CTL_PUE | PAD_CTL_PUS_100K_UP | PAD_CTL_SRE_FAST),
	MX25_PAD_FEC_MDC__FEC_MDC(PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
	MX25_PAD_FEC_MDIO__FEC_MDIO(PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PUS_22K_UP),
	MX25_PAD_FEC_RDATA1__FEC_RDATA1(PAD_CTL_PUE),
	MX25_PAD_FEC_TDATA1__FEC_TDATA1(PAD_CTL_PUE | PAD_CTL_PUS_100K_UP | PAD_CTL_SRE_FAST),
	MX25_PAD_CTL_GRP_DSE_FEC(PAD_CTL_DSE_STANDARD),

	/* PHY_RST */
	MX25_PAD_VSTBY_ACK__GPIO_3_18(0),
	/* LINK_LED */
	MX25_PAD_UPLL_BYPCLK__GPIO_3_16,
};

static struct pad_desc armadillo400_usb_pads[] = {
	MX25_PAD_NFWE_B__GPIO_3_26,
};

static struct pad_desc armadillo400_sdhc1_pads_revb[] = {
	MX25_PAD_SD1_CMD__SD1_CMD(PAD_CTL_PUE | PAD_CTL_PUS_100K_UP | PAD_CTL_SRE_FAST),
	MX25_PAD_SD1_CLK__SD1_CLK(PAD_CTL_PUE | PAD_CTL_PUS_100K_UP | PAD_CTL_SRE_FAST),
	MX25_PAD_SD1_DATA0__SD1_DATA0(PAD_CTL_PUE | PAD_CTL_PUS_100K_UP | PAD_CTL_SRE_FAST),
	MX25_PAD_SD1_DATA1__SD1_DATA1(PAD_CTL_PUE | PAD_CTL_PUS_100K_UP | PAD_CTL_SRE_FAST),
	MX25_PAD_SD1_DATA2__SD1_DATA2(PAD_CTL_PUE | PAD_CTL_PUS_100K_UP | PAD_CTL_SRE_FAST),
	MX25_PAD_SD1_DATA3__SD1_DATA3(PAD_CTL_PUE | PAD_CTL_PUS_100K_UP | PAD_CTL_SRE_FAST),

	MX25_PAD_CTL_GRP_DSE_SDHC1(PAD_CTL_DSE_HIGH),

	/* SDHC1_CD */
	MX25_PAD_NFRB__GPIO_3_31(0),
	/* SDHC1_PWRE */
	MX25_PAD_VSTBY_REQ__GPIO_3_17(0),
};

static struct pad_desc armadillo400_sdhc1_pads_revc[] = {
	MX25_PAD_SD1_CMD__SD1_CMD(PAD_CTL_PUE | PAD_CTL_PUS_100K_UP | PAD_CTL_SRE_FAST),
	MX25_PAD_SD1_CLK__SD1_CLK(PAD_CTL_PUE | PAD_CTL_PUS_100K_UP | PAD_CTL_SRE_FAST),
	MX25_PAD_SD1_DATA0__SD1_DATA0(PAD_CTL_PUE | PAD_CTL_PUS_100K_UP | PAD_CTL_SRE_FAST),
	MX25_PAD_SD1_DATA1__SD1_DATA1(PAD_CTL_PUE | PAD_CTL_PUS_100K_UP | PAD_CTL_SRE_FAST),
	MX25_PAD_SD1_DATA2__SD1_DATA2(PAD_CTL_PUE | PAD_CTL_PUS_100K_UP | PAD_CTL_SRE_FAST),
	MX25_PAD_SD1_DATA3__SD1_DATA3(PAD_CTL_PUE | PAD_CTL_PUS_100K_UP | PAD_CTL_SRE_FAST),

	MX25_PAD_CTL_GRP_DSE_SDHC1(PAD_CTL_DSE_HIGH),

	/* SDHC1_CD */
	MX25_PAD_NFRB__GPIO_3_31(0),
	/* SDHC1_PWRE */
	MX25_PAD_NFRE_B__GPIO_3_27,
};

static struct pad_desc armadillo460_sdhc1_pads[] = {
	MX25_PAD_BCLK__GPIO_4_4,
};

#if defined(CONFIG_MMC_MXC_SELECT2)
static struct pad_desc armadillo400_sdhc2_pads[] = {
#if defined(CONFIG_ARMADILLO400_CON9_16_SDHC2_CMD) || defined(CONFIG_ARMADILLO410_CON2_76_SDHC2_CMD)
	MX25_PAD_CSI_D6__SD2_CMD(PAD_CTL_SRE_FAST),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_18_SDHC2_CLK) || defined(CONFIG_ARMADILLO410_CON2_74_SDHC2_CLK)
	MX25_PAD_CSI_D7__SD2_CLK(PAD_CTL_SRE_FAST),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_21_SDHC2_DATA0) || defined(CONFIG_ARMADILLO410_CON2_73_SDHC2_DATA0)
	MX25_PAD_CSI_MCLK__SD2_DATA0(PAD_CTL_SRE_FAST),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_22_SDHC2_DATA1) || defined(CONFIG_ARMADILLO410_CON2_72_SDHC2_DATA1)
	MX25_PAD_CSI_VSYNC__SD2_DATA1(PAD_CTL_SRE_FAST),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_23_SDHC2_DATA2) || defined(CONFIG_ARMADILLO410_CON2_71_SDHC2_DATA2)
	MX25_PAD_CSI_HSYNC__SD2_DATA2(PAD_CTL_SRE_FAST),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_24_SDHC2_DATA3) || defined(CONFIG_ARMADILLO410_CON2_70_SDHC2_DATA3)
	MX25_PAD_CSI_PIXCLK__SD2_DATA3(PAD_CTL_HYS | PAD_CTL_SRE_FAST),
#endif

	MX25_PAD_CTL_GRP_DSE_CSI(PAD_CTL_DSE_HIGH),

	/* SDHC2_CD */
#if defined(CONFIG_ARMADILLO400_CON9_17_SDHC2_CD) || defined(CONFIG_ARMADILLO410_CON2_75_SDHC2_CD)
	MX25_PAD_CSI_D9__GPIO_4_21(PAD_CTL_HYS | PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
	/* SDHC2_WP */
#if defined(CONFIG_ARMADILLO400_CON9_15_SDHC2_WP) || defined(CONFIG_ARMADILLO410_CON2_77_SDHC2_WP)
	MX25_PAD_CSI_D8__GPIO_1_7(PAD_CTL_HYS | PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
};
#endif

static struct pad_desc armadillo400_lcdc_pads[] = {
	MX25_PAD_LD0__LD0(0),
	MX25_PAD_LD1__LD1(0),
	MX25_PAD_LD2__LD2(0),
	MX25_PAD_LD3__LD3(0),
	MX25_PAD_LD4__LD4(0),
	MX25_PAD_LD5__LD5(0),
	MX25_PAD_LD6__LD6(0),
	MX25_PAD_LD7__LD7(0),
	MX25_PAD_LD8__LD8(0),
	MX25_PAD_LD9__LD9(0),
	MX25_PAD_LD10__LD10(0),
	MX25_PAD_LD11__LD11(0),
	MX25_PAD_LD12__LD12(0),
	MX25_PAD_LD13__LD13(0),
	MX25_PAD_LD14__LD14(0),
	MX25_PAD_LD15__LD15(0),
	MX25_PAD_GPIO_E__LD16(0),
	MX25_PAD_GPIO_F__LD17(0),
	MX25_PAD_HSYNC__HSYNC(0),
	MX25_PAD_VSYNC__VSYNC(0),
	MX25_PAD_LSCLK__LSCLK(PAD_CTL_SRE_FAST),
	MX25_PAD_OE_ACD__OE_ACD(0),
	MX25_PAD_CTL_GRP_DSE_LCD(PAD_CTL_DSE_STANDARD),
};

static struct pad_desc armadillo400_lcd_bl_pads[] = {
	MX25_PAD_PWM__PWM(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
};

static struct pad_desc armadillo400_audio_pads[] = {
#if defined(CONFIG_ARMADILLO400_CON11_44_AUD5_TXD) || defined(CONFIG_ARMADILLO410_CON2_58_AUD5_TXD)
	MX25_PAD_KPP_COL0__AUD5_TXD(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_45_AUD5_RXD) || defined(CONFIG_ARMADILLO410_CON2_57_AUD5_RXD)
	MX25_PAD_KPP_COL1__AUD5_RXD(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_46_AUD5_TXC) || defined(CONFIG_ARMADILLO410_CON2_56_AUD5_TXC)
	MX25_PAD_KPP_COL2__AUD5_TXC(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_47_AUD5_TXFS) || defined(CONFIG_ARMADILLO410_CON2_55_AUD5_TXFS)
	MX25_PAD_KPP_COL3__AUD5_TXFS(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_42_AUD5_RXC) || defined(CONFIG_ARMADILLO410_CON2_60_AUD5_RXC)
	MX25_PAD_KPP_ROW2__AUD5_RXC(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP | PAD_CTL_SRE_FAST),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_43_AUD5_RXFS) || defined(CONFIG_ARMADILLO410_CON2_59_AUD5_RXFS)
	MX25_PAD_KPP_ROW3__AUD5_RXFS(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif

#if defined(CONFIG_ARMADILLO400_CON9_21_AUD6_TXD) || defined(CONFIG_ARMADILLO410_CON2_73_AUD6_TXD)
	MX25_PAD_CSI_MCLK__AUD6_TXD(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_22_AUD6_RXD) || defined(CONFIG_ARMADILLO410_CON2_72_AUD6_RXD)
	MX25_PAD_CSI_VSYNC__AUD6_RXD(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_23_AUD6_TXC) || defined(CONFIG_ARMADILLO410_CON2_71_AUD6_TXC)
	MX25_PAD_CSI_HSYNC__AUD6_TXC(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_24_AUD6_TXFS) || defined(CONFIG_ARMADILLO410_CON2_70_AUD6_TXFS)
	MX25_PAD_CSI_PIXCLK__AUD6_TXFS(PAD_CTL_HYS | PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_15_AUD6_RXC) || defined(CONFIG_ARMADILLO410_CON2_77_AUD6_RXC)
	MX25_PAD_CSI_D8__AUD6_RXC(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP | PAD_CTL_SRE_FAST),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_17_AUD6_RXFS) || defined(CONFIG_ARMADILLO410_CON2_75_AUD6_RXFS)
	MX25_PAD_CSI_D9__AUD6_RXFS(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
};

static struct pad_desc armadillo400_pwm1_pads[] = {
};

static struct pad_desc armadillo400_pwm2_pads[] = {
#if defined(CONFIG_ARMADILLO400_CON9_25_PWMO2) || defined(CONFIG_ARMADILLO410_CON2_69_PWMO2)
	MX25_PAD_CSPI1_SS0__PWM2(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
};

static struct pad_desc armadillo400_pwm3_pads[] = {
};

static struct pad_desc armadillo400_pwm4_pads[] = {
#if defined(CONFIG_ARMADILLO400_CON14_3_PWMO4) || defined(CONFIG_ARMADILLO410_CON2_65_PWMO4)
	MX25_PAD_GPIO_C__PWM4(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
};

static struct pad_desc armadillo400_mxc_w1_pads[] = {
#if defined(CONFIG_ARMADILLO400_CON9_2_W1) || defined(CONFIG_ARMADILLO410_CON2_86_W1)
	MX25_PAD_RTCK__OWIRE(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_22K_UP),
#endif
};

static struct pad_desc armadillo400_flexcan1_pads[] = {

};

static struct pad_desc armadillo400_flexcan2_pads[] = {
#if defined(CONFIG_ARMADILLO400_CON14_3_CAN2_TXCAN) || defined(CONFIG_ARMADILLO410_CON2_65_CAN2_TXCAN)
	MX25_PAD_GPIO_C__CAN2_TX(0),
#endif
#if defined(CONFIG_ARMADILLO400_CON14_4_CAN2_RXCAN) || defined(CONFIG_ARMADILLO410_CON2_64_CAN2_RXCAN)
	MX25_PAD_GPIO_D__CAN2_RX(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
};

static struct pad_desc armadillo440_keypad_pads[] = {
#if defined(CONFIG_ARMADILLO400_CON11_40_KPP_ROW0) || defined(CONFIG_ARMADILLO410_CON2_62_KPP_ROW0)
	MX25_PAD_KPP_ROW0__KPP_ROW0(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_41_KPP_ROW1) || defined(CONFIG_ARMADILLO410_CON2_61_KPP_ROW1)
	MX25_PAD_KPP_ROW1__KPP_ROW1(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_42_KPP_ROW2) || defined(CONFIG_ARMADILLO410_CON2_60_KPP_ROW2)
	MX25_PAD_KPP_ROW2__KPP_ROW2(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_43_KPP_ROW3) || defined(CONFIG_ARMADILLO410_CON2_59_KPP_ROW3)
	MX25_PAD_KPP_ROW3__KPP_ROW3(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_48_KPP_ROW4) || defined(CONFIG_ARMADILLO410_CON2_54_KPP_ROW4)
	MX25_PAD_GPIO_A__KPP_ROW4(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_49_KPP_ROW5) || defined(CONFIG_ARMADILLO410_CON2_53_KPP_ROW5)
	MX25_PAD_GPIO_B__KPP_ROW5(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif

#if defined(CONFIG_ARMADILLO400_CON11_44_KPP_COL0) || defined(CONFIG_ARMADILLO410_CON2_58_KPP_COL0)
	MX25_PAD_KPP_COL0__KPP_COL0(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP | PAD_CTL_ODE),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_45_KPP_COL1) || defined(CONFIG_ARMADILLO410_CON2_57_KPP_COL1)
	MX25_PAD_KPP_COL1__KPP_COL1(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP | PAD_CTL_ODE),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_46_KPP_COL2) || defined(CONFIG_ARMADILLO410_CON2_56_KPP_COL2)
	MX25_PAD_KPP_COL2__KPP_COL2(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP | PAD_CTL_ODE),
#endif
#if defined(CONFIG_ARMADILLO400_CON11_47_KPP_COL3) || defined(CONFIG_ARMADILLO410_CON2_55_KPP_COL3)
	MX25_PAD_KPP_COL3__KPP_COL3(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP | PAD_CTL_ODE),
#endif
};

#if defined(CONFIG_ARMADILLO400_CON9_2_RTC_ALM_INT) || defined(CONFIG_ARMADILLO410_CON2_86_RTC_ALM_INT)
static struct pad_desc armadillo400_rtc_pads[] = {
	MX25_PAD_RTCK__GPIO_3_14(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_22K_UP),
};
#endif

/*!
 * Enable FEC
 */
void gpio_fec_enable(void)
{
	gpio_direction_output(PHY_RST_GPIO, 0);

	/* PHY spec says 100us min and Armadillo-400 has low pass filter
	 * for PHY_RST pin (300us). */
	udelay(1000);

	gpio_set_value(PHY_RST_GPIO, 1);
}
EXPORT_SYMBOL(gpio_fec_enable);

/*!
 * Disable FEC
 */
void gpio_fec_disable(void)
{
	gpio_direction_output(PHY_RST_GPIO, 0);
}
EXPORT_SYMBOL(gpio_fec_disable);

/*!
 * Link LED Active
 */
void gpio_link_led_active(void)
{
	gpio_direction_output(LINK_LED_GPIO, 0);
}
EXPORT_SYMBOL(gpio_link_led_active);

/*!
 * Link LED Inactive
 */
void gpio_link_led_inactive(void)
{
	gpio_direction_output(LINK_LED_GPIO, 1);
}
EXPORT_SYMBOL(gpio_link_led_inactive);

/*!
 * Activate FEC
 */
void gpio_fec_active(void)
{
	mxc_iomux_v3_setup_multiple_pads(armadillo400_fec_pads,
					 ARRAY_SIZE(armadillo400_fec_pads));

	/*
	 * Set up the GPIO pin.
	 *
	 * PHY_RST: gpio3[18] is ALT 5 mode of pin VSTBY_ACK
	 * LINK_LED: gpio3[16] is ALT 5 mode of pin UPLL_BYPCLK
	 */
	gpio_request(PHY_RST_GPIO, "PHY_RST");
	gpio_fec_enable();

	gpio_request(LINK_LED_GPIO, "LINK_LED");
	gpio_link_led_inactive();
}
EXPORT_SYMBOL(gpio_fec_active);

/*!
 * Inactivate FEC
 */
void gpio_fec_inactive(void)
{
	gpio_link_led_inactive();
	gpio_fec_disable();

	gpio_free(PHY_RST_GPIO);

	gpio_free(LINK_LED_GPIO);
}
EXPORT_SYMBOL(gpio_fec_inactive);

void gpio_fec_suspend(void)
{
	gpio_link_led_inactive();
}
EXPORT_SYMBOL(gpio_fec_suspend);

void gpio_fec_resume(void)
{
	gpio_fec_enable();
}
EXPORT_SYMBOL(gpio_fec_resume);

/*!
 * Activate a UART port
 *
 * @param  port         a UART port
 * @param  no_irda      indicates if the port is used for SIR
 */
void gpio_uart_active(int port, int no_irda)
{
	static int gpio_uart_enabled = 0;

	if (!gpio_uart_enabled)
		mxc_iomux_v3_setup_pad(&armadillo400_uart_pad);

	/*
	 * Configure the IOMUX control registers for the UART signals
	 */
	switch (port) {
	case 0: /* UART1 is not used */
		break;
	case 1: /* UART2 IOMUX Configs */
		mxc_iomux_v3_setup_multiple_pads(armadillo400_uart2_pads,
						 ARRAY_SIZE(armadillo400_uart2_pads));
		if (!(gpio_uart_enabled & (1 << port)))
			gpio_request(UART2_FORCEOFF_GPIO, "UART2_FORCEOFF");
		gpio_direction_output(UART2_FORCEOFF_GPIO, 1);
		break;
	case 2: /* UART3 IOMUX Configs */
		mxc_iomux_v3_setup_multiple_pads(armadillo400_uart3_pads,
						 ARRAY_SIZE(armadillo400_uart3_pads));
		break;
	case 3: /* UART4 IOMUX Configs */
		mxc_iomux_v3_setup_multiple_pads(armadillo400_uart4_pads,
						 ARRAY_SIZE(armadillo400_uart4_pads));
		break;
	case 4: /* UART5 is IOMUX Configs */
		mxc_iomux_v3_setup_multiple_pads(armadillo400_uart5_pads,
						 ARRAY_SIZE(armadillo400_uart5_pads));
		break;
	default:
		break;
	}

	gpio_uart_enabled |= (1 << port);
}
EXPORT_SYMBOL(gpio_uart_active);

/*!
 * Inactivate a UART port
 *
 * @param  port         a UART port
 * @param  no_irda      indicates if the port is used for SIR
 */
void gpio_uart_inactive(int port, int no_irda)
{
	switch (port) {
	case 0: /* UART1 */
		break;
	case 1: /* UART2 */
		gpio_set_value(UART2_FORCEOFF_GPIO, 0);
		break;
	case 2: /* UART3 */
		break;
	case 3: /* UART4 */
		break;
	case 4: /* UART5 */
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(gpio_uart_inactive);

/*!
 * Configure the IOMUX GPR register to receive shared SDMA UART events
 *
 * @param  port         a UART port
 */
void config_uartdma_event(int port)
{
}
EXPORT_SYMBOL(config_uartdma_event);

void gpio_activate_ext_gpio(struct mxc_ext_gpio *ext_gpios, int nr_ext_gpios)
{
	int i;

	for (i = 0; i < nr_ext_gpios; i++) {
		gpio_request(ext_gpios[i].gpio, ext_gpios[i].name);
		gpio_export(ext_gpios[i].gpio, true);
		if (ext_gpios[i].default_direction == MXC_EXT_GPIO_DIRECTION_OUTPUT)
			gpio_direction_output(ext_gpios[i].gpio, ext_gpios[i].default_value);
		else
			gpio_direction_input(ext_gpios[i].gpio);
	}
}

#define GPIO_I2C_DUMMY_CLOCK_NUM 128
#define GPIO_I2C_DUMMY_CLOCK_FREQ 40000

int gpio_i2c_dummy_clock(unsigned gpio_clk, unsigned gpio_dat)
{
	int i;

	if (gpio_request(gpio_clk, "i2c_clk_gpio"))
		return -EINVAL;

	if (gpio_request(gpio_dat, "i2c_dat_gpio")) {
		gpio_free(gpio_clk);
		return -EINVAL;
	}

	gpio_direction_output(gpio_clk, 0);
	gpio_direction_input(gpio_dat);

	if (gpio_get_value(gpio_dat) == 0) {
		for (i = 0; i < GPIO_I2C_DUMMY_CLOCK_NUM; i++) {
			gpio_set_value(gpio_clk, 1);
			udelay(1000*1000/GPIO_I2C_DUMMY_CLOCK_FREQ);
			gpio_set_value(gpio_clk, 0);
			udelay(1000*1000/GPIO_I2C_DUMMY_CLOCK_FREQ);
		}
	}

	gpio_free(gpio_clk);
	gpio_free(gpio_dat);

	return 0;
}

/*!
 * Activate an I2C device
 *
 * @param  i2c_num         an I2C device
 */
void gpio_i2c_active(int i2c_num)
{
	switch (i2c_num) {
	case 0: /*I2C1*/
		mxc_iomux_v3_setup_multiple_pads(armadillo400_i2c1_gpio_pads,
						 ARRAY_SIZE(armadillo400_i2c1_gpio_pads));
		gpio_i2c_dummy_clock(I2C1_CLK_GPIO, I2C1_DAT_GPIO);

		mxc_iomux_v3_setup_multiple_pads(armadillo400_i2c1_pads,
						 ARRAY_SIZE(armadillo400_i2c1_pads));

		break;
	case 1: /*I2C2*/
#if defined(CONFIG_I2C_MXC_SELECT2)
		mxc_iomux_v3_setup_multiple_pads(armadillo400_i2c2_gpio_pads,
						 ARRAY_SIZE(armadillo400_i2c2_gpio_pads));
		gpio_i2c_dummy_clock(I2C2_CLK_GPIO, I2C2_DAT_GPIO);

		mxc_iomux_v3_setup_multiple_pads(armadillo400_i2c2_pads,
						 ARRAY_SIZE(armadillo400_i2c2_pads));
#endif
		break;
	case 2: /*I2C3*/
#if defined(CONFIG_I2C_MXC_SELECT3)
		mxc_iomux_v3_setup_multiple_pads(armadillo400_i2c3_gpio_pads,
						 ARRAY_SIZE(armadillo400_i2c3_gpio_pads));
		gpio_i2c_dummy_clock(I2C3_CLK_GPIO, I2C3_DAT_GPIO);

		mxc_iomux_v3_setup_multiple_pads(armadillo400_i2c3_pads,
						 ARRAY_SIZE(armadillo400_i2c3_pads));
#endif
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(gpio_i2c_active);

void gpio_rtc_alarm_int_active(void)
{
#if defined(CONFIG_ARMADILLO400_CON9_2_RTC_ALM_INT) || defined(CONFIG_ARMADILLO410_CON2_86_RTC_ALM_INT)
	mxc_iomux_v3_setup_multiple_pads(armadillo400_rtc_pads,
					 ARRAY_SIZE(armadillo400_rtc_pads));
	gpio_request(GPIO(3, 14), "RTC_ALM_INT");
	gpio_direction_input(GPIO(3, 14));
#endif
}
EXPORT_SYMBOL(gpio_rtc_alarm_int_active);

/*!
 * Inactivate an I2C device
 *
 * @param  i2c_num         an I2C device
 */
void gpio_i2c_inactive(int i2c_num)
{
	switch (i2c_num) {
	case 0:
		/*I2C1*/
		break;
	case 1:
		/*I2C2*/
		break;
	case 2:
		/*I2C3*/
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(gpio_i2c_inactive);

static void gpio_spi_initialize(int cspi_mod)
{
	switch (cspi_mod) {
	case 0:
		mxc_iomux_v3_setup_multiple_pads(armadillo400_spi1_pads,
						 ARRAY_SIZE(armadillo400_spi1_pads));
#if defined(CONFIG_ARMADILLO400_CON9_25_CSPI1_SS0) || defined(CONFIG_ARMADILLO410_CON2_69_CSPI1_SS0)
		gpio_request(GPIO(1, 16), "CSPI1_SS0");
		gpio_direction_output(GPIO(1, 16), 1);
#endif
#if defined(CONFIG_ARMADILLO400_CON9_11_CSPI1_SS1) || defined(CONFIG_ARMADILLO410_CON2_81_CSPI1_SS1)
		gpio_request(GPIO(1, 17), "CSPI1_SS1");
		gpio_direction_output(GPIO(1, 17), 1);
#endif
		break;
	case 1:
		mxc_iomux_v3_setup_multiple_pads(armadillo400_spi2_pads,
						 ARRAY_SIZE(armadillo400_spi2_pads));
		break;
	case 2:
		mxc_iomux_v3_setup_multiple_pads(armadillo400_spi3_pads,
						 ARRAY_SIZE(armadillo400_spi3_pads));
#if defined(CONFIG_ARMADILLO400_CON9_16_CSPI3_SS0) || defined(CONFIG_ARMADILLO410_CON2_76_CSPI3_SS0)
		gpio_request(GPIO(1, 31), "CSPI3_SS0");
		gpio_direction_output(GPIO(1, 31), 1);
#endif
#if defined(CONFIG_ARMADILLO400_CON9_18_CSPI3_SS1) || defined(CONFIG_ARMADILLO410_CON2_74_CSPI3_SS1)
		gpio_request(GPIO(1, 6), "CSPI3_SS1");
		gpio_direction_output(GPIO(1, 6), 1);
#endif
#if defined(CONFIG_ARMADILLO400_CON9_15_CSPI3_SS2) || defined(CONFIG_ARMADILLO410_CON2_77_CSPI3_SS2)
		gpio_request(GPIO(1, 7), "CSPI3_SS2");
		gpio_direction_output(GPIO(1, 7), 1);
#endif
#if defined(CONFIG_ARMADILLO400_CON9_17_CSPI3_SS3) || defined(CONFIG_ARMADILLO410_CON2_75_CSPI3_SS3)
		gpio_request(GPIO(4, 21), "CSPI3_SS3");
		gpio_direction_output(GPIO(4, 21), 1);
#endif
		break;
	default:
		WARN_ON(1);
		break;
	}
}

void gpio_spi_active(int cspi_mod)
{
	static int spi_initialized = 0;

	if (!(spi_initialized & (1 << cspi_mod))) {
		gpio_spi_initialize(cspi_mod);
		spi_initialized |= (1 << cspi_mod);
	}

	switch (cspi_mod) {
	case 0:
#if defined(CONFIG_ARMADILLO400_CON9_25_CSPI1_SS0) || defined(CONFIG_ARMADILLO410_CON2_69_CSPI1_SS0)
		gpio_set_value(GPIO(1, 16), 1);
#endif
#if defined(CONFIG_ARMADILLO400_CON9_11_CSPI1_SS1) || defined(CONFIG_ARMADILLO410_CON2_81_CSPI1_SS1)
		gpio_set_value(GPIO(1, 17), 1);
#endif
		break;
	case 1:
		break;
	case 2:
#if defined(CONFIG_ARMADILLO400_CON9_16_CSPI3_SS0) || defined(CONFIG_ARMADILLO410_CON2_76_CSPI3_SS0)
		gpio_set_value(GPIO(1, 31), 1);
#endif
#if defined(CONFIG_ARMADILLO400_CON9_18_CSPI3_SS1) || defined(CONFIG_ARMADILLO410_CON2_74_CSPI3_SS1)
		gpio_set_value(GPIO(1, 6), 1);
#endif
#if defined(CONFIG_ARMADILLO400_CON9_15_CSPI3_SS2) || defined(CONFIG_ARMADILLO410_CON2_77_CSPI3_SS2)
		gpio_set_value(GPIO(1, 7), 1);
#endif
#if defined(CONFIG_ARMADILLO400_CON9_17_CSPI3_SS3) || defined(CONFIG_ARMADILLO410_CON2_75_CSPI3_SS3)
		gpio_set_value(GPIO(4, 21), 1);
#endif
		break;
	default:
		WARN_ON(1);
		break;
	}
}
EXPORT_SYMBOL(gpio_spi_active);

void gpio_spi_inactive(int cspi_mod)
{
	switch (cspi_mod) {
	case 0:
#if defined(CONFIG_ARMADILLO400_CON9_25_CSPI1_SS0) || defined(CONFIG_ARMADILLO410_CON2_69_CSPI1_SS0)
		gpio_set_value(GPIO(1, 16), 0);
#endif
#if defined(CONFIG_ARMADILLO400_CON9_11_CSPI1_SS1) || defined(CONFIG_ARMADILLO410_CON2_81_CSPI1_SS1)
		gpio_set_value(GPIO(1, 17), 0);
#endif
		break;
	case 1:
		break;
	case 2:
#if defined(CONFIG_ARMADILLO400_CON9_16_CSPI3_SS0) || defined(CONFIG_ARMADILLO410_CON2_76_CSPI3_SS0)
		gpio_set_value(GPIO(1, 31), 0);
#endif
#if defined(CONFIG_ARMADILLO400_CON9_18_CSPI3_SS1) || defined(CONFIG_ARMADILLO410_CON2_74_CSPI3_SS1)
		gpio_set_value(GPIO(1, 6), 0);
#endif
#if defined(CONFIG_ARMADILLO400_CON9_15_CSPI3_SS2) || defined(CONFIG_ARMADILLO410_CON2_77_CSPI3_SS2)
		gpio_set_value(GPIO(1, 7), 0);
#endif
#if defined(CONFIG_ARMADILLO400_CON9_17_CSPI3_SS3) || defined(CONFIG_ARMADILLO410_CON2_75_CSPI3_SS3)
		gpio_set_value(GPIO(4, 21), 0);
#endif
		break;
	default:
		WARN_ON(1);
		break;
	}
}
EXPORT_SYMBOL(gpio_spi_inactive);

static int usb_gpio_use_count = 0;

static int gpio_usb_request(void)
{
	if (!usb_gpio_use_count) {
		mxc_iomux_v3_setup_multiple_pads(armadillo400_usb_pads,
						 ARRAY_SIZE(armadillo400_usb_pads));

		gpio_request(USB_PWRSEL_GPIO, "USB_PWRSEL");
		gpio_direction_output(USB_PWRSEL_GPIO, USB_PWRSRC_5V);
	}
	usb_gpio_use_count++;

	return 0;
}

static void usb_gpio_free(void)
{
	if (usb_gpio_use_count > 0 && !(--usb_gpio_use_count)) {
		gpio_free(USB_PWRSEL_GPIO);
	}
}

int gpio_usbh2_active(void)
{
	int ret;

	ret = gpio_usb_request();
	if (ret != 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL(gpio_usbh2_active);

void gpio_usbh2_inactive(void)
{
	usb_gpio_free();
}
EXPORT_SYMBOL(gpio_usbh2_inactive);

int gpio_usbotg_utmi_active(void)
{
	int ret;

	ret = gpio_usb_request();
	if (ret != 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL(gpio_usbotg_utmi_active);

void gpio_usbotg_utmi_inactive(void)
{
	usb_gpio_free();
}
EXPORT_SYMBOL(gpio_usbotg_utmi_inactive);

/*!
 * Activate SDHC
 *
 * @param module SDHC module number
 */
int sdhc1_wp_gpio = SDHC1_WP_GPIO;

void gpio_sdhc_active(int module)
{
/* #define SDHC_CLK_PAD_CTL (PAD_CTL_SRE_FAST) */
/* #define SDHC_PAD_CTL (PAD_CTL_SRE_FAST| PAD_CTL_HYS_SCHMITZ) */

	static int sdhc_activated = 0;
	unsigned sdhc1_pwre_gpio = -1;

	switch (module) {
	case 0:
		/* SDHC1 */
		switch (system_rev & ARMADILLO_400_BOARD_REV_MAJOR_MASK) {
		case ARMADILLO_400_BOARD_REV_B:
			mxc_iomux_v3_setup_multiple_pads(
				armadillo400_sdhc1_pads_revb,
				ARRAY_SIZE(armadillo400_sdhc1_pads_revb));
			sdhc1_pwre_gpio = SDHC1_PWRE_GPIO_REVB;
			break;
		default:
			pr_warning("Unknown Board Revision 0x%x\n", system_rev);
			/* fall through */
		case ARMADILLO_400_BOARD_REV_C:
			mxc_iomux_v3_setup_multiple_pads(
				armadillo400_sdhc1_pads_revc,
				ARRAY_SIZE(armadillo400_sdhc1_pads_revc));
			sdhc1_pwre_gpio = SDHC1_PWRE_GPIO_REVC;
			break;
		}

		if (machine_is_armadillo460())
			mxc_iomux_v3_setup_multiple_pads(
				armadillo460_sdhc1_pads,
				ARRAY_SIZE(armadillo460_sdhc1_pads));

		gpio_request(SDHC1_CD_GPIO, "SDHC1_CD");
		gpio_direction_input(SDHC1_CD_GPIO);

		if (gpio_is_valid(sdhc1_wp_gpio)) {
			gpio_request(sdhc1_wp_gpio, "SDHC1_WP");
			gpio_direction_input(sdhc1_wp_gpio);
		}

		if (!(sdhc_activated & (0x01 << module)))
			gpio_request(sdhc1_pwre_gpio, "SDHC1_PWRE");

		gpio_direction_output(sdhc1_pwre_gpio, 0);
		break;
	case 1:
		/* SDHC2 */
#if defined(CONFIG_MMC_MXC_SELECT2)
		mxc_iomux_v3_setup_multiple_pads(armadillo400_sdhc2_pads,
						 ARRAY_SIZE(armadillo400_sdhc2_pads));

		/* Card-Detect pin */
		gpio_request(SDHC2_CD_GPIO, "SDHC2_CD");
		gpio_direction_input(SDHC2_CD_GPIO);

		/* Write-Protect pin */
		gpio_request(SDHC2_WP_GPIO, "SDHC2_WP");
		gpio_direction_input(SDHC2_WP_GPIO);

		/* No Power-Switch pin for SDHC2 */
#endif
		break;
	default:
		/* This ID is not supported */
		break;
	}

	sdhc_activated |= 0x01 << module;
}
EXPORT_SYMBOL(gpio_sdhc_active);

/*!
 * Inactivate SDHC
 *
 * @param module SDHC module number
 */
void gpio_sdhc_inactive(int module)
{
	unsigned sdhc1_pwre_gpio = -1;

	switch (module) {
	case 0:
	{
		struct pad_desc sdhc1_inactive[] = {
			MX25_PAD_SD1_CLK__GPIO_2_24(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_DOWN),
		};
		mxc_iomux_v3_setup_multiple_pads(sdhc1_inactive,
						 ARRAY_SIZE(sdhc1_inactive));
		/* ENGcm02759: See "Chip Errata for the i.MX25" for details. */
		gpio_direction_output(GPIO(2, 24), 0);

		/* SDHC1 */
		gpio_free(SDHC1_CD_GPIO);
		if (gpio_is_valid(sdhc1_wp_gpio))
			gpio_free(sdhc1_wp_gpio);

		/* Power down */
		/* Keep SDHC1_PWRE_GPIO while module is inactivated */
		switch (system_rev & ARMADILLO_400_BOARD_REV_MAJOR_MASK) {
		case ARMADILLO_400_BOARD_REV_B:
			sdhc1_pwre_gpio = SDHC1_PWRE_GPIO_REVB;
			break;
		default:
			pr_warning("Unknown Board Revision 0x%x\n", system_rev);
			/* fall through */
		case ARMADILLO_400_BOARD_REV_C:
			sdhc1_pwre_gpio = SDHC1_PWRE_GPIO_REVC;
			break;
		}

		gpio_set_value(sdhc1_pwre_gpio, 1);
		break;
	}
	case 1:
#if defined(CONFIG_MMC_MXC_SELECT2)
	{
#if defined(CONFIG_ARMADILLO400_CON9_18_SDHC2_CLK) || defined(CONFIG_ARMADILLO410_CON2_74_SDHC2_CLK)
		struct pad_desc sdhc2_inactive[] = {
			MX25_PAD_CSI_D7__GPIO_1_6(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_DOWN),
		};
		mxc_iomux_v3_setup_multiple_pads(sdhc2_inactive,
						 ARRAY_SIZE(sdhc2_inactive));
		/* ENGcm02759: See "Chip Errata for the i.MX25" for details. */
		gpio_direction_output(GPIO(1, 6), 0);
#endif

		gpio_free(SDHC2_CD_GPIO);
		gpio_free(SDHC2_WP_GPIO);
	}
#endif
		break;
	default:
		/* This ID is not supported */
		break;
	}
}
EXPORT_SYMBOL(gpio_sdhc_inactive);

/*
 * Probe for the card. If present the GPIO data would be set.
 */
unsigned int sdhc_get_card_det_status(struct device *dev)
{
	if (to_platform_device(dev)->id == 0)
		return gpio_get_value(SDHC1_CD_GPIO);
	else
		return gpio_get_value(SDHC2_CD_GPIO);
}
EXPORT_SYMBOL(sdhc_get_card_det_status);

int sdhc_get_write_protect(struct device *dev)
{
	if (to_platform_device(dev)->id == 0)
		return (gpio_is_valid(sdhc1_wp_gpio) &&
			gpio_get_value(sdhc1_wp_gpio));
	else
		return gpio_get_value(SDHC2_WP_GPIO);
}

/*!
 * Activate LCD
 */
void gpio_lcdc_active(void)
{
	mxc_iomux_v3_setup_multiple_pads(armadillo400_lcdc_pads,
					 ARRAY_SIZE(armadillo400_lcdc_pads));
}
EXPORT_SYMBOL(gpio_lcdc_active);

/*!
 * Inactivate LCD
 */
void gpio_lcdc_inactive(void)
{
}
EXPORT_SYMBOL(gpio_lcdc_inactive);

void board_power_lcd(int on, const char *mode)
{
	if (on && strcmp(mode, "VGG322423") == 0) {
		/* Use LCD_OE_ACD for reset pin */
		struct pad_desc oe_acd_pad = MX25_PAD_OE_ACD__GPIO_1_25(0);
		mxc_iomux_v3_setup_pad(&oe_acd_pad);

		gpio_request(GPIO(1, 25), "LCD_RESET");
		gpio_direction_output(GPIO(1, 25), 0);
		mdelay(1);
		gpio_set_value(GPIO(1, 25), 1);
	}
}
EXPORT_SYMBOL(board_power_lcd);

void gpio_lcd_bl_active(void)
{
	mxc_iomux_v3_setup_multiple_pads(armadillo400_lcd_bl_pads,
					 ARRAY_SIZE(armadillo400_lcd_bl_pads));
}
EXPORT_SYMBOL(gpio_lcd_bl_active);

void gpio_lcd_bl_inactive(void)
{
}
EXPORT_SYMBOL(gpio_lcd_bl_inactive);

void gpio_activate_audio_ports(void)
{
	mxc_iomux_v3_setup_multiple_pads(armadillo400_audio_pads,
					 ARRAY_SIZE(armadillo400_audio_pads));
}
EXPORT_SYMBOL(gpio_activate_audio_ports);

void gpio_inactive_audio_ports(void)
{
}
EXPORT_SYMBOL(gpio_inactive_audio_ports);

int headphone_det_status(void)
{
	return 1;
}
EXPORT_SYMBOL(headphone_det_status);

void armadillo400_gpio_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(armadillo400_gpio_led_pads,
					 ARRAY_SIZE(armadillo400_gpio_led_pads));

	mxc_iomux_v3_setup_multiple_pads(armadillo400_gpio_keys_pads,
					 ARRAY_SIZE(armadillo400_gpio_keys_pads));

	switch (system_rev & ARMADILLO_400_BOARD_REV_MAJOR_MASK) {
	case ARMADILLO_400_BOARD_REV_B:
		mxc_iomux_v3_setup_multiple_pads(
			armadillo400_ext_gpio_pads_revb,
			ARRAY_SIZE(armadillo400_ext_gpio_pads_revb));
		break;
	default:
		pr_warning("Unknown Board Revision 0x%x\n", system_rev);
		/* fall through */
	case ARMADILLO_400_BOARD_REV_C:
		mxc_iomux_v3_setup_multiple_pads(
			armadillo400_ext_gpio_pads_revc,
			ARRAY_SIZE(armadillo400_ext_gpio_pads_revc));
		break;
	}
}

void gpio_activate_pwm(int id)
{
	switch (id) {
	case 0:
		mxc_iomux_v3_setup_multiple_pads(armadillo400_pwm1_pads,
						 ARRAY_SIZE(armadillo400_pwm1_pads));
		break;
	case 1:
		mxc_iomux_v3_setup_multiple_pads(armadillo400_pwm2_pads,
						 ARRAY_SIZE(armadillo400_pwm2_pads));
		break;
	case 2:
		mxc_iomux_v3_setup_multiple_pads(armadillo400_pwm3_pads,
						 ARRAY_SIZE(armadillo400_pwm3_pads));
		break;
	case 3:
		mxc_iomux_v3_setup_multiple_pads(armadillo400_pwm4_pads,
						 ARRAY_SIZE(armadillo400_pwm4_pads));
		break;
	default:
		/* This ID is not supported */
		break;
	}
}
EXPORT_SYMBOL(gpio_activate_pwm);

void gpio_owire_active(void)
{
	mxc_iomux_v3_setup_multiple_pads(armadillo400_mxc_w1_pads,
					 ARRAY_SIZE(armadillo400_mxc_w1_pads));
}
EXPORT_SYMBOL(gpio_owire_active);

void gpio_owire_inactive(void)
{
}
EXPORT_SYMBOL(gpio_owire_inactive);

void gpio_flexcan_active(int id)
{
	switch (id) {
	case 0:
		mxc_iomux_v3_setup_multiple_pads(armadillo400_flexcan1_pads,
						 ARRAY_SIZE(armadillo400_flexcan1_pads));
		break;
	case 1:
		mxc_iomux_v3_setup_multiple_pads(armadillo400_flexcan2_pads,
						 ARRAY_SIZE(armadillo400_flexcan2_pads));
		break;
	default:
		/* This ID is not supported */
		break;
	}
}

void gpio_flexcan_inactive(int id)
{

}

void gpio_keypad_active(void)
{
	mxc_iomux_v3_setup_multiple_pads(armadillo440_keypad_pads,
					 ARRAY_SIZE(armadillo440_keypad_pads));
}
EXPORT_SYMBOL(gpio_keypad_active);

void gpio_keypad_inactive(void)
{
}
EXPORT_SYMBOL(gpio_keypad_inactive);

void gpio_spi1_cs_active(int cspi_mode, int chipselect)
{
	int __maybe_unused val = !(cspi_mode & SPI_CPOL);

	switch (chipselect) {
	case 0:
#if defined(CONFIG_ARMADILLO400_CON9_25_CSPI1_SS0) || defined(CONFIG_ARMADILLO410_CON2_69_CSPI1_SS0)
		gpio_set_value(GPIO(1, 16), val);
#endif
		break;
	case 1:
#if defined(CONFIG_ARMADILLO400_CON9_11_CSPI1_SS1) || defined(CONFIG_ARMADILLO410_CON2_81_CSPI1_SS1)
		gpio_set_value(GPIO(1, 17), val);
#endif
		break;
	case 2:
		break;
	case 3:
		break;
	default:
		break;
	}
}

void gpio_spi1_cs_inactive(int cspi_mode, int chipselect)
{
	int __maybe_unused val = !!(cspi_mode & SPI_CPOL);

	switch (chipselect) {
	case 0:
#if defined(CONFIG_ARMADILLO400_CON9_25_CSPI1_SS0) || defined(CONFIG_ARMADILLO410_CON2_69_CSPI1_SS0)
		gpio_set_value(GPIO(1, 16), val);
#endif
		break;
	case 1:
#if defined(CONFIG_ARMADILLO400_CON9_11_CSPI1_SS1) || defined(CONFIG_ARMADILLO410_CON2_81_CSPI1_SS1)
		gpio_set_value(GPIO(1, 17), val);
#endif
		break;
	case 2:
		break;
	case 3:
		break;
	default:
		break;
	}
}

void gpio_spi3_cs_active(int cspi_mode, int chipselect)
{
	int __maybe_unused val = !(cspi_mode & SPI_CPOL);

	switch (chipselect) {
	case 0:
#if defined(CONFIG_ARMADILLO400_CON9_16_CSPI3_SS0) || defined(CONFIG_ARMADILLO410_CON2_76_CSPI3_SS0)
		gpio_set_value(GPIO(1, 31), val);
#endif
		break;
	case 1:
#if defined(CONFIG_ARMADILLO400_CON9_18_CSPI3_SS1) || defined(CONFIG_ARMADILLO410_CON2_74_CSPI3_SS1)
		gpio_set_value(GPIO(1, 6), val);
#endif
		break;
	case 2:
#if defined(CONFIG_ARMADILLO400_CON9_15_CSPI3_SS2) || defined(CONFIG_ARMADILLO410_CON2_77_CSPI3_SS2)
		gpio_set_value(GPIO(1, 7), val);
#endif
		break;
	case 3:
#if defined(CONFIG_ARMADILLO400_CON9_17_CSPI3_SS3) || defined(CONFIG_ARMADILLO410_CON2_75_CSPI3_SS3)
		gpio_set_value(GPIO(4, 21), val);
#endif
		break;
	default:
		break;
	}
}

void gpio_spi3_cs_inactive(int cspi_mode, int chipselect)
{
	int __maybe_unused val = !!(cspi_mode & SPI_CPOL);

	switch (chipselect) {
	case 0:
#if defined(CONFIG_ARMADILLO400_CON9_16_CSPI3_SS0) || defined(CONFIG_ARMADILLO410_CON2_76_CSPI3_SS0)
		gpio_set_value(GPIO(1, 31), val);
#endif
		break;
	case 1:
#if defined(CONFIG_ARMADILLO400_CON9_18_CSPI3_SS1) || defined(CONFIG_ARMADILLO410_CON2_74_CSPI3_SS1)
		gpio_set_value(GPIO(1, 6), val);
#endif
		break;
	case 2:
#if defined(CONFIG_ARMADILLO400_CON9_15_CSPI3_SS2) || defined(CONFIG_ARMADILLO410_CON2_77_CSPI3_SS2)
		gpio_set_value(GPIO(1, 7), val);
#endif
		break;
	case 3:
#if defined(CONFIG_ARMADILLO400_CON9_17_CSPI3_SS3) || defined(CONFIG_ARMADILLO410_CON2_75_CSPI3_SS3)
		gpio_set_value(GPIO(4, 21), val);
#endif
		break;
	default:
		break;
	}
}
