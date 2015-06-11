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
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/i2c/pca953x.h>
#include <linux/spi/spi.h>
#include <linux/pwm.h>
#include <linux/fsl_devices.h>
#include <linux/usb/fsl_xcvr.h>
#include <linux/regulator/fixed.h>
#include <linux/regulator/regulator-platform.h>
#include <linux/rtc/rtc-s35390a.h>
#include <linux/ti-adc081c.h>
#if defined(CONFIG_MTD) || defined(CONFIG_MTD_MODULE)
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/cfi.h>

#include <asm/mach/flash.h>
#include <asm/arch/mtd.h>
#endif

#include <asm/arch/common.h>
#include <asm/arch/iomux-mx25.h>
#include <asm/arch/mx25_fec.h>
#include <asm/arch/mmc.h>
#include <asm/arch/mxc_flexcan.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#include "board.h"
#include "devices.h"

extern void mx25_generic_gpio_init(void);
extern void armadillo_iotg_std_gpio_init(void);
extern void armadillo_iotg_std_addon_gpio_init(struct mxc_ext_gpio *ext_gpios, int nr_ext_gpios);

static struct pca953x_platform_data armadillo_iotg_std_pca953x_plat_data = { 
	.gpio_base      = MXC_MAX_GPIO_LINES,
};

static struct pad_desc armadillo_iotg_std_s35390a_pads[] = {
	MX25_PAD_CSPI1_MISO__GPIO_1_15(PAD_CTL_PKE | PAD_CTL_PUS_100K_UP),
};

static void armadillo_iotg_std_s35390a_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(armadillo_iotg_std_s35390a_pads,
					 ARRAY_SIZE(armadillo_iotg_std_s35390a_pads));
	gpio_request(GPIO(1, 15), "RTC_ALM_INT");
	gpio_direction_input(GPIO(1, 15));
}

static struct s35390a_platform_data armadillo_iotg_std_s35390a_plat_data = { 
	.alarm_irq = GPIO_TO_IRQ(GPIO(1, 15)),
	.alarm_irq_init = armadillo_iotg_std_s35390a_init,
	.alarm_is_wakeup_src = 1,
};

static struct adc081c_platform_data armadillo_iotg_std_adc081c_plat_data = { 
	.vref_mv        = 3300,
};

static struct i2c_board_info armadillo_iotg_std_gpio_i2c1_board_info[] __initdata = { 
	{
		.type = "s35390a",
		.addr = 0x30,
		.platform_data = &armadillo_iotg_std_s35390a_plat_data,
	},
	{
		.type = "lm75b",
		.addr = 0x48,
	},
	{
		.type = "adc081c",
		.addr = 0x54,
		.platform_data = &armadillo_iotg_std_adc081c_plat_data,
	},
	{
		.type = "pca9538",
		.addr = 0x70,
		.platform_data = &armadillo_iotg_std_pca953x_plat_data,
	},
};

static struct i2c_gpio_platform_data armadillo_iotg_std_gpio_i2c1_data = {
	.sda_pin = GPIO(1, 17),
	.scl_pin = GPIO(1, 18),
	.scl_is_output_only = 1,
	.udelay = 5,
	.timeout = HZ,
};

static struct platform_device armadillo_iotg_std_gpio_i2c1_device = {
	.name = "i2c-gpio",
	.id = AIOTG_STD_GPIO_I2C1_ID,
};

static struct i2c_gpio_platform_data armadillo_iotg_std_gpio_i2c2_data = {
	.sda_pin = GPIO(3, 2),
	.scl_pin = GPIO(3, 1),
	.scl_is_output_only = 1,
	.udelay = 5,
	.timeout = HZ,
};

static struct platform_device armadillo_iotg_std_gpio_i2c2_device = {
	.name = "i2c-gpio",
	.id = AIOTG_STD_GPIO_I2C2_ID,
};

static void __init armadillo_iotg_std_gpio_i2c_init(void)
{
	mxc_register_device(&armadillo_iotg_std_gpio_i2c1_device,
			    &armadillo_iotg_std_gpio_i2c1_data);
	i2c_register_board_info(armadillo_iotg_std_gpio_i2c1_device.id,
				armadillo_iotg_std_gpio_i2c1_board_info,
				ARRAY_SIZE(armadillo_iotg_std_gpio_i2c1_board_info));

	mxc_register_device(&armadillo_iotg_std_gpio_i2c2_device,
			    &armadillo_iotg_std_gpio_i2c2_data);
}

static struct gpio_keys_button armadillo_iotg_std_gpio_key_buttons[] = {
	{KEY_1, GPIO(3, 30), 1, "SW1", EV_KEY, 0},
};

static struct gpio_keys_platform_data armadillo_iotg_std_gpio_key_data = {
	.buttons = armadillo_iotg_std_gpio_key_buttons,
	.nbuttons = ARRAY_SIZE(armadillo_iotg_std_gpio_key_buttons),
	.wakeup_default_disabled = 1,
};

static void __init armadillo_iotg_std_gpio_key_init(void)
{
	mxc_register_device(&mx25_gpio_key_device, &armadillo_iotg_std_gpio_key_data);
}

static struct gpio_keys_button armadillo_iotg_std_gpio_key_polled_buttons[] = {
	{KEY_2, MXC_MAX_GPIO_LINES + 0, 1, "SW2", EV_KEY, 0},
	{KEY_3, MXC_MAX_GPIO_LINES + 1, 1, "SW3", EV_KEY, 0},
};

static struct gpio_keys_platform_data armadillo_iotg_std_gpio_key_polled_data = {
	.buttons = armadillo_iotg_std_gpio_key_polled_buttons,
	.nbuttons = ARRAY_SIZE(armadillo_iotg_std_gpio_key_polled_buttons),
	.poll_interval = 20,	/* msecs */
};

static struct platform_device armadillo_iotg_std_gpio_key_polled_device = {
	.name = "gpio-keys-polled",
	.id = 1,
};

static void __init armadillo_iotg_std_gpio_key_polled_init(void)
{
	mxc_register_device(&armadillo_iotg_std_gpio_key_polled_device,
			    &armadillo_iotg_std_gpio_key_polled_data);
}

extern void gpio_sierra_wireless_active(void);

static void __init armadillo_iotg_std_sierra_wireless_init(void)
{
	gpio_sierra_wireless_active();
}

extern void gpio_temp_sensor_active(void);

static void __init armadillo_iotg_std_temp_sensor_init(void)
{
	gpio_temp_sensor_active();
}

extern void gpio_ad_converter_active(void);

static void __init armadillo_iotg_std_ad_converter_init(void)
{
	gpio_ad_converter_active();
}

static void __init armadillo_iotg_std_regulator_init(void)
{
}

void armadillo400_set_vbus_power(struct fsl_usb2_platform_data *pdata,
				 int on)
{
	static int usb_use_count = 0;

	if (on) {
		if (!usb_use_count) {
			if (gpio_get_value(USB_PWREN_GPIO) == USB_PWREN_ON) {
				gpio_set_value(USB_PWREN_GPIO, USB_PWREN_OFF);
				mdelay(500);
			}

			gpio_set_value(USB_PWREN_GPIO, USB_PWREN_ON);
		}
		usb_use_count++;
	} else {
		if (usb_use_count > 0 && !(--usb_use_count))
				gpio_set_value(USB_PWREN_GPIO, USB_PWREN_OFF);
	}
}
EXPORT_SYMBOL(armadillo400_set_vbus_power);

#define MTD_PART(_name, _size, _offset, _mask_flags) \
	{                                            \
		.name		= _name,             \
		.size		= _size,             \
		.offset		= _offset,           \
		.mask_flags	= _mask_flags,       \
	}

static struct mtd_partition armadillo_iotg_std_mtd_nor_partitions_8MB[] = {
	MTD_PART("nor.bootloader",   4* 32*1024, 0,
		 MTD_WRITEABLE /* force read-only */ ),
	MTD_PART("nor.kernel",      32*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.userland",    23*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.config",       8*128*1024, MTDPART_OFS_APPEND, 0),
};

static struct mtd_partition armadillo_iotg_std_mtd_nor_partitions_16MB[] = {
	MTD_PART("nor.bootloader",   4* 32*1024, 0,
		 MTD_WRITEABLE /* force read-only */ ),
	MTD_PART("nor.kernel",      32*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.userland",    87*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.config",       8*128*1024, MTDPART_OFS_APPEND, 0),
};

static struct mtd_partition armadillo_iotg_std_mtd_nor_partitions_32MB[] = {
	MTD_PART("nor.bootloader",   4* 32*1024, 0,
		 MTD_WRITEABLE /* force read-only */ ),
	MTD_PART("nor.kernel",      32*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.userland",   215*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.config",       8*128*1024, MTDPART_OFS_APPEND, 0),
};

static struct mtd_partition armadillo_iotg_std_mtd_nor_partitions_64MB[] = {
	MTD_PART("nor.bootloader",   4* 32*1024, 0,
		 MTD_WRITEABLE /* force read-only */ ),
	MTD_PART("nor.kernel",      32*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.userland",   471*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.config",       8*128*1024, MTDPART_OFS_APPEND, 0),
};

static int
armadillo_iotg_std_mtd_nor_update_partitions(struct map_info *map,
			  struct flash_platform_data *plat)
{
	struct cfi_private *cfi = map->fldrv_priv;

	switch (cfi->cfiq->DevSize) {
	case 23: /* 8MB */
		plat->parts	= armadillo_iotg_std_mtd_nor_partitions_8MB;
		plat->nr_parts	= ARRAY_SIZE(armadillo_iotg_std_mtd_nor_partitions_8MB);
		break;
	case 24: /* 16MB */
		plat->parts	= armadillo_iotg_std_mtd_nor_partitions_16MB;
		plat->nr_parts	= ARRAY_SIZE(armadillo_iotg_std_mtd_nor_partitions_16MB);
		break;
	case 25: /* 32MB */
		break;
	case 26: /* 64MB */
		plat->parts	= armadillo_iotg_std_mtd_nor_partitions_64MB;
		plat->nr_parts	= ARRAY_SIZE(armadillo_iotg_std_mtd_nor_partitions_64MB);
		break;
	default:
		printk("Not support flash-size.\n");
		return -1;
	}

	return 0;
}

static struct armadillo_flash_private_data armadillo_iotg_std_mtd_nor_data = {
	.plat = {
		.map_name	= "cfi_probe",
		.width		= 2,
		.parts		= armadillo_iotg_std_mtd_nor_partitions_32MB,
		.nr_parts	= ARRAY_SIZE(armadillo_iotg_std_mtd_nor_partitions_32MB),
	},
	.update_partitions = armadillo_iotg_std_mtd_nor_update_partitions,
	.map_name = "armadillo-nor",
};

static void __init armadillo_iotg_std_mtd_nor_init(void)
{
	mxc_register_device(&mx25_mtd_nor_device, &armadillo_iotg_std_mtd_nor_data);
}

static struct gpio_led armadillo_iotg_std_led_pins[] = {
	{"led1",  "default-on", GPIO(3, 29), 0},
	{"led2",  "default-on", GPIO(3, 28), 0},
	{"led3",  "default-off", MXC_MAX_GPIO_LINES + 2, 0},
	{"led4",  "default-off", MXC_MAX_GPIO_LINES + 3, 0},
	{"yellow", NULL,         GPIO(4, 30), 0},
};

struct gpio_led_platform_data armadillo_iotg_std_led_data = {
	.leds = armadillo_iotg_std_led_pins,
	.num_leds = ARRAY_SIZE(armadillo_iotg_std_led_pins),
};

void __init armadillo_iotg_std_led_init(void)
{
	mxc_register_device(&mx25_gpio_led_device, &armadillo_iotg_std_led_data);
}

static struct mxc_ext_gpio gpio_list_addon[] = {
#if defined(CONFIG_AIOTG_STD_CON1_3_CON2_24_GPIO1_2)
	{"CON1_3",   GPIO(1, 2), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_4_CON2_25_GPIO1_3)
	{"CON1_4",   GPIO(1, 3), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_5_CON2_33_GPIO1_26)
	{"CON1_5",   GPIO(1, 26), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_6_CON2_32_GPIO3_14)
	{"CON1_6",   GPIO(3, 14), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_7_CON2_41_GPIO4_22)
	{"CON1_7",   GPIO(4, 22), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_8_CON2_40_GPIO4_23)
	{"CON1_8",   GPIO(4, 23), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_9_CON2_7_39_GPIO4_24)
	{"CON1_9",   GPIO(4, 24), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_10_CON2_8_38_GPIO4_25)
	{"CON1_10",   GPIO(4, 25), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_11_CON2_50_GPIO4_21)
	{"CON1_11",   GPIO(4, 21), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_12_CON2_16_37_GPIO1_27)
	{"CON1_12",   GPIO(1, 27), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_13_CON2_17_36_GPIO1_28)
	{"CON1_13",   GPIO(1, 28), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_14_CON2_12_18_35_GPIO1_29)
	{"CON1_14",   GPIO(1, 29), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_15_CON2_13_19_34_GPIO1_30)
	{"CON1_15",   GPIO(1, 30), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_16_CON2_49_GPIO2_29)
	{"CON1_16",   GPIO(2, 29), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_17_CON2_48_GPIO2_30)
	{"CON1_17",   GPIO(2, 30), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_18_CON2_47_GPIO2_31)
	{"CON1_18",   GPIO(2, 31), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_19_CON2_46_GPIO3_0)
	{"CON1_19",   GPIO(3, 0), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_20_CON2_20_45_GPIO3_1)
	{"CON1_20",   GPIO(3, 1), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_21_CON2_21_44_GPIO3_2)
	{"CON1_21",   GPIO(3, 2), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_22_CON2_22_43_GPIO3_3)
	{"CON1_22",   GPIO(3, 3), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_23_CON2_23_42_GPIO3_4)
	{"CON1_23",   GPIO(3, 4), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_24_GPIO1_0)
	{"CON1_24",   GPIO(1, 0), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_25_GPIO1_1)
	{"CON1_25",   GPIO(1, 1), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_32_GPIO1_5)
	{"CON1_32",   GPIO(1, 5), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_33_GPIO1_4)
	{"CON1_33",   GPIO(1, 4), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_42_GPIO1_21)
	{"CON1_42",   GPIO(1, 21), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_43_GPIO1_20)
	{"CON1_43",   GPIO(1, 20), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_44_GPIO1_19)
	{"CON1_44",   GPIO(1, 19), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_45_GPIO2_19)
	{"CON1_45",   GPIO(2, 19), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_46_GPIO2_18)
	{"CON1_46",   GPIO(2, 18), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_47_GPIO2_17)
	{"CON1_47",   GPIO(2, 17), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_48_GPIO2_16)
	{"CON1_48",   GPIO(2, 16), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_49_GPIO2_15)
	{"CON1_49",   GPIO(2, 15), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_50_GPIO1_25)
	{"CON1_50",   GPIO(1, 25), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_51_GPIO1_23)
	{"CON1_51",   GPIO(1, 23), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_52_GPIO1_22)
	{"CON1_52",   GPIO(1, 22), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_AIOTG_STD_CON1_53_GPIO1_24)
	{"CON1_53",   GPIO(1, 24), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
};

#if defined(CONFIG_I2C_MXC_SELECT1)
static struct mxc_i2c_platform_data armadillo_iotg_std_i2c1_data = {
	.i2c_clk = 40000,
};

static struct i2c_board_info armadillo_iotg_std_i2c1_board_info[] __initdata = {
	{
		.type = "mc34704",
		.addr = 0x54,
	},
};
#endif /* defined(CONFIG_I2C_MXC_SELECT1) */

#if defined(CONFIG_I2C_MXC_SELECT2)
static struct i2c_board_info armadillo_iotg_std_i2c2_board_info[] __initdata = {
};

static struct mxc_i2c_platform_data armadillo_iotg_std_i2c2_data = {
	.i2c_clk = 40000,
};
#endif /* defined(CONFIG_I2C_MXC_SELECT2) */

#if defined(CONFIG_I2C_MXC_SELECT3)
static struct i2c_board_info armadillo_iotg_std_i2c3_board_info[] __initdata = {
};

static struct mxc_i2c_platform_data armadillo_iotg_std_i2c3_data = {
	.i2c_clk = 40000,
};
#endif /* defined(CONFIG_I2C_MXC_SELECT3) */

static void __init armadillo_iotg_std_i2c_init(void)
{
#if defined(CONFIG_I2C_MXC_SELECT1)
	mxc_register_device(&mx25_i2c1_device,
			    &armadillo_iotg_std_i2c1_data);
	i2c_register_board_info(0, armadillo_iotg_std_i2c1_board_info,
				ARRAY_SIZE(armadillo_iotg_std_i2c1_board_info));
#endif /* defined(CONFIG_I2C_MXC_SELECT1) */

#if defined(CONFIG_I2C_MXC_SELECT2)
	mxc_register_device(&mx25_i2c2_device,
			    &armadillo_iotg_std_i2c2_data);
	i2c_register_board_info(1, armadillo_iotg_std_i2c2_board_info,
				ARRAY_SIZE(armadillo_iotg_std_i2c2_board_info));
#endif /* defined(CONFIG_I2C_MXC_SELECT2) */

#if defined(CONFIG_I2C_MXC_SELECT3)
	mxc_register_device(&mx25_i2c3_device,
			    &armadillo_iotg_std_i2c3_data);
	i2c_register_board_info(2, armadillo_iotg_std_i2c3_board_info,
				ARRAY_SIZE(armadillo_iotg_std_i2c3_board_info));
#endif /* defined(CONFIG_I2C_MXC_SELECT3) */
}

#if defined(CONFIG_SPI_MXC_SELECT2)
extern void gpio_spi2_cs_active(int cspi_mode, int chipselect);
extern void gpio_spi2_cs_inactive(int cspi_mode, int chipselect);

static struct mxc_spi_master armadillo_iotg_std_spi2_data = {
	.maxchipselect = 4,
	.spi_version = 7,
	.chipselect_active = gpio_spi2_cs_active,
	.chipselect_inactive = gpio_spi2_cs_inactive,
};

static struct spi_board_info armadillo_iotg_std_spi2_board_info[] __initdata = {
};
#endif

#if defined(CONFIG_SPI_MXC_SELECT3)
extern void gpio_spi3_cs_active(int cspi_mode, int chipselect);
extern void gpio_spi3_cs_inactive(int cspi_mode, int chipselect);

static struct mxc_spi_master armadillo_iotg_std_spi3_data = {
	.maxchipselect = 4,
	.spi_version = 7,
	.chipselect_active = gpio_spi3_cs_active,
	.chipselect_inactive = gpio_spi3_cs_inactive,
};

static struct spi_board_info armadillo_iotg_std_spi3_board_info[] __initdata = {
};
#endif

static void __init armadillo_iotg_std_spi_init(void)
{
#if defined(CONFIG_SPI_MXC_SELECT2)
	mxc_register_device(&mx25_spi2_device, &armadillo_iotg_std_spi2_data);
	spi_register_board_info(armadillo_iotg_std_spi2_board_info,
				ARRAY_SIZE(armadillo_iotg_std_spi2_board_info));
#endif

#if defined(CONFIG_SPI_MXC_SELECT3)
	mxc_register_device(&mx25_spi3_device, &armadillo_iotg_std_spi3_data);
	spi_register_board_info(armadillo_iotg_std_spi3_board_info,
				ARRAY_SIZE(armadillo_iotg_std_spi3_board_info));
#endif
}

extern void gpio_fec_suspend(void);
extern void gpio_fec_resume(void);

static int armadillo_iotg_std_fec_suspend(struct platform_device *pdev, pm_message_t state)
{
	gpio_fec_suspend();

	return 0;
}

static int armadillo_iotg_std_fec_resume(struct platform_device *pdev)
{
	gpio_fec_resume();

	return 0;
}

static struct fec_platform_data armadillo_iotg_std_fec_data = {
	.suspend = armadillo_iotg_std_fec_suspend,
	.resume  = armadillo_iotg_std_fec_resume,
};

static int __init armadillo_iotg_std_init_fec(void)
{
	mxc_register_device(&mx25_fec_device, &armadillo_iotg_std_fec_data);
	return 0;
}
late_initcall(armadillo_iotg_std_init_fec);

extern unsigned int sdhc_get_card_det_status(struct device *dev);
extern int sdhc_get_write_protect(struct device *dev);

#if defined(CONFIG_MMC_MXC_SELECT1) | defined(CONFIG_MMC_MXC_SELECT2)
extern void gpio_sdhc_active(int module);
extern void gpio_sdhc_inactive(int module);
#endif

#if defined(CONFIG_MMC_MXC_SELECT1)

static struct mxc_mmc_platform_data armadillo_iotg_std_sdhc1_data = {
	.ocr_mask = MMC_VDD_29_30 | MMC_VDD_32_33,
	.caps = MMC_CAP_4_BIT_DATA,
	.min_clk = 400000,
	.max_clk = 52000000,
	.card_inserted_state = 0,
	.status = sdhc_get_card_det_status,
	.wp_status = sdhc_get_write_protect,
	.clock_mmc = "esdhc_clk",
};
#endif /* defined(CONFIG_MMC_MXC_SELECT1) */

#if defined(CONFIG_MMC_MXC_SELECT2)
static struct mxc_mmc_platform_data armadillo_iotg_std_sdhc2_data = {
	.ocr_mask = MMC_VDD_29_30 | MMC_VDD_32_33,
	.caps = MMC_CAP_4_BIT_DATA,
	.min_clk = 400000,
	.max_clk = 52000000,
	.card_inserted_state = 0,
	.status = sdhc_get_card_det_status,
	.wp_status = sdhc_get_write_protect,
	.clock_mmc = "esdhc_clk",
};
#endif /* defined(CONFIG_MMC_MXC_SELECT2) */

static void __init armadillo_iotg_std_sdhc_init(void)
{
#if defined(CONFIG_MMC_MXC_SELECT1)
	mx25_sdhc1_device.resource[2].start = GPIO_TO_IRQ(SDHC1_CD_GPIO);
	mx25_sdhc1_device.resource[2].end = GPIO_TO_IRQ(SDHC1_CD_GPIO);
	mxc_register_device(&mx25_sdhc1_device,
			    &armadillo_iotg_std_sdhc1_data);
	gpio_sdhc_active(0);
	gpio_sdhc_inactive(0);
#endif
#if defined(CONFIG_MMC_MXC_SELECT2)
	mx25_sdhc2_device.resource[2].start = GPIO_TO_IRQ(AIOTG_STD_SDHC2_CD_GPIO);
	mx25_sdhc2_device.resource[2].end = GPIO_TO_IRQ(AIOTG_STD_SDHC2_WP_GPIO);
	mxc_register_device(&mx25_sdhc2_device,
			    &armadillo_iotg_std_sdhc2_data);
	gpio_sdhc_active(1);
	gpio_sdhc_inactive(1);
#endif
}

static struct mxc_w1_config armadillo_iotg_std_mxc_w1_data __maybe_unused = {
	.search_rom_accelerator = 0,
};

static void __init armadillo_iotg_std_mxc_w1_init(void)
{
#if defined(CONFIG_AIOTG_STD_CON1_6_CON2_32_W1)
	mxc_register_device(&mx25_mxc_w1_device,
			    &armadillo_iotg_std_mxc_w1_data);
#endif
}

extern void gpio_flexcan_active(int id);
extern void gpio_flexcan_inactive(int id);

#if defined(CONFIG_FLEXCAN_SELECT1)
struct flexcan_platform_data armadillo_iotg_std_flexcan1_data = {
        .core_reg = NULL,
        .io_reg = NULL,
        .active = gpio_flexcan_active,
        .inactive = gpio_flexcan_inactive,
};
#endif

#if defined(CONFIG_FLEXCAN_SELECT2)
struct flexcan_platform_data armadillo_iotg_std_flexcan2_data = {
        .core_reg = NULL,
        .io_reg = NULL,
        .active = gpio_flexcan_active,
        .inactive = gpio_flexcan_inactive,
};
#endif

static void __init armadillo_iotg_std_flexcan_init(void)
{
#if defined(CONFIG_FLEXCAN_SELECT1)
	mxc_register_device(&mx25_flexcan1_device,
			    &armadillo_iotg_std_flexcan1_data);
#endif

#if defined(CONFIG_FLEXCAN_SELECT2)
	mxc_register_device(&mx25_flexcan2_device,
			    &armadillo_iotg_std_flexcan2_data);
#endif
}

static struct platform_pwm_data armadillo_iotg_std_pwm1_data __maybe_unused = {
	.name = "CON1_5",
	.invert = 0,
	.export = 1,
};

static struct platform_pwm_data armadillo_iotg_std_pwm2_data __maybe_unused = {
	.name = "CON1_24",
	.invert = 0,
	.export = 1,
};

static struct platform_pwm_data armadillo_iotg_std_pwm3_data __maybe_unused = {
	.name = "CON1_25",
	.invert = 0,
	.export = 1,
};

static struct platform_pwm_data armadillo_iotg_std_pwm4_data __maybe_unused = {
	.name = "CON1_3",
	.invert = 0,
	.export = 1,
};

static void armadillo_iotg_std_pwm_init(void)
{
#if defined(CONFIG_MXC_PWM_SELECT1)
	mxc_register_device(&mx25_pwm1_device, &armadillo_iotg_std_pwm1_data);
#endif
#if defined(CONFIG_MXC_PWM_SELECT2)
	mxc_register_device(&mx25_pwm2_device, &armadillo_iotg_std_pwm2_data);
#endif
#if defined(CONFIG_MXC_PWM_SELECT3)
	mxc_register_device(&mx25_pwm3_device, &armadillo_iotg_std_pwm3_data);
#endif
#if defined(CONFIG_MXC_PWM_SELECT4)
	mxc_register_device(&mx25_pwm4_device, &armadillo_iotg_std_pwm4_data);
#endif
}

/*!
 * Board specific fixup function. It is called by \b setup_arch() in
 * setup.c file very early on during kernel starts. It allows the user to
 * statically fill in the proper values for the passed-in parameters. None of
 * the parameters is used currently.
 *
 * @param  desc         pointer to \b struct \b machine_desc
 * @param  tags         pointer to \b struct \b tag
 * @param  cmdline      pointer to the command line
 * @param  mi           pointer to \b struct \b meminfo
 */
static void __init fixup_armadillo410_board(struct machine_desc *desc, struct tag *tags,
				   char **cmdline, struct meminfo *mi)
{
	mxc_cpu_init();
}

/*!
 * Board specific initialization.
 */
static void __init armadillo_iotg_std_init(void)
{
	mxc_cpu_common_init();
	mxc_clocks_init();
	armadillo_iotg_std_gpio_init();
	mxc_gpio_init();
#if defined(CONFIG_GENERIC_GPIO)
	mx25_generic_gpio_init();
#endif
	early_console_setup(saved_command_line);

	armadillo_iotg_std_regulator_init();
	armadillo_iotg_std_i2c_init();
	armadillo_iotg_std_mtd_nor_init();
	armadillo_iotg_std_led_init();
	armadillo_iotg_std_sdhc_init();
	armadillo_iotg_std_gpio_i2c_init();
	armadillo_iotg_std_gpio_key_init();
	armadillo_iotg_std_gpio_key_polled_init();
	armadillo_iotg_std_sierra_wireless_init();
	armadillo_iotg_std_temp_sensor_init();
	armadillo_iotg_std_ad_converter_init();

	armadillo_iotg_std_addon_gpio_init(gpio_list_addon,
					   ARRAY_SIZE(gpio_list_addon));
	armadillo_iotg_std_pwm_init();
	armadillo_iotg_std_mxc_w1_init();
	armadillo_iotg_std_spi_init();
	armadillo_iotg_std_flexcan_init();
}

#if defined(CONFIG_MACH_ARMADILLO410)
/*
 * The following uses standard kernel macros define in arch.h in order to
 * initialize __mach_desc_ARMADILLO410 data structure.
 */
MACHINE_START(ARMADILLO410, "Armadillo-410")
	/* Maintainer: Atmark Techno, Inc. */
	.phys_io = AIPS1_BASE_ADDR,
	.io_pg_offst = ((AIPS1_BASE_ADDR_VIRT) >> 18) & 0xfffc,
	.boot_params = PHYS_OFFSET + 0x100,
	.fixup = fixup_armadillo410_board,
	.map_io = mxc_map_io,
	.init_irq = mxc_init_irq,
	/* Initialization for Armadillo-410 is same as initialization for armadillo-440. */
	.init_machine = armadillo_iotg_std_init,
	.timer = &mxc_timer,
MACHINE_END
#endif

