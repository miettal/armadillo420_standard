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
#include <linux/gpio_keys.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>

#include <asm/arch/common.h>
#include <asm/arch/iomux-mx25.h>
#include <asm/arch/mxc_uart.h>
#include <asm/arch/spba.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#include "board.h"
#include "devices.h"
#include "serial.h"
#include "armadillo_iotg_std_addon.h"

struct aisa_vendor_name
{
        u16 vendor;
        const char *name;
};
#define VENDOR_NAME(v, n) { AISA_VENDOR_ID_##v, n }

static struct aisa_vendor_name vendor_names[] = {
	VENDOR_NAME(ATMARK_TECHNO, "Atmark Techno"),
};

static const char *unknownvendorname = "Unknown Vendor";

struct aisa_product_name
{
        u16 vendor;
        u16 product;
        const char *name;
};
#define PRODUCT_NAME(v, p, n) { AISA_VENDOR_ID_##v, \
				AISA_PRODUCT_ID_##v##_##p, n }

static struct aisa_product_name product_names[] = {
	PRODUCT_NAME(ATMARK_TECHNO, WI_SUN, "Wi-SUN"),
	PRODUCT_NAME(ATMARK_TECHNO, EN_OCEAN, "EnOcean"),
	PRODUCT_NAME(ATMARK_TECHNO, SERIAL, "RS485/RS422/RS232C"),
	PRODUCT_NAME(ATMARK_TECHNO, DIDOAD, "DI/DO/AD"),
	PRODUCT_NAME(ATMARK_TECHNO, BLE, "Bluetooth Low Energy"),
	PRODUCT_NAME(ATMARK_TECHNO, CAN, "Can"),
	PRODUCT_NAME(ATMARK_TECHNO, ZIGBEE, "ZigBee"),
	PRODUCT_NAME(ATMARK_TECHNO, RS232C, "RS232C"),
};

static const char *unknownproductname = "Unknown Product";

static const char *aisa_get_vendor_name(u16 vendor)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(vendor_names); i++)
                if (vendor_names[i].vendor == vendor) 
                        return vendor_names[i].name;

        return unknownvendorname;
}

static const char *aisa_get_product_name(u16 vendor, u16 product)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(product_names); i++) {
                if ((product_names[i].vendor == vendor) &&
		    (product_names[i].product == product)) 
                        return product_names[i].name;
	}

        return unknownproductname;
}

static int aisa_eeprom_read_word_swapped(u16 addr, u16 command)
{
	struct i2c_adapter *adap;
	union i2c_smbus_data data;
	int status;

	adap = i2c_get_adapter(AIOTG_STD_GPIO_I2C2_ID);
	if (!adap) {
		pr_err("failed to get i2c adapter\n");
		return -EINVAL;
	}

	status = i2c_smbus_xfer(adap, addr, 0, I2C_SMBUS_READ,
				command, I2C_SMBUS_WORD_DATA, &data);

	i2c_put_adapter(adap);

	return (status < 0) ? status : swab16(data.word);
}

static void aisa_gpio_request(unsigned gpio, const char *label)
{
	int ret;

	ret = gpio_request(gpio, label);
	if (ret)
		pr_err(KERN_ERR "failed to request gpio %d\n", gpio);
}

/*
 * platform devices
 */
static void aisa_uart1_register(int hardware_flow)
{
	static uart_mxc_port *port;

	if (!CONFIG_SERIAL_MXC_ENABLED1) {
		port = mxc_uart_device1.dev.platform_data;
		port->enabled = 1;
		port->hardware_flow = !!hardware_flow;
		port->wakeup = 1;

		platform_device_register(&mxc_uart_device1);
	}
}

static void aisa_uart1_rs485_register(int rs485_enable, void (*rs485_duplex)(int))
{
	static uart_mxc_port *port;

	if (!CONFIG_SERIAL_MXC_ENABLED1) {
		port = mxc_uart_device1.dev.platform_data;
		port->enabled = 1;
		port->hardware_flow = 1;
		port->wakeup = 1;
		if (rs485_enable) {
			port->port.rs485.flags = (SER_RS485_ENABLED |
						  SER_RS485_RTS_ON_SEND);
			port->rs485_duplex = rs485_duplex;
		}

		platform_device_register(&mxc_uart_device1);
	}
}

static void aisa_uart4_register(int hardware_flow)
{
	static uart_mxc_port *port;

	if (!CONFIG_SERIAL_MXC_ENABLED4) {
		port = mxc_uart_device4.dev.platform_data;
		port->enabled = 1;
		port->hardware_flow = !!hardware_flow;
		port->wakeup = 1;

#if UART4_DMA_ENABLE == 1
		spba_take_ownership(UART4_SHARED_PERI,
				    (SPBA_MASTER_A | SPBA_MASTER_C));
#else
		spba_take_ownership(UART4_SHARED_PERI, SPBA_MASTER_A);
#endif
		platform_device_register(&mxc_uart_device4);
	}
}

static void aisa_uart4_rs485_register(int rs485_enable, void (*rs485_duplex)(int))
{
	static uart_mxc_port *port;

	if (!CONFIG_SERIAL_MXC_ENABLED4) {
		port = mxc_uart_device4.dev.platform_data;
		port->enabled = 1;
		port->hardware_flow = 1;
		port->wakeup = 1;
		if (rs485_enable) {
			port->port.rs485.flags = (SER_RS485_ENABLED |
						  SER_RS485_RTS_ON_SEND);
			port->rs485_duplex = rs485_duplex;
		}

#if UART4_DMA_ENABLE == 1
		spba_take_ownership(UART4_SHARED_PERI,
				    (SPBA_MASTER_A | SPBA_MASTER_C));
#else
		spba_take_ownership(UART4_SHARED_PERI, SPBA_MASTER_A);
#endif
		platform_device_register(&mxc_uart_device4);
	}
}

/*
 * Atmark Techno: Wi-SUN
 */
#define BP35A1_RESET_ASSERT	(0)
#define BP35A1_RESET_DEASSERT	(1)

static void aisa_wi_sun_setup(int reset, int nmix)
{
	/* refered: reference circuit */
	gpio_direction_output(nmix, 1); /* always high */

	/* reset */
	gpio_direction_output(reset, BP35A1_RESET_ASSERT);
	ndelay(500);
	gpio_direction_output(reset, BP35A1_RESET_DEASSERT);
}

static void aisa_setup_atmark_techno_wi_sun_con1(u16 revision __maybe_unused)
{
	struct pad_desc pads[] = {
		MX25_PAD_LD11__UART4_CTS(0), /* CON1_38 */
		MX25_PAD_LD10__UART4_RTS(PAD_CTL_PKE | PAD_CTL_PUE |
					 PAD_CTL_PUS_100K_DOWN), /* CON1_39 */
		MX25_PAD_LD9__UART4_TXD(0), /* CON1_40 */
		MX25_PAD_LD8__UART4_RXD(PAD_CTL_PKE | PAD_CTL_PUE |
					PAD_CTL_PUS_100K_UP), /* CON1_41 */
		MX25_PAD_LD7__GPIO_1_21(0), /* CON1_42 */
		MX25_PAD_LD6__GPIO_1_20(0), /* CON1_43 */
	};

	mxc_iomux_v3_setup_multiple_pads(pads, ARRAY_SIZE(pads));
	aisa_uart4_register(1);

	aisa_gpio_request(GPIO(1, 21), "BP35A1_RESET_CON1");
	aisa_gpio_request(GPIO(1, 20), "BP35A1_NMIX_CON1");
	aisa_wi_sun_setup(GPIO(1, 21), GPIO(1, 20));
}

static void aisa_setup_atmark_techno_wi_sun_con2(u16 revision __maybe_unused)
{
	struct pad_desc pads[] = {
		MX25_PAD_UART1_CTS__UART1_CTS(0), /* CON2_38 */
		MX25_PAD_UART1_RTS__UART1_RTS(PAD_CTL_PKE | PAD_CTL_PUE |
					      PAD_CTL_PUS_100K_DOWN), /* CON2_39 */
		MX25_PAD_UART1_TXD__UART1_TXD(0), /* CON2_40 */
		MX25_PAD_UART1_RXD__UART1_RXD(PAD_CTL_PKE |
					      PAD_CTL_PUS_100K_UP), /* CON2_41 */
		MX25_PAD_KPP_COL3__GPIO_3_4(0), /* CON2_42 */
		MX25_PAD_KPP_COL2__GPIO_3_3(0), /* CON2_43 */
	};

	mxc_iomux_v3_setup_multiple_pads(pads, ARRAY_SIZE(pads));
	aisa_uart1_register(1);

	aisa_gpio_request(GPIO(3, 4), "BP35A1_RESET_CON2");
	aisa_gpio_request(GPIO(3, 3), "BP35A1_NMIX_CON2");
	aisa_wi_sun_setup(GPIO(3, 4), GPIO(3, 3));
}

/*
 * Atmark Techno: EnOcean
 */
#define BP35A3_RESET_ASSERT	(1)
#define BP35A3_RESET_DEASSERT	(0)

static void aisa_en_ocean_setup(int reset)
{
	/* reset */
	gpio_direction_output(reset, BP35A3_RESET_ASSERT);
	mdelay(10); /* FIXME: reset period isn't specified in its data sheet. */
	gpio_direction_output(reset, BP35A3_RESET_DEASSERT);
}

static void aisa_setup_atmark_techno_en_ocean_con1(u16 revision __maybe_unused)
{
	struct pad_desc pads[] = {
		MX25_PAD_LD9__UART4_TXD(0), /* CON1_40 */
		MX25_PAD_LD8__UART4_RXD(PAD_CTL_PKE | PAD_CTL_PUE |
					PAD_CTL_PUS_100K_UP), /* CON1_41 */
		MX25_PAD_LD7__GPIO_1_21(0), /* CON1_42 */
	};

	mxc_iomux_v3_setup_multiple_pads(pads, ARRAY_SIZE(pads));
	aisa_uart4_register(0);

	aisa_gpio_request(GPIO(1, 21), "BP35A3_RESET_CON1");
	aisa_en_ocean_setup(GPIO(1, 21));
}

static void aisa_setup_atmark_techno_en_ocean_con2(u16 revision __maybe_unused)
{
	struct pad_desc pads[] = {
		MX25_PAD_UART1_TXD__UART1_TXD(0), /* CON2_40 */
		MX25_PAD_UART1_RXD__UART1_RXD(PAD_CTL_PKE |
					      PAD_CTL_PUS_100K_UP), /* CON2_41 */
		MX25_PAD_KPP_COL3__GPIO_3_4(0), /* CON2_42 */
	};

	mxc_iomux_v3_setup_multiple_pads(pads, ARRAY_SIZE(pads));
	aisa_uart1_register(0);

	aisa_gpio_request(GPIO(3, 4), "BP35A3_RESET_CON2");
	aisa_en_ocean_setup(GPIO(3, 4));
}

/*
 * Atmark Techno: BLE(Bluetooth Low Energy)
 */
#define RN4020_COMMAND_MODE	(1)
#define RN4020_MLDP_MODE	(0)
#define RN4020_WAKE_SW_ASSERT	(1)
#define RN4020_WAKE_SW_DEASSERT	(0)
#define RN4020_WAKE_HW_ASSERT	(1)
#define RN4020_WAKE_HW_DEASSERT	(0)

static void aisa_ble_setup(int cmd_mldp, int wake_sw, int wake_hw)
{
	gpio_direction_output(wake_sw, RN4020_WAKE_SW_ASSERT);
	gpio_direction_output(wake_hw, RN4020_WAKE_HW_DEASSERT);
	gpio_direction_output(cmd_mldp, RN4020_COMMAND_MODE);

	gpio_export(cmd_mldp, false);
	gpio_export(wake_sw, false);
	gpio_export(wake_hw, false);
}

static void aisa_setup_atmark_techno_ble_con1(u16 revision __maybe_unused)
{
	struct pad_desc pads[] = {
		MX25_PAD_LD11__UART4_CTS(0), /* CON1_38 */
		MX25_PAD_LD10__UART4_RTS(PAD_CTL_PKE | PAD_CTL_PUE |
					 PAD_CTL_PUS_100K_DOWN), /* CON1_39 */
		MX25_PAD_LD9__UART4_TXD(0), /* CON1_40 */
		MX25_PAD_LD8__UART4_RXD(PAD_CTL_PKE | PAD_CTL_PUE |
					PAD_CTL_PUS_100K_UP), /* CON1_41 */
		MX25_PAD_LD7__GPIO_1_21(0), /* CON1_42 */
		MX25_PAD_LD6__GPIO_1_20(0), /* CON1_43 */
		MX25_PAD_LD3__GPIO_2_18(0), /* CON1_46 */
	};

	mxc_iomux_v3_setup_multiple_pads(pads, ARRAY_SIZE(pads));
	aisa_uart4_register(1);

	aisa_gpio_request(GPIO(2, 18), "RN4020_CMDMLDP_CON1");
	aisa_gpio_request(GPIO(1, 20), "RN4020_WAKE_SW_CON1");
	aisa_gpio_request(GPIO(1, 21), "RN4020_WAKE_HW_CON1");
	aisa_ble_setup(GPIO(2, 18), GPIO(1, 20), GPIO(1, 21));
}

static void aisa_setup_atmark_techno_ble_con2(u16 revision __maybe_unused)
{
	struct pad_desc pads[] = {
		MX25_PAD_UART1_CTS__UART1_CTS(0), /* CON2_38 */
		MX25_PAD_UART1_RTS__UART1_RTS(PAD_CTL_PKE | PAD_CTL_PUE |
					      PAD_CTL_PUS_100K_DOWN), /* CON2_39 */
		MX25_PAD_UART1_TXD__UART1_TXD(0), /* CON2_40 */
		MX25_PAD_UART1_RXD__UART1_RXD(PAD_CTL_PKE |
					      PAD_CTL_PUS_100K_UP), /* CON2_41 */
		MX25_PAD_KPP_COL3__GPIO_3_4(0), /* CON2_42 */
		MX25_PAD_KPP_COL2__GPIO_3_3(0), /* CON2_43 */
		MX25_PAD_KPP_ROW3__GPIO_3_0(0), /* CON2_46 */
	};

	mxc_iomux_v3_setup_multiple_pads(pads, ARRAY_SIZE(pads));
	aisa_uart1_register(1);

	aisa_gpio_request(GPIO(3, 0), "RN4020_CMDMLDP_CON2");
	aisa_gpio_request(GPIO(3, 3), "RN4020_WAKE_SW_CON2");
	aisa_gpio_request(GPIO(3, 4), "RN4020_WAKE_HW_CON2");
	aisa_ble_setup(GPIO(3, 0), GPIO(3, 3), GPIO(3, 4));
}

/*
 * Atmark Techno: RS485/RS232C/RS422
 */
#define XR3160_MODE_RS485	(1)
#define XR3160_MODE_RS232	(0)
#define XR3160_DUPLEX_FULL	(1)
#define XR3160_DUPLEX_HALF	(0)
#define ADUM1402_VE1_ENABLE	(1)
#define ADUM1402_VE1_DISABLE	(0)

static void xr3160_duplex_con1(int duplex)
{
	if (duplex == RS485_DUPLEX_HALF)
		gpio_direction_output(GPIO(1, 5), XR3160_DUPLEX_HALF);
	else
		gpio_direction_output(GPIO(1, 5), XR3160_DUPLEX_FULL);
}

static void xr3160_duplex_con2(int duplex)
{
	if (duplex == RS485_DUPLEX_HALF)
		gpio_direction_output(GPIO(3, 14), XR3160_DUPLEX_HALF);
	else
		gpio_direction_output(GPIO(3, 14), XR3160_DUPLEX_FULL);
}

static void aisa_serial_setup(int mode, int duplex, int ve1)
{
	gpio_direction_input(mode);
	gpio_direction_output(ve1, ADUM1402_VE1_ENABLE);

	gpio_export(mode, false);
	gpio_export(ve1, false);
}

static void aisa_setup_atmark_techno_serial_con1(u16 revision __maybe_unused)
{
	struct pad_desc pads[] = {
		MX25_PAD_GPIO_F__GPIO_F(0), /* CON1_32(GPIO1_5) */
		MX25_PAD_LD11__UART4_CTS(0), /* CON1_38 */
		MX25_PAD_LD10__UART4_RTS(PAD_CTL_PKE | PAD_CTL_PUE |
					 PAD_CTL_PUS_100K_UP), /* CON1_39 */
		MX25_PAD_LD9__UART4_TXD(0), /* CON1_40 */
		MX25_PAD_LD8__UART4_RXD(PAD_CTL_PKE | PAD_CTL_PUE |
					PAD_CTL_PUS_100K_UP), /* CON1_41 */
		MX25_PAD_LD7__GPIO_1_21(PAD_CTL_PKE | PAD_CTL_PUE |
					PAD_CTL_PUS_100K_UP), /* CON1_42 */
		MX25_PAD_LD6__GPIO_1_20(0), /* CON1_43 */
	};

	mxc_iomux_v3_setup_multiple_pads(pads, ARRAY_SIZE(pads));

	aisa_gpio_request(GPIO(1, 21), "XR3160_MODE_CON1");
	aisa_gpio_request(GPIO(1, 5),  "XR3160_DUPLEX_CON1");
	aisa_gpio_request(GPIO(1, 20), "ADUM1402_VE1_CON1");
	aisa_serial_setup(GPIO(1, 21), GPIO(1, 5), GPIO(1, 20));

	aisa_uart4_rs485_register(gpio_get_value(GPIO(1, 21)),
				  xr3160_duplex_con1);
}

static void aisa_setup_atmark_techno_serial_con2(u16 revision __maybe_unused)
{
	struct pad_desc pads[] = {
		MX25_PAD_RTCK__GPIO_3_14(0), /* CON2_32 */
		MX25_PAD_UART1_CTS__UART1_CTS(0), /* CON2_38 */
		MX25_PAD_UART1_RTS__UART1_RTS(PAD_CTL_PKE | PAD_CTL_PUE |
					      PAD_CTL_PUS_100K_UP), /* CON2_39 */
		MX25_PAD_UART1_TXD__UART1_TXD(0), /* CON2_40 */
		MX25_PAD_UART1_RXD__UART1_RXD(PAD_CTL_PKE |
					      PAD_CTL_PUS_100K_UP), /* CON2_41 */
		MX25_PAD_KPP_COL3__GPIO_3_4(PAD_CTL_PKE |
					PAD_CTL_PUS_100K_UP), /* CON2_42 */
		MX25_PAD_KPP_COL2__GPIO_3_3(0), /* CON2_43 */
	};

	mxc_iomux_v3_setup_multiple_pads(pads, ARRAY_SIZE(pads));

	aisa_gpio_request(GPIO(3, 4),  "XR3160_MODE_CON2");
	aisa_gpio_request(GPIO(3, 14),  "XR3160_DUPLEX_CON2");
	aisa_gpio_request(GPIO(3, 3),  "ADUM1402_VE1_CON2");
	aisa_serial_setup(GPIO(3, 4), GPIO(3, 14), GPIO(3, 3));

	aisa_uart1_rs485_register(gpio_get_value(GPIO(3, 4)),
				  xr3160_duplex_con2);
}

static void aisa_rs232c_setup(int forceoff, int ri, int dcd, int dsr, int dtr)
{
	gpio_direction_output(forceoff, 1);
	gpio_direction_input(ri);
	gpio_direction_input(dcd);
	gpio_direction_input(dsr);
	gpio_direction_output(dtr, 0);

	gpio_export(forceoff, false);
	gpio_export(ri, false);
	gpio_export(dcd, false);
	gpio_export(dsr, false);
	gpio_export(dtr, false);
}

static void aisa_setup_atmark_techno_rs232c_con1(u16 revision __maybe_unused)
{
	struct pad_desc pads[] = {
		MX25_PAD_LD11__UART4_CTS(0), /* CON1_38 */
		MX25_PAD_LD10__UART4_RTS(PAD_CTL_PKE | PAD_CTL_PUE |
					 PAD_CTL_PUS_100K_DOWN), /* CON1_39 */
		MX25_PAD_LD9__UART4_TXD(0), /* CON1_40 */
		MX25_PAD_LD8__UART4_RXD(PAD_CTL_PKE | PAD_CTL_PUE |
					PAD_CTL_PUS_100K_UP), /* CON1_41 */
		MX25_PAD_LD7__GPIO_1_21(PAD_CTL_PKE | PAD_CTL_PUE |
					PAD_CTL_PUS_100K_UP), /* CON1_42 FORCEOFF */
		MX25_PAD_LD3__GPIO_2_18(PAD_CTL_PKE | PAD_CTL_PUE |
					PAD_CTL_PUS_100K_UP),/* CON1_46 RI */
		MX25_PAD_LD2__GPIO_2_17(PAD_CTL_PKE | PAD_CTL_PUE |
					PAD_CTL_PUS_100K_UP),/* CON1_47 DCD */
		MX25_PAD_LD1__GPIO_2_16(PAD_CTL_PKE | PAD_CTL_PUE |
					PAD_CTL_PUS_100K_UP),/* CON1_48 DSR */
		MX25_PAD_LD0__GPIO_2_15(0),/* CON1_49 DTR*/
	};

	mxc_iomux_v3_setup_multiple_pads(pads, ARRAY_SIZE(pads));
	aisa_uart4_register(1);

	aisa_gpio_request(GPIO(1, 21), "FORCEOFF_CON1");
	aisa_gpio_request(GPIO(2, 18), "RI_CON1");
	aisa_gpio_request(GPIO(2, 17), "DCD_CON1");
	aisa_gpio_request(GPIO(2, 16), "DSR_CON1");
	aisa_gpio_request(GPIO(2, 15), "DTR_CON1");
	aisa_rs232c_setup(GPIO(1, 21), GPIO(2, 18), GPIO(2, 17),
			  GPIO(2, 16), GPIO(2, 15));
}

static void aisa_setup_atmark_techno_rs232c_con2(u16 revision __maybe_unused)
{
	struct pad_desc pads[] = {
		MX25_PAD_UART1_CTS__UART1_CTS(0), /* CON2_38 */
		MX25_PAD_UART1_RTS__UART1_RTS(PAD_CTL_PKE | PAD_CTL_PUE |
						 PAD_CTL_PUS_100K_UP), /* CON2_39 */
		MX25_PAD_UART1_TXD__UART1_TXD(0), /* CON2_40 */
		MX25_PAD_UART1_RXD__UART1_RXD(PAD_CTL_PKE |
					      PAD_CTL_PUS_100K_UP), /* CON2_41 */
		MX25_PAD_KPP_COL3__GPIO_3_4(PAD_CTL_PKE |
					    PAD_CTL_PUS_100K_UP), /* CON2_42 FORCEOFF */
		MX25_PAD_KPP_ROW3__GPIO_3_0(PAD_CTL_PKE | PAD_CTL_PUE |
					    PAD_CTL_PUS_100K_UP), /* CON2_46 RI */
		MX25_PAD_KPP_ROW2__GPIO_2_31(PAD_CTL_PKE |
					    PAD_CTL_PUE | PAD_CTL_PUS_100K_UP), /* CON2_47 DCD */
		MX25_PAD_KPP_ROW1__GPIO_2_30(PAD_CTL_PKE |
					     PAD_CTL_PUS_100K_UP), /* CON2_48 DSR */
		MX25_PAD_KPP_ROW0__GPIO_2_29(0), /* CON2_49 DTR */
	};

	mxc_iomux_v3_setup_multiple_pads(pads, ARRAY_SIZE(pads));
	aisa_uart1_register(1);

	aisa_gpio_request(GPIO(3, 4), "FORCEOFF_CON2");
	aisa_gpio_request(GPIO(3, 0), "RI_CON2");
	aisa_gpio_request(GPIO(2, 31), "DCD_CON2");
	aisa_gpio_request(GPIO(2, 30), "DSR_CON2");
	aisa_gpio_request(GPIO(2, 29), "DTR_CON2");
	aisa_rs232c_setup(GPIO(3, 4), GPIO(3, 0), GPIO(2, 31),
			  GPIO(2, 30), GPIO(2, 29));
}

/*
 * Setup
 */
static void aisa_setup_con1(u16 vendor, u16 product)
{
	int revision;

	revision = aisa_eeprom_read_word_swapped(AISA_ADDR_CON1, AISA_REVISION);
	switch (vendor) {
	case AISA_VENDOR_ID_ATMARK_TECHNO:
		switch (product) {
		case AISA_PRODUCT_ID_ATMARK_TECHNO_WI_SUN:
			aisa_setup_atmark_techno_wi_sun_con1(revision);
			break;
		case AISA_PRODUCT_ID_ATMARK_TECHNO_EN_OCEAN:
			aisa_setup_atmark_techno_en_ocean_con1(revision);
			break;
		case AISA_PRODUCT_ID_ATMARK_TECHNO_SERIAL:
			aisa_setup_atmark_techno_serial_con1(revision);
			break;
		case AISA_PRODUCT_ID_ATMARK_TECHNO_DIDOAD:
			break;
		case AISA_PRODUCT_ID_ATMARK_TECHNO_BLE:
			aisa_setup_atmark_techno_ble_con1(revision);
			break;
		case AISA_PRODUCT_ID_ATMARK_TECHNO_CAN:
			break;
		case AISA_PRODUCT_ID_ATMARK_TECHNO_ZIGBEE:
			break;
		case AISA_PRODUCT_ID_ATMARK_TECHNO_RS232C:
			aisa_setup_atmark_techno_rs232c_con1(revision);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	pr_info("%s %s board detected at CON1(0x%04x).\n",
		aisa_get_vendor_name(vendor),
		aisa_get_product_name(vendor, product), revision);
}

static void aisa_setup_con2(u16 vendor, u16 product)
{
	int revision;

	revision = aisa_eeprom_read_word_swapped(AISA_ADDR_CON2, AISA_REVISION);
	switch (vendor) {
	case AISA_VENDOR_ID_ATMARK_TECHNO:
		switch (product) {
		case AISA_PRODUCT_ID_ATMARK_TECHNO_WI_SUN:
			aisa_setup_atmark_techno_wi_sun_con2(revision);
			break;
		case AISA_PRODUCT_ID_ATMARK_TECHNO_EN_OCEAN:
			aisa_setup_atmark_techno_en_ocean_con2(revision);
			break;
		case AISA_PRODUCT_ID_ATMARK_TECHNO_SERIAL:
			aisa_setup_atmark_techno_serial_con2(revision);
			break;
		case AISA_PRODUCT_ID_ATMARK_TECHNO_DIDOAD:
			break;
		case AISA_PRODUCT_ID_ATMARK_TECHNO_BLE:
			aisa_setup_atmark_techno_ble_con2(revision);
			break;
		case AISA_PRODUCT_ID_ATMARK_TECHNO_CAN:
			break;
		case AISA_PRODUCT_ID_ATMARK_TECHNO_ZIGBEE:
			break;
		case AISA_PRODUCT_ID_ATMARK_TECHNO_RS232C:
			aisa_setup_atmark_techno_rs232c_con2(revision);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	pr_info("%s %s board detected at CON2(0x%04x).\n",
		aisa_get_vendor_name(vendor),
		aisa_get_product_name(vendor, product), revision);
}

static int __init aisa_setup(void)
{
	int vendor, product;

	vendor = aisa_eeprom_read_word_swapped(AISA_ADDR_CON1, AISA_VENDOR_ID);
	product = aisa_eeprom_read_word_swapped(AISA_ADDR_CON1, AISA_PRODUCT_ID);
	if ((vendor > 0) && (product > 0)) {
		pr_debug("Add-on expansion board detected at CON1(0x%04x:0x%04x).\n",
			 vendor, product);
		aisa_setup_con1(vendor, product);
	}

	vendor = aisa_eeprom_read_word_swapped(AISA_ADDR_CON2, AISA_VENDOR_ID);
	product = aisa_eeprom_read_word_swapped(AISA_ADDR_CON2, AISA_PRODUCT_ID);
	if ((vendor > 0) && (product > 0)) {
		pr_debug("Add-on expansion board detected at CON2(0x%04x:0x%04x).\n",
			 vendor, product);
		aisa_setup_con2(vendor, product);
	}

	return 0;
}
subsys_initcall_sync(aisa_setup);
