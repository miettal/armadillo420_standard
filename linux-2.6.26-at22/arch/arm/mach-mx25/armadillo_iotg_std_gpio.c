/*
 * Copyright (C) 2014 Atmark Techno, Inc. All Rights Reserved.
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

static struct pad_desc armadillo_iotg_std_gpio_led_pads[] = {
	MX25_PAD_NFALE__GPIO_3_28, /* led2 */
	MX25_PAD_NFCLE__GPIO_3_29, /* led3 */
	MX25_PAD_BOOT_MODE0__GPIO_4_30, /* yellow led */
};

static struct pad_desc armadillo_iotg_std_gpio_keys_pads[] = {
	MX25_PAD_NFWP_B__GPIO_3_30, /* sw1 */
	/* sw2 and sw3 are connected to the i2c gpio expander */
};

static struct pad_desc armadillo_iotg_std_gpio_i2c1[] = {
	MX25_PAD_CSPI1_SS1__GPIO_1_17(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
	MX25_PAD_CSPI1_SCLK__GPIO_1_18(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
};

static struct pad_desc armadillo_iotg_std_gpio_i2c2[] = {
	MX25_PAD_KPP_COL1__GPIO_3_2(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
	MX25_PAD_KPP_COL0__GPIO_3_1(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
};

static struct pad_desc armadillo_iotg_std_gpio_sierra_wireless[] = {
	MX25_PAD_EXT_ARMCLK__GPIO_3_15, /* 3G_SYS_RESET_N_1.8 */
	MX25_PAD_CSPI1_SS0__GPIO_1_16(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),/* 3G_W_DISABLE_N_1.8 */
};


static struct pad_desc armadillo_iotg_std_gpio_temp_sensor[] = {
	MX25_PAD_CSPI1_RDY__GPIO_2_22(0), /* Temp_ALERT_N */
};

static struct pad_desc armadillo_iotg_std_addon_gpio_pads[] = {
#if defined(CONFIG_AIOTG_STD_CON1_3_CON2_24_GPIO1_2)
	MX25_PAD_GPIO_C__GPIO_C(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_DOWN),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_4_CON2_25_GPIO1_3)
	MX25_PAD_GPIO_D__GPIO_D(PAD_CTL_PKE | PAD_CTL_PUS_100K_DOWN),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_5_CON2_33_GPIO1_26)
	MX25_PAD_PWM__GPIO_1_26(0),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_6_CON2_32_GPIO3_14)
	MX25_PAD_RTCK__GPIO_3_14(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_DOWN),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_7_CON2_41_GPIO4_22)
	MX25_PAD_UART1_RXD__GPIO_4_22(PAD_CTL_PKE | PAD_CTL_PUS_100K_DOWN),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_8_CON2_40_GPIO4_23)
	MX25_PAD_UART1_TXD__GPIO_4_23(PAD_CTL_PKE | PAD_CTL_PUS_100K_DOWN),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_9_CON2_7_39_GPIO4_24)
	MX25_PAD_UART1_RTS__GPIO_4_24(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_DOWN),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_10_CON2_8_38_GPIO4_25)
	MX25_PAD_UART1_CTS__GPIO_4_25(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_DOWN),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_11_CON2_50_GPIO4_21)
	MX25_PAD_CSI_D9__GPIO_4_21(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_DOWN),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_12_CON2_16_37_GPIO1_27)
	MX25_PAD_CSI_D2__GPIO_1_27(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_DOWN),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_13_CON2_17_36_GPIO1_28)
	MX25_PAD_CSI_D3__GPIO_1_28(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_DOWN),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_14_CON2_12_18_35_GPIO1_29)
	MX25_PAD_CSI_D4__GPIO_1_29(PAD_CTL_HYS | PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_DOWN),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_15_CON2_13_19_34_GPIO1_30)
	MX25_PAD_CSI_D5__GPIO_1_30(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_DOWN),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_16_CON2_49_GPIO2_29)
	MX25_PAD_KPP_ROW0__GPIO_2_29(PAD_CTL_PKE | PAD_CTL_PUS_100K_DOWN),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_17_CON2_48_GPIO2_30)
	MX25_PAD_KPP_ROW1__GPIO_2_30(PAD_CTL_PKE | PAD_CTL_PUS_100K_DOWN),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_18_CON2_47_GPIO2_31)
	MX25_PAD_KPP_ROW2__GPIO_2_31(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_DOWN),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_19_CON2_46_GPIO3_0)
	MX25_PAD_KPP_ROW3__GPIO_3_0(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_DOWN),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_20_CON2_20_45_GPIO3_1)
	MX25_PAD_KPP_COL0__GPIO_3_1(PAD_CTL_PKE | PAD_CTL_PUS_100K_DOWN),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_21_CON2_21_44_GPIO3_2)
	MX25_PAD_KPP_COL1__GPIO_3_2(PAD_CTL_PKE | PAD_CTL_PUS_100K_DOWN),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_22_CON2_22_43_GPIO3_3)
	MX25_PAD_KPP_COL2__GPIO_3_3(PAD_CTL_PKE | PAD_CTL_PUS_100K_DOWN),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_23_CON2_23_42_GPIO3_4)
	MX25_PAD_KPP_COL3__GPIO_3_4(PAD_CTL_PKE | PAD_CTL_PUS_100K_DOWN),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_24_GPIO1_0)
	MX25_PAD_GPIO_A__GPIO_A(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_DOWN),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_25_GPIO1_1)
	MX25_PAD_GPIO_B__GPIO_B(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_DOWN),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_32_GPIO1_5)
	MX25_PAD_GPIO_F__GPIO_F(0),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_33_GPIO1_4)
	MX25_PAD_GPIO_E__GPIO_E(0),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_42_GPIO1_21)
	MX25_PAD_LD7__GPIO_1_21(0),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_43_GPIO1_20)
	MX25_PAD_LD6__GPIO_1_20(0),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_44_GPIO1_19)
	MX25_PAD_LD5__GPIO_1_19(0),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_45_GPIO2_19)
	MX25_PAD_LD4__GPIO_2_19(0),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_46_GPIO2_18)
	MX25_PAD_LD3__GPIO_2_18(0),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_47_GPIO2_17)
	MX25_PAD_LD2__GPIO_2_17(0),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_48_GPIO2_16)
	MX25_PAD_LD1__GPIO_2_16(0),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_49_GPIO2_15)
	MX25_PAD_LD0__GPIO_2_15(0),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_50_GPIO1_25)
	MX25_PAD_OE_ACD__GPIO_1_25(0),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_51_GPIO1_23)
	MX25_PAD_VSYNC__GPIO_1_23(0),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_52_GPIO1_22)
	MX25_PAD_HSYNC__GPIO_1_22(0),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_53_GPIO1_24)
	MX25_PAD_LSCLK__GPIO_1_24(0),
#endif
};

static struct pad_desc armadillo_iotg_std_gpio_ad_converter[] = {
	MX25_PAD_VSTBY_REQ__GPIO_3_17(0), /* VIN_ALERT_N */
};

static struct pad_desc armadillo_iotg_std_uart_pad =
	MX25_PAD_CTL_GRP_DSE_UART(PAD_CTL_DSE_STANDARD);

static struct pad_desc armadillo_iotg_std_uart1_pads[] = {
#if defined(CONFIG_AIOTG_STD_CON1_7_CON2_41_UART1_RXD)
	MX25_PAD_UART1_RXD__UART1_RXD(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_8_CON2_40_UART1_TXD)
	MX25_PAD_UART1_TXD__UART1_TXD(0),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_9_CON2_7_39_UART1_RTS)
	MX25_PAD_UART1_RTS__UART1_RTS(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_10_CON2_8_38_UART1_CTS)
	MX25_PAD_UART1_CTS__UART1_CTS(0),
#endif
};

static struct pad_desc armadillo_iotg_std_uart2_pads[] = {
	MX25_PAD_UART2_RXD__UART2_RXD(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
	MX25_PAD_UART2_TXD__UART2_TXD(0),
	MX25_PAD_UART2_RTS__UART2_RTS(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
	MX25_PAD_UART2_CTS__UART2_CTS(0),
	/* UART2_FORCEOFF */
	MX25_PAD_BOOT_MODE1__GPIO_4_31,
};

static struct pad_desc armadillo_iotg_std_uart3_pads[] = {
#if defined(CONFIG_AIOTG_STD_CON1_16_CON2_49_UART3_RXD)
	MX25_PAD_KPP_ROW0__UART3_RXD(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_17_CON2_48_UART3_TXD)
	MX25_PAD_KPP_ROW1__UART3_TXD(0),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_18_CON2_47_UART3_RTS)
	MX25_PAD_KPP_ROW2__UART3_RTS(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_19_CON2_46_UART3_CTS)
	MX25_PAD_KPP_ROW3__UART3_CTS(0),
#endif
};

static struct pad_desc armadillo_iotg_std_uart4_pads[] = {
#if defined(CONFIG_AIOTG_STD_CON1_20_CON2_20_45_UART4_RXD)
	MX25_PAD_KPP_COL0__UART4_RXD(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_21_CON2_21_44_UART4_TXD)
	MX25_PAD_KPP_COL1__UART4_TXD(0),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_22_CON2_22_43_UART4_RTS)
	MX25_PAD_KPP_COL2__UART4_RTS(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_23_CON2_23_42_UART4_CTS)
	MX25_PAD_KPP_COL3__UART4_CTS(0),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_41_UART4_RXD)
	MX25_PAD_LD8__UART4_RXD(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_40_UART4_TXD)
	MX25_PAD_LD9__UART4_TXD(0),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_39_UART4_RTS)
	MX25_PAD_LD10__UART4_RTS(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_38_UART4_CTS)
	MX25_PAD_LD11__UART4_CTS(0),
#endif
};

static struct pad_desc armadillo_iotg_std_uart5_pads[] = {
#if defined(CONFIG_AIOTG_STD_CON1_12_CON2_16_37_UART5_RXD)
	MX25_PAD_CSI_D2__UART5_RXD(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_13_CON2_17_36_UART5_TXD)
	MX25_PAD_CSI_D3__UART5_TXD(0),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_14_CON2_12_18_35_UART5_RTS)
	MX25_PAD_CSI_D4__UART5_RTS(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_15_CON2_13_19_34_UART5_CTS)
	MX25_PAD_CSI_D5__UART5_CTS(0),
#endif
};

#define I2C_PAD_CTRL (PAD_CTL_HYS | PAD_CTL_PKE | \
		      PAD_CTL_PUE | PAD_CTL_PUS_100K_UP | PAD_CTL_ODE)

static struct pad_desc armadillo_iotg_std_i2c1_pads[] = {
	MX25_PAD_I2C1_CLK__I2C1_CLK(PAD_CTL_PUS_100K_UP | PAD_CTL_ODE),
	MX25_PAD_I2C1_DAT__I2C1_DAT(PAD_CTL_PUS_100K_UP | PAD_CTL_ODE),
};

static struct pad_desc armadillo_iotg_std_i2c1_gpio_pads[] = {
	MX25_PAD_I2C1_CLK__GPIO_1_12(PAD_CTL_PUS_100K_UP),
	MX25_PAD_I2C1_DAT__GPIO_1_13(PAD_CTL_PUS_100K_UP),
};
#define I2C1_CLK_GPIO GPIO(1, 12)
#define I2C1_DAT_GPIO GPIO(1, 13)

static struct pad_desc __maybe_unused armadillo_iotg_std_i2c2_pads[] = {
#if  defined(CONFIG_AIOTG_STD_CON1_3_CON2_24_I2C2_SCL)
	MX25_PAD_GPIO_C__I2C2_SCL(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_22K_UP | PAD_CTL_ODE),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_4_CON2_25_I2C2_SDA)
	MX25_PAD_GPIO_D__I2C2_SDA(PAD_CTL_PKE | PAD_CTL_PUS_22K_UP | PAD_CTL_ODE),
#endif
};

static struct pad_desc __maybe_unused armadillo_iotg_std_i2c2_gpio_pads[] = {
#if defined(CONFIG_AIOTG_STD_CON1_3_CON2_24_I2C2_SCL)
	MX25_PAD_GPIO_C__GPIO_C(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_22K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_4_CON2_25_I2C2_SDA)
	MX25_PAD_GPIO_D__GPIO_D(PAD_CTL_PKE | PAD_CTL_PUS_22K_UP),
#endif
};

#define I2C2_CLK_GPIO GPIO(1, 2)
#define I2C2_DAT_GPIO GPIO(1, 3)

static struct pad_desc __maybe_unused armadillo_iotg_std_i2c3_pads[] = {
#if defined(CONFIG_AIOTG_STD_CON1_24_I2C3_SCL)
	MX25_PAD_GPIO_A__I2C3_SCL(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_22K_UP | PAD_CTL_ODE),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_25_I2C3_SDA)
	MX25_PAD_GPIO_B__I2C3_SDA(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_22K_UP | PAD_CTL_ODE),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_52_I2C3_SCL)
	MX25_PAD_HSYNC__I2C3_SCL(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_22K_UP | PAD_CTL_ODE),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_51_I2C3_SDA)
	MX25_PAD_VSYNC__I2C3_SDA(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_22K_UP | PAD_CTL_ODE),
#endif
};

static struct pad_desc __maybe_unused armadillo_iotg_std_i2c3_gpio_pads[] = {
#if defined(CONFIG_AIOTG_STD_CON1_24_I2C3_SCL)
	MX25_PAD_GPIO_A__GPIO_A(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_22K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_25_I2C3_SDA)
	MX25_PAD_GPIO_B__GPIO_B(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_22K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_52_I2C3_SCL)
	MX25_PAD_HSYNC__GPIO_1_22(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_22K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_51_I2C3_SDA)
	MX25_PAD_VSYNC__GPIO_1_23(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_22K_UP),
#endif
};

#if defined(CONFIG_AIOTG_STD_CON1_24_I2C3_SCL)
#define I2C3_CLK_GPIO GPIO(1, 0)
#else
#define I2C3_CLK_GPIO GPIO(1, 22)
#endif

#if defined(CONFIG_AIOTG_STD_CON1_25_I2C3_SDA)
#define I2C3_DAT_GPIO GPIO(1, 1)
#else
#define I2C3_DAT_GPIO GPIO(1, 23)
#endif

static struct pad_desc armadillo_iotg_std_spi2_pads[] = {
#if defined(CONFIG_AIOTG_STD_CON1_37_CSPI2_MOSI)
	MX25_PAD_LD12__CSPI2_MOSI(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_36_CSPI2_MISO)
	MX25_PAD_LD13__CSPI2_MISO(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_35_CSPI2_SCLK)
	MX25_PAD_LD14__CSPI2_SCLK(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_34_CSPI2_RDY)
	MX25_PAD_LD15__CSPI2_RDY(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_50_GPIO_SPI2_SS0)
	MX25_PAD_OE_ACD__GPIO_1_25(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_3_CON2_24_GPIO_SPI2_SS1)
	MX25_PAD_GPIO_C__GPIO_C(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
};

static struct pad_desc armadillo_iotg_std_spi3_pads[] = {
#if defined(CONFIG_AIOTG_STD_CON1_12_CON2_16_37_CSPI3_MOSI)
	MX25_PAD_CSI_D2__CSPI3_MOSI(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_13_CON2_17_36_CSPI3_MISO)
	MX25_PAD_CSI_D3__CSPI3_MISO(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_14_CON2_12_18_35_CSPI3_SCLK)
	MX25_PAD_CSI_D4__CSPI3_SCLK(PAD_CTL_HYS | PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_15_CON2_13_19_34_CSPI3_RDY)
	MX25_PAD_CSI_D5__CSPI3_RDY(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_4_CON2_25_GPIO_SPI3_SS0)
	MX25_PAD_GPIO_D__GPIO_D(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_11_CON2_50_GPIO_SPI3_SS1)
	MX25_PAD_CSI_D9__GPIO_4_21(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
};

#define FEC_PAD_CTRL1 (PAD_CTL_HYS | PAD_CTL_PUE | \
		      PAD_CTL_PKE)
#define FEC_PAD_CTRL2 (PAD_CTL_PUE)

static struct pad_desc armadillo_iotg_std_fec_pads[] = {
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

static struct pad_desc armadillo_iotg_std_usb_pads[] = {
	MX25_PAD_NFWE_B__GPIO_3_26,
};

static struct pad_desc armadillo_iotg_std_sdhc1_pads_revc[] = {
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

static struct pad_desc armadillo_iotg_std_sdhc2_pads[] = {
	MX25_PAD_CSI_D6__SD2_CMD(PAD_CTL_SRE_FAST),
	MX25_PAD_CSI_D7__SD2_CLK(PAD_CTL_SRE_FAST),
	MX25_PAD_CSI_MCLK__SD2_DATA0(PAD_CTL_SRE_FAST),
	MX25_PAD_CSI_VSYNC__SD2_DATA1(PAD_CTL_SRE_FAST),
	MX25_PAD_CSI_HSYNC__SD2_DATA2(PAD_CTL_SRE_FAST),
	MX25_PAD_CSI_PIXCLK__SD2_DATA3(PAD_CTL_HYS | PAD_CTL_SRE_FAST),
	MX25_PAD_CSI_D8__GPIO_1_7(PAD_CTL_HYS | PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP), /* CD CON2_77 */
	MX25_PAD_CLKO__GPIO_2_21(0), /* WP CON2_67 */
	MX25_PAD_DE_B__GPIO_2_20(0), /* SDHC2_PWRE */
	MX25_PAD_CTL_GRP_DSE_CSI(PAD_CTL_DSE_HIGH),
};

static struct pad_desc armadillo_iotg_std_pwm1_pads[] = {
#if defined(CONFIG_AIOTG_STD_CON1_5_CON2_33_PWMO1)
	MX25_PAD_PWM__PWM(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
};

static struct pad_desc armadillo_iotg_std_pwm2_pads[] = {
#if defined(CONFIG_AIOTG_STD_CON1_24_PWMO2)
	MX25_PAD_GPIO_A__PWM2(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
};

static struct pad_desc armadillo_iotg_std_pwm3_pads[] = {
#if defined(CONFIG_AIOTG_STD_CON1_25_PWMO3)
	MX25_PAD_GPIO_B__PWM3(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
};

static struct pad_desc armadillo_iotg_std_pwm4_pads[] = {
#if defined(CONFIG_AIOTG_STD_CON1_3_CON2_24_PWMO4)
	MX25_PAD_GPIO_C__PWM4(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP),
#endif
};

static struct pad_desc armadillo_iotg_std_mxc_w1_pads[] = {
#if defined(CONFIG_AIOTG_STD_CON1_6_CON2_32_W1)
	MX25_PAD_RTCK__OWIRE(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_22K_UP),
#endif
};

static struct pad_desc armadillo_iotg_std_flexcan1_pads[] = {
#if defined(CONFIG_AIOTG_STD_CON1_24_CAN1_TXCAN)
	MX25_PAD_GPIO_A__CAN1_TX(0),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_25_CAN1_RXCAN)
	MX25_PAD_GPIO_B__CAN1_RX(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
};

static struct pad_desc armadillo_iotg_std_flexcan2_pads[] = {
#if defined(CONFIG_AIOTG_STD_CON1_3_CON2_24_CAN2_TXCAN)
	MX25_PAD_GPIO_C__CAN2_TX(0),
#endif
#if defined(CONFIG_AIOTG_STD_CON1_4_CON2_25_CAN2_RXCAN)
	MX25_PAD_GPIO_D__CAN2_RX(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
#endif
};

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
	mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_fec_pads,
					 ARRAY_SIZE(armadillo_iotg_std_fec_pads));

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
		mxc_iomux_v3_setup_pad(&armadillo_iotg_std_uart_pad);

	/*
	 * Configure the IOMUX control registers for the UART signals
	 */
	switch (port) {
	case 0: /* UART1 IOMUX Configs */
		mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_uart1_pads,
						 ARRAY_SIZE(armadillo_iotg_std_uart1_pads));
		break;
	case 1: /* UART2 IOMUX Configs */
		mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_uart2_pads,
						 ARRAY_SIZE(armadillo_iotg_std_uart2_pads));
		if (!(gpio_uart_enabled & (1 << port)))
			gpio_request(UART2_FORCEOFF_GPIO, "UART2_FORCEOFF");
		gpio_direction_output(UART2_FORCEOFF_GPIO, 1);
		break;
	case 2: /* UART3 IOMUX Configs */
		mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_uart3_pads,
						 ARRAY_SIZE(armadillo_iotg_std_uart3_pads));
		break;
	case 3: /* UART4 IOMUX Configs */
		mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_uart4_pads,
						 ARRAY_SIZE(armadillo_iotg_std_uart4_pads));
		break;
	case 4: /* UART5 IOMUX Configs */
		mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_uart5_pads,
						 ARRAY_SIZE(armadillo_iotg_std_uart5_pads));
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
	case 1: /* UART2 */
		gpio_set_value(UART2_FORCEOFF_GPIO, 0);
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
		mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_i2c1_gpio_pads,
						 ARRAY_SIZE(armadillo_iotg_std_i2c1_gpio_pads));
		gpio_i2c_dummy_clock(I2C1_CLK_GPIO, I2C1_DAT_GPIO);

		mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_i2c1_pads,
						 ARRAY_SIZE(armadillo_iotg_std_i2c1_pads));

		break;
	case 1: /*I2C2*/
#if defined(CONFIG_I2C_MXC_SELECT2)
		mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_i2c2_gpio_pads,
						 ARRAY_SIZE(armadillo_iotg_std_i2c2_gpio_pads));
		gpio_i2c_dummy_clock(I2C2_CLK_GPIO, I2C2_DAT_GPIO);

		mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_i2c2_pads,
						 ARRAY_SIZE(armadillo_iotg_std_i2c2_pads));
#endif
		break;
	case 2: /*I2C3*/
#if defined(CONFIG_I2C_MXC_SELECT3)
		mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_i2c3_gpio_pads,
						 ARRAY_SIZE(armadillo_iotg_std_i2c3_gpio_pads));
		gpio_i2c_dummy_clock(I2C3_CLK_GPIO, I2C3_DAT_GPIO);

		mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_i2c3_pads,
						 ARRAY_SIZE(armadillo_iotg_std_i2c3_pads));
#endif
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(gpio_i2c_active);

void gpio_rtc_alarm_int_active(void)
{
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
	default:
		break;
	}
}
EXPORT_SYMBOL(gpio_i2c_inactive);

static void gpio_spi_initialize(int cspi_mod)
{
	switch (cspi_mod) {
	case 1:
		mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_spi2_pads,
						 ARRAY_SIZE(armadillo_iotg_std_spi2_pads));
#if defined(CONFIG_AIOTG_STD_CON1_50_GPIO_SPI2_SS0)
		gpio_request(GPIO(1, 25), "CSPI2_SS0");
		gpio_direction_output(GPIO(1, 25), 1);
#endif
#if defined(CONFIG_AIOTG_STD_CON1_3_CON2_24_GPIO_SPI2_SS1)
		gpio_request(GPIO(1, 2), "CSPI2_SS1");
		gpio_direction_output(GPIO(1, 2), 1);
#endif
		break;
	case 2:
		mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_spi3_pads,
						 ARRAY_SIZE(armadillo_iotg_std_spi3_pads));
#if defined(CONFIG_AIOTG_STD_CON1_4_CON2_25_GPIO_SPI3_SS0)
		gpio_request(GPIO(1, 3), "CSPI3_SS0");
		gpio_direction_output(GPIO(1, 3), 1);
#endif
#if defined(CONFIG_AIOTG_STD_CON1_11_CON2_50_GPIO_SPI3_SS1)
		gpio_request(GPIO(4, 21), "CSPI3_SS1");
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
	case 1:
#if defined(CONFIG_AIOTG_STD_CON1_50_GPIO_SPI2_SS0)
		gpio_set_value(GPIO(1, 25), 1);
#endif
#if defined(CONFIG_AIOTG_STD_CON1_3_CON2_24_GPIO_SPI2_SS1)
		gpio_set_value(GPIO(1, 2), 1);
#endif
		break;
	case 2:
#if defined(CONFIG_AIOTG_STD_CON1_4_CON2_25_GPIO_SPI3_SS0)
		gpio_set_value(GPIO(1, 3), 1);
#endif
#if defined(CONFIG_AIOTG_STD_CON1_11_CON2_50_GPIO_SPI3_SS1)
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
	case 1:
#if defined(CONFIG_AIOTG_STD_CON1_50_GPIO_SPI2_SS0)
		gpio_set_value(GPIO(1, 25), 0);
#endif
#if defined(CONFIG_AIOTG_STD_CON1_3_CON2_24_GPIO_SPI2_SS1)
		gpio_set_value(GPIO(1, 2), 0);
#endif
		break;
	case 2:
#if defined(CONFIG_AIOTG_STD_CON1_4_CON2_25_GPIO_SPI3_SS0)
		gpio_set_value(GPIO(1, 3), 0);
#endif
#if defined(CONFIG_AIOTG_STD_CON1_11_CON2_50_GPIO_SPI3_SS1)
		gpio_set_value(GPIO(4, 21), 0);
#endif
		break;
	default:
		WARN_ON(1);
		break;
	}
}
EXPORT_SYMBOL(gpio_spi_inactive);

void gpio_spi2_cs_active(int cspi_mode, int chipselect)
{
	int __maybe_unused val = !(cspi_mode & SPI_CPOL);

	switch (chipselect) {
	case 0:
#if defined(CONFIG_AIOTG_STD_CON1_50_GPIO_SPI2_SS0)
		gpio_set_value(GPIO(1, 25), val);
#endif
		break;
	case 1:
#if defined(CONFIG_AIOTG_STD_CON1_3_CON2_24_GPIO_SPI2_SS1)
		gpio_set_value(GPIO(1, 2), val);
#endif
		break;
	default:
		break;
	}
}

void gpio_spi2_cs_inactive(int cspi_mode, int chipselect)
{
	int __maybe_unused val = !!(cspi_mode & SPI_CPOL);

	switch (chipselect) {
	case 0:
#if defined(CONFIG_AIOTG_STD_CON1_50_GPIO_SPI2_SS0)
		gpio_set_value(GPIO(1, 25), val);
#endif
		break;
	case 1:
#if defined(CONFIG_AIOTG_STD_CON1_3_CON2_24_GPIO_SPI2_SS1)
		gpio_set_value(GPIO(1, 2), val);
#endif
	default:
		break;
	}
}

void gpio_spi3_cs_active(int cspi_mode, int chipselect)
{
	int __maybe_unused val = !(cspi_mode & SPI_CPOL);

	switch (chipselect) {
	case 0:
#if defined(CONFIG_AIOTG_STD_CON1_4_CON2_25_GPIO_SPI3_SS0)
		gpio_set_value(GPIO(1, 3), val);
#endif
		break;
	case 1:
#if defined(CONFIG_AIOTG_STD_CON1_11_CON2_50_GPIO_SPI3_SS1)
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
#if defined(CONFIG_AIOTG_STD_CON1_4_CON2_25_GPIO_SPI3_SS0)
		gpio_set_value(GPIO(1, 3), val);
#endif
		break;
	case 1:
#if defined(CONFIG_AIOTG_STD_CON1_11_CON2_50_GPIO_SPI3_SS1)
		gpio_set_value(GPIO(4, 21), val);
#endif
	default:
		break;
	}
}

static int usb_gpio_use_count = 0;

static int gpio_usb_request(void)
{
	if (!usb_gpio_use_count) {
		mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_usb_pads,
						 ARRAY_SIZE(armadillo_iotg_std_usb_pads));

		gpio_request(USB_PWREN_GPIO, "USB_PWREN");
		gpio_direction_output(USB_PWREN_GPIO, USB_PWREN_OFF);

		gpio_request(EXT_USB_SEL_GPIO, "EXT_USB_SEL");
		gpio_direction_output(EXT_USB_SEL_GPIO, CONFIG_AIOTG_STD_IS_USB_CON1);
	}
	usb_gpio_use_count++;

	return 0;
}

static void usb_gpio_free(void)
{
	if (usb_gpio_use_count > 0 && !(--usb_gpio_use_count)) {
		gpio_free(USB_PWREN_GPIO);
		gpio_free(EXT_USB_SEL_GPIO);
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
		mxc_iomux_v3_setup_multiple_pads(
			armadillo_iotg_std_sdhc1_pads_revc,
			ARRAY_SIZE(armadillo_iotg_std_sdhc1_pads_revc));
		sdhc1_pwre_gpio = SDHC1_PWRE_GPIO_REVC;

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
		mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_sdhc2_pads,
						 ARRAY_SIZE(armadillo_iotg_std_sdhc2_pads));

		/* Card-Detect pin */
		gpio_request(AIOTG_STD_SDHC2_CD_GPIO, "SDHC2_CD");
		gpio_direction_input(AIOTG_STD_SDHC2_CD_GPIO);

		/* Write-Protect pin */
		gpio_request(AIOTG_STD_SDHC2_WP_GPIO, "SDHC2_WP");
		gpio_direction_input(AIOTG_STD_SDHC2_WP_GPIO);

		gpio_request(AIOTG_STD_SDHC2_SDHC2_PWRE, "SDHC2_PWRE");

		/* SDHC2 Poewer off */
		gpio_direction_output(AIOTG_STD_SDHC2_SDHC2_PWRE, 0);

		/* select SD/AWLAN */
		gpio_request(AIOTG_STD_SD_AWLAN_SEL, "SD_AWLAN_SEL");
		gpio_direction_output(AIOTG_STD_SD_AWLAN_SEL,
				      CONFIG_AIOTG_STD_IS_ESDHC2_SD_AWLAN);

		/* SDHC2 Power on */
		gpio_direction_output(AIOTG_STD_SDHC2_SDHC2_PWRE, 1);

		gpio_export(AIOTG_STD_SDHC2_SDHC2_PWRE, false);
		gpio_export(AIOTG_STD_SD_AWLAN_SEL, false);
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
		sdhc1_pwre_gpio = SDHC1_PWRE_GPIO_REVC;
		gpio_set_value(sdhc1_pwre_gpio, 1);
		break;
	}
	case 1:
	{
		struct pad_desc sdhc2_inactive[] = {
			MX25_PAD_CSI_D7__GPIO_1_6(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_DOWN),
		};
		mxc_iomux_v3_setup_multiple_pads(sdhc2_inactive,
						 ARRAY_SIZE(sdhc2_inactive));
		/* ENGcm02759: See "Chip Errata for the i.MX25" for details. */
		gpio_direction_output(GPIO(1, 6), 0);

		if (gpio_is_valid(AIOTG_STD_SDHC2_CD_GPIO))
			gpio_free(AIOTG_STD_SDHC2_CD_GPIO);
		if (gpio_is_valid(AIOTG_STD_SDHC2_WP_GPIO))
			gpio_free(AIOTG_STD_SDHC2_WP_GPIO);
		gpio_set_value(AIOTG_STD_SDHC2_SDHC2_PWRE, 0);
		break;
	}
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
		return gpio_get_value(AIOTG_STD_SDHC2_CD_GPIO);
}
EXPORT_SYMBOL(sdhc_get_card_det_status);

int sdhc_get_write_protect(struct device *dev)
{
	if (to_platform_device(dev)->id == 0)
		return (gpio_is_valid(sdhc1_wp_gpio) &&
			gpio_get_value(sdhc1_wp_gpio));
	else
		return gpio_get_value(AIOTG_STD_SDHC2_WP_GPIO);
}

void gpio_sierra_wireless_active(void)
{
	gpio_request(AIOTG_STD_RESET_N_3G_GPIO, "RESET_N_3G");
	gpio_request(AIOTG_STD_W_DISABLE_3G_GPIO, "W_DISABLE_3G");

	gpio_direction_output(AIOTG_STD_RESET_N_3G_GPIO, 0);
	mdelay(100);
	gpio_direction_output(AIOTG_STD_RESET_N_3G_GPIO, 1);
	gpio_direction_output(AIOTG_STD_W_DISABLE_3G_GPIO, 1);

	gpio_export(AIOTG_STD_RESET_N_3G_GPIO, false);
	gpio_export(AIOTG_STD_W_DISABLE_3G_GPIO, false);
}
EXPORT_SYMBOL(gpio_sierra_wireless_active);

void gpio_sierra_wireless_inactive(void)
{
	gpio_direction_output(AIOTG_STD_RESET_N_3G_GPIO, 0);
	gpio_direction_output(AIOTG_STD_W_DISABLE_3G_GPIO, 0);
}
EXPORT_SYMBOL(gpio_sierra_wireless_inactive);

void gpio_temp_sensor_active(void)
{
	gpio_request(AIOTG_STD_TEMP_SENSOR_OS_GPIO, "TEMP_ALERT_N");
	gpio_direction_input(AIOTG_STD_TEMP_SENSOR_OS_GPIO);
	gpio_export(AIOTG_STD_TEMP_SENSOR_OS_GPIO, false);
}
EXPORT_SYMBOL(gpio_temp_sensor_active);

void gpio_ad_converter_active(void)
{
	gpio_request(AIOTG_STD_AD_CONVERTER_ALERT_GPIO, "VIN_ALERT_N");
	gpio_direction_input(AIOTG_STD_AD_CONVERTER_ALERT_GPIO);
	gpio_export(AIOTG_STD_AD_CONVERTER_ALERT_GPIO, false);
}
EXPORT_SYMBOL(gpio_ad_converter_active);

void armadillo_iotg_std_gpio_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_gpio_led_pads,
					 ARRAY_SIZE(armadillo_iotg_std_gpio_led_pads));
	mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_gpio_keys_pads,
					 ARRAY_SIZE(armadillo_iotg_std_gpio_keys_pads));

	mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_gpio_i2c1,
					 ARRAY_SIZE(armadillo_iotg_std_gpio_i2c1));
	mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_gpio_i2c2,
					 ARRAY_SIZE(armadillo_iotg_std_gpio_i2c2));

	mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_gpio_sierra_wireless,
					 ARRAY_SIZE(armadillo_iotg_std_gpio_sierra_wireless));

	mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_gpio_temp_sensor,
					 ARRAY_SIZE(armadillo_iotg_std_gpio_temp_sensor));
	mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_gpio_ad_converter,
					 ARRAY_SIZE(armadillo_iotg_std_gpio_ad_converter));
}

void armadillo_iotg_std_addon_gpio_init(struct mxc_ext_gpio *ext_gpios, int nr_ext_gpios)
{
	mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_addon_gpio_pads,
					 ARRAY_SIZE(armadillo_iotg_std_addon_gpio_pads));

	gpio_activate_ext_gpio(ext_gpios, nr_ext_gpios);
}

void gpio_activate_pwm(int id)
{
	switch (id) {
	case 0:
		mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_pwm1_pads,
						 ARRAY_SIZE(armadillo_iotg_std_pwm1_pads));
		break;
	case 1:
		mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_pwm2_pads,
						 ARRAY_SIZE(armadillo_iotg_std_pwm2_pads));
		break;
	case 2:
		mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_pwm3_pads,
						 ARRAY_SIZE(armadillo_iotg_std_pwm3_pads));
		break;
	case 3:
		mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_pwm4_pads,
						 ARRAY_SIZE(armadillo_iotg_std_pwm4_pads));
		break;
	default:
		/* This ID is not supported */
		break;
	}
}
EXPORT_SYMBOL(gpio_activate_pwm);

void gpio_owire_active(void)
{
	mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_mxc_w1_pads,
					 ARRAY_SIZE(armadillo_iotg_std_mxc_w1_pads));
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
		mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_flexcan1_pads,
						 ARRAY_SIZE(armadillo_iotg_std_flexcan1_pads));
		break;
	case 1:
		mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_flexcan2_pads,
						 ARRAY_SIZE(armadillo_iotg_std_flexcan2_pads));
		break;
	default:
		/* This ID is not supported */
		break;
	}
}

void gpio_flexcan_inactive(int id)
{

}
