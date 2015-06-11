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
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <linux/armadillo2x0_led.h>
#include <linux/armadillo2x0_gpio.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/spi/spi.h>
#include <linux/pwm.h>
#include <linux/pwm_backlight.h>
#include <linux/fsl_devices.h>
#include <linux/usb/fsl_xcvr.h>
#include <linux/w1-gpio.h>
#include <linux/regulator/fixed.h>
#include <linux/regulator/regulator-platform.h>
#include <linux/rtc/rtc-s35390a.h>
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
#include <asm/arch/imx_adc.h>
#include <asm/arch/imx_adc_ts.h>
#include <asm/arch/mxc_flexcan.h>

#include <asm/mach/keypad.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#include "board.h"
#include "devices.h"

extern void mx25_generic_gpio_init(void);
extern void armadillo400_gpio_init(void);

#if defined(CONFIG_MACH_ARMADILLO460)
static struct pad_desc armadillo460_s35390a_pads[] = {
	MX25_PAD_EB0__GPIO_2_12(0),
};

static void armadillo460_s35390a_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(armadillo460_s35390a_pads,
					 ARRAY_SIZE(armadillo460_s35390a_pads));
	gpio_request(GPIO(2, 12), "A460_RTC_ALM_INT");
	gpio_direction_input(GPIO(2, 12));
}

#if defined(CONFIG_ARMADILLO460_RTC)
static struct s35390a_platform_data armadillo460_s35390a_plat_data = { 
	.alarm_irq = GPIO_TO_IRQ(GPIO(2, 12)),
	.alarm_irq_init = armadillo460_s35390a_init,
#if defined(CONFIG_ARMADILLO460_RTC_ALM_INT_WAKE_SRC_SELECT)
	.alarm_is_wakeup_src = 1,
#endif
};

static struct i2c_board_info armadillo460_ext_i2c_board_info[] __initdata = { 
	{
		.type = "s35390a",
		.addr = 0x30,
		.platform_data = &armadillo460_s35390a_plat_data,
	},
};

static struct i2c_gpio_platform_data armadillo460_ext_i2c_data = {
	.sda_pin = CPLD_GPIO(0),
	.scl_pin = CPLD_GPIO(1),
	.sda_is_open_drain = 1,
	.scl_is_open_drain = 1,
	.scl_is_output_only = 1,
	.udelay = 5,
	.timeout = HZ,
};

static struct platform_device armadillo460_ext_i2c_device = {
	.name = "i2c-gpio",
	.id = ARMADILLO460_EXT_I2C_ID,
};
#endif

static void __init armadillo460_init_ext_i2c(void)
{
#if defined(CONFIG_ARMADILLO460_RTC)
	mxc_register_device(&armadillo460_ext_i2c_device,
			    &armadillo460_ext_i2c_data);
	i2c_register_board_info(armadillo460_ext_i2c_device.id,
				armadillo460_ext_i2c_board_info,
				ARRAY_SIZE(armadillo460_ext_i2c_board_info));
#else
	armadillo460_s35390a_init();
#endif
}
#endif

#if defined(CONFIG_ARMADILLO400_SDHC2_PWREN_CON9_1)

static int __init armadillo400_regulator_setup(void)
{
	struct regulator *sdhc2, *reg5;
	sdhc2 = regulator_get(NULL, "SDHC2");
	reg5 = regulator_get(NULL, "REG5");

	if (regulator_set_platform_source(sdhc2, reg5))
		pr_warning("failed to parent regulator: SDHC2\n");

	return 0;
}
subsys_initcall_sync(armadillo400_regulator_setup);

static struct fixed_voltage_config armadillo400_sdhc2_regulator_config = {
	.supply_name = "SDHC2",
	.gpio = GPIO(3,17),
	.enable_high = 0,
	.enabled_at_boot = 0,
	.startup_delay = 100000,
	.microvolts = 3300000,
};

static struct platform_device armadillo400_sdhc2_regulator = {
        .name = "reg-fixed-voltage",
        .id = 0,
};
#endif

static void __init armadillo400_init_regulator(void)
{
#if defined(CONFIG_ARMADILLO400_SDHC2_PWREN_CON9_1)
	mxc_register_device(&armadillo400_sdhc2_regulator,
			    &armadillo400_sdhc2_regulator_config);
#endif
}

void armadillo400_set_vbus_power(struct fsl_usb2_platform_data *pdata,
				 int on)
{
	static int usb_use_count = 0;

	if (on) {
		if (!usb_use_count) {
			if (regulator_is_enabled(pdata->xcvr_pwr->regu1)) {
				regulator_disable(pdata->xcvr_pwr->regu1);
				gpio_set_value(USB_PWRSEL_GPIO, USB_PWRSRC_5V);
				mdelay(500);
			}

			if (USB_PWRSRC == USB_PWRSRC_5V) {
				/* precharge the capacitor to prevent rush current */
				gpio_set_value(USB_PWRSEL_GPIO, USB_PWRSRC_VIN);
				mdelay(250);

				regulator_enable(pdata->xcvr_pwr->regu1);
				gpio_set_value(USB_PWRSEL_GPIO, USB_PWRSRC_5V);
			} else
				gpio_set_value(USB_PWRSEL_GPIO, USB_PWRSRC_VIN);
		}
		usb_use_count++;
	} else {
		if (usb_use_count > 0 && !(--usb_use_count)) {
			regulator_disable(pdata->xcvr_pwr->regu1);
			if (USB_PWRSRC == USB_PWRSRC_VIN)
				gpio_set_value(USB_PWRSEL_GPIO, USB_PWRSRC_5V);
		}
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

static struct mtd_partition armadillo400_mtd_nor_partitions_8MB[] = {
	MTD_PART("nor.bootloader",   4* 32*1024, 0,
		 MTD_WRITEABLE /* force read-only */ ),
	MTD_PART("nor.kernel",      16*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.userland",    46*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.config",       1*128*1024, MTDPART_OFS_APPEND, 0),
};

static struct mtd_partition armadillo400_mtd_nor_partitions_16MB[] = {
	MTD_PART("nor.bootloader",   4* 32*1024, 0,
		 MTD_WRITEABLE /* force read-only */ ),
	MTD_PART("nor.kernel",      16*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.userland",   110*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.config",       1*128*1024, MTDPART_OFS_APPEND, 0),
};

static struct mtd_partition armadillo400_mtd_nor_partitions_32MB[] = {
	MTD_PART("nor.bootloader",   4* 32*1024, 0,
		 MTD_WRITEABLE /* force read-only */ ),
	MTD_PART("nor.kernel",      16*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.userland",   238*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.config",       1*128*1024, MTDPART_OFS_APPEND, 0),
};

static struct mtd_partition armadillo400_mtd_nor_partitions_64MB[] = {
	MTD_PART("nor.bootloader",   4* 32*1024, 0,
		 MTD_WRITEABLE /* force read-only */ ),
	MTD_PART("nor.kernel",      16*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.userland",   494*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.config",       1*128*1024, MTDPART_OFS_APPEND, 0),
};

static int
armadillo400_mtd_nor_update_partitions(struct map_info *map,
			  struct flash_platform_data *plat)
{
	struct cfi_private *cfi = map->fldrv_priv;

	switch (cfi->cfiq->DevSize) {
	case 23: /* 8MB */
		plat->parts	= armadillo400_mtd_nor_partitions_8MB;
		plat->nr_parts	= ARRAY_SIZE(armadillo400_mtd_nor_partitions_8MB);
		break;
	case 24: /* 16MB */
		break;
	case 25: /* 32MB */
		plat->parts	= armadillo400_mtd_nor_partitions_32MB;
		plat->nr_parts	= ARRAY_SIZE(armadillo400_mtd_nor_partitions_32MB);
		break;
	case 26: /* 64MB */
		plat->parts	= armadillo400_mtd_nor_partitions_64MB;
		plat->nr_parts	= ARRAY_SIZE(armadillo400_mtd_nor_partitions_64MB);
		break;
	default:
		printk("Not support flash-size.\n");
		return -1;
	}

	return 0;
}

static struct armadillo_flash_private_data armadillo400_mtd_nor_data = {
	.plat = {
		.map_name	= "cfi_probe",
		.width		= 2,
		.parts		= armadillo400_mtd_nor_partitions_16MB,
		.nr_parts	= ARRAY_SIZE(armadillo400_mtd_nor_partitions_16MB),
	},
	.update_partitions = armadillo400_mtd_nor_update_partitions,
	.map_name = "armadillo-nor",
};

static void __init armadillo400_mtd_nor_init(void)
{
	mxc_register_device(&mx25_mtd_nor_device, &armadillo400_mtd_nor_data);
}

static struct gpio_led armadillo400_led_pins[] = {
	{"red",    "default-on", GPIO(3, 28), 0},
	{"green",  "default-on", GPIO(3, 29), 0},
	{"yellow", NULL,         GPIO(4, 30), 0},
};

struct gpio_led_platform_data armadillo400_led_data = {
	.leds = armadillo400_led_pins,
	.num_leds = ARRAY_SIZE(armadillo400_led_pins),
};

static struct armadillo2x0_led_platform_data armadillo400_a2x0_compat_led_data = {
	.led_red_data = {
		.gpio = GPIO(3, 28),
		.active_low = 0,
	},
	.led_green_data = {
		.gpio = GPIO(3, 29),
		.active_low = 0,
	},
};

void __init armadillo400_led_init(void)
{
	mxc_register_device(&mx25_gpio_led_device, &armadillo400_led_data);

	mxc_register_device(&mx25_a2x0_compat_led_device,
			    &armadillo400_a2x0_compat_led_data);
}

static struct gpio_keys_button armadillo400_key_buttons[] = {
#if !defined(CONFIG_ARMADILLO400_SELECT_MUX_AS_A410)
	{KEY_ENTER, GPIO(3, 30), 1, "SW1",     EV_KEY, CONFIG_ARMADILLO400_SW1_GPIO_3_30_IS_WAKE_SRC},
#if defined(CONFIG_MACH_ARMADILLO410) || defined(CONFIG_MACH_ARMADILLO440) || defined(CONFIG_MACH_ARMADILLO460)
	{KEY_BACK,  GPIO(2, 20), 1, "LCD_SW1", EV_KEY, CONFIG_ARMADILLO400_CON11_39_GPIO_2_20_IS_WAKE_SRC},
#if defined(CONFIG_ARMADILLO400_CON11_40_GPIO_2_29)
	{KEY_MENU,  GPIO(2, 29), 1, "LCD_SW2", EV_KEY, CONFIG_ARMADILLO400_CON11_40_GPIO_2_29_IS_WAKE_SRC},
#endif
#if defined(CONFIG_ARMADILLO400_CON11_41_GPIO_2_30)
	{KEY_HOME,  GPIO(2, 30), 1, "LCD_SW3", EV_KEY, CONFIG_ARMADILLO400_CON11_41_GPIO_2_30_IS_WAKE_SRC},
#endif
#endif /* CONFIG_MACH_ARMADILLO410 || CONFIG_MACH_ARMADILLO440 || CONFIG_MACH_ARMADILLO460 */
#endif /* CONFIG_ARMADILLO400_SELECT_MUX_AS_A410 */
};

static struct gpio_keys_platform_data armadillo400_gpio_key_data = {
	.buttons = armadillo400_key_buttons,
	.nbuttons = ARRAY_SIZE(armadillo400_key_buttons),
	.wakeup_default_disabled = !CONFIG_ARMADILLO400_GPIO_KEYS_IS_WAKE_SRC,
};

static void __init armadillo400_key_init(void)
{
	mxc_register_device(&mx25_gpio_key_device, &armadillo400_gpio_key_data);
}

static struct mxc_ext_gpio gpio_list_revb[] = {
#if defined(CONFIG_ARMADILLO400_CON9_21_GPIO1_8)
	{"CON9_21",  GPIO(1, 8),  MXC_EXT_GPIO_DIRECTION_INPUT}, /* EXT_GPIO0*/
#endif
#if defined(CONFIG_ARMADILLO400_CON9_22_GPIO1_9)
	{"CON9_22",  GPIO(1, 9),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_23_GPIO1_10)
	{"CON9_23",  GPIO(1, 10), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_24_GPIO1_11)
	{"CON9_24",  GPIO(1, 11), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_25_GPIO1_16)
	{"CON9_25",  GPIO(1, 16), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_26_GPIO)
	{"CON9_26",  GPIO(2, 22), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_27_GPIO2_21)
	{"CON9_27",  GPIO(2, 21), MXC_EXT_GPIO_DIRECTION_OUTPUT, 0},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_28_GPIO3_15)
	{"CON9_28",  GPIO(3, 15), MXC_EXT_GPIO_DIRECTION_OUTPUT, 0},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_11_GPIO1_17)
	{"CON9_11",  GPIO(1, 17), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_12_GPIO1_29)
	{"CON9_12",  GPIO(1, 29), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_13_GPIO1_18)
	{"CON9_13",  GPIO(1, 18), MXC_EXT_GPIO_DIRECTION_INPUT}, /* EXT_GPIO10 */
#endif
#if defined(CONFIG_ARMADILLO400_CON9_14_GPIO1_30)
	{"CON9_14",  GPIO(1, 30), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_15_GPIO1_7)
	{"CON9_15",  GPIO(1, 7),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_16_GPIO1_31)
	{"CON9_16",  GPIO(1, 31), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_17_GPIO4_21)
	{"CON9_17",  GPIO(4, 21), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_18_GPIO1_6)
	{"CON9_18",  GPIO(1, 6),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON14_3_GPIO1_2)
	{"CON9_1",   GPIO(1, 2),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON14_4_GPIO1_3)
	{"CON9_2",   GPIO(1, 3),  MXC_EXT_GPIO_DIRECTION_INPUT}, /* EXT_GPIO17 */
#endif
#if defined(CONFIG_ARMADILLO400_CON11_42_GPIO_2_31)
	{"CON11_42", GPIO(2, 31), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON11_43_GPIO_3_0)
	{"CON11_43", GPIO(3, 0),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON11_44_GPIO_3_1)
	{"CON11_44", GPIO(3, 1),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON11_45_GPIO_3_2)
	{"CON11_45", GPIO(3, 2),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON11_46_GPIO_3_3)
	{"CON11_46", GPIO(3, 3),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON11_47_GPIO_3_4)
	{"CON11_47", GPIO(3, 4),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON11_48_GPIO1_0)
	{"CON11_48", GPIO(1, 0),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON11_49_GPIO1_1)
	{"CON11_49", GPIO(1, 1),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_3_GPIO1_14)
	{"CON9_3",   GPIO(1, 14), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_4_GPIO1_27)
	{"CON9_4",   GPIO(1, 27), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_5_GPIO1_15)
	{"CON9_5",   GPIO(1, 15), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_6_GPIO1_28)
	{"CON9_6",   GPIO(1, 28), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
};

static struct mxc_ext_gpio gpio_list_revc[] = {
#if defined(CONFIG_ARMADILLO400_CON9_21_GPIO1_8)
	{"CON9_21",  GPIO(1, 8),  MXC_EXT_GPIO_DIRECTION_INPUT}, /* EXT_GPIO0*/
#endif
#if defined(CONFIG_ARMADILLO400_CON9_22_GPIO1_9)
	{"CON9_22",  GPIO(1, 9),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_23_GPIO1_10)
	{"CON9_23",  GPIO(1, 10), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_24_GPIO1_11)
	{"CON9_24",  GPIO(1, 11), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_25_GPIO1_16)
	{"CON9_25",  GPIO(1, 16), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_26_GPIO)
	{"CON9_26",  GPIO(2, 22), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_27_GPIO2_21)
	{"CON9_27",  GPIO(2, 21), MXC_EXT_GPIO_DIRECTION_OUTPUT, 0},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_28_GPIO3_15)
	{"CON9_28",  GPIO(3, 15), MXC_EXT_GPIO_DIRECTION_OUTPUT, 0},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_11_GPIO1_17)
	{"CON9_11",  GPIO(1, 17), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_12_GPIO1_29)
	{"CON9_12",  GPIO(1, 29), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_13_GPIO1_18)
	{"CON9_13",  GPIO(1, 18), MXC_EXT_GPIO_DIRECTION_INPUT}, /* EXT_GPIO10 */
#endif
#if defined(CONFIG_ARMADILLO400_CON9_14_GPIO1_30)
	{"CON9_14",  GPIO(1, 30), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_15_GPIO1_7)
	{"CON9_15",  GPIO(1, 7),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_16_GPIO1_31)
	{"CON9_16",  GPIO(1, 31), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_17_GPIO4_21)
	{"CON9_17",  GPIO(4, 21), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_18_GPIO1_6)
	{"CON9_18",  GPIO(1, 6),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_1_GPIO3_17)
	{"CON9_1",   GPIO(3, 17), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_2_GPIO3_14)
	{"CON9_2",   GPIO(3, 14), MXC_EXT_GPIO_DIRECTION_INPUT}, /* EXT_GPIO17 */
#endif
#if defined(CONFIG_ARMADILLO400_CON11_42_GPIO_2_31)
	{"CON11_42", GPIO(2, 31), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON11_43_GPIO_3_0)
	{"CON11_43", GPIO(3, 0),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON11_44_GPIO_3_1)
	{"CON11_44", GPIO(3, 1),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON11_45_GPIO_3_2)
	{"CON11_45", GPIO(3, 2),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON11_46_GPIO_3_3)
	{"CON11_46", GPIO(3, 3),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON11_47_GPIO_3_4)
	{"CON11_47", GPIO(3, 4),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON11_48_GPIO1_0)
	{"CON11_48", GPIO(1, 0),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON11_49_GPIO1_1)
	{"CON11_49", GPIO(1, 1),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON14_3_GPIO1_2)
	{"CON14_3",   GPIO(1, 2), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON14_4_GPIO1_3)
	{"CON14_4",   GPIO(1, 3), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_3_GPIO1_14)
	{"CON9_3",   GPIO(1, 14), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_4_GPIO1_27)
	{"CON9_4",   GPIO(1, 27), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_5_GPIO1_15)
	{"CON9_5",   GPIO(1, 15), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO400_CON9_6_GPIO1_28)
	{"CON9_6",   GPIO(1, 28), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
/* Armadillo-410 CON2 */
#if defined(CONFIG_ARMADILLO410_CON2_73_GPIO1_8)
	{"CON2_73",  GPIO(1, 8),  MXC_EXT_GPIO_DIRECTION_INPUT}, /* EXT_GPIO0*/
#endif
#if defined(CONFIG_ARMADILLO410_CON2_72_GPIO1_9)
	{"CON2_72",  GPIO(1, 9),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_71_GPIO1_10)
	{"CON2_71",  GPIO(1, 10), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_70_GPIO1_11)
	{"CON2_70",  GPIO(1, 11), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_69_GPIO1_16)
	{"CON2_69",  GPIO(1, 16), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_68_GPIO)
	{"CON2_68",  GPIO(2, 22), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_67_GPIO2_21)
	{"CON2_67",  GPIO(2, 21), MXC_EXT_GPIO_DIRECTION_OUTPUT, 0},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_66_GPIO3_15)
	{"CON2_66",  GPIO(3, 15), MXC_EXT_GPIO_DIRECTION_OUTPUT, 0},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_81_GPIO1_17)
	{"CON2_81",  GPIO(1, 17), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_80_GPIO1_29)
	{"CON2_80",  GPIO(1, 29), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_79_GPIO1_18)
	{"CON2_79",  GPIO(1, 18), MXC_EXT_GPIO_DIRECTION_INPUT}, /* EXT_GPIO10 */
#endif
#if defined(CONFIG_ARMADILLO410_CON2_78_GPIO1_30)
	{"CON2_78",  GPIO(1, 30), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_77_GPIO1_7)
	{"CON2_77",  GPIO(1, 7),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_76_GPIO1_31)
	{"CON2_76",  GPIO(1, 31), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_75_GPIO4_21)
	{"CON2_75",  GPIO(4, 21), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_74_GPIO1_6)
	{"CON2_74",  GPIO(1, 6),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_87_GPIO3_17)
	{"CON2_87",   GPIO(3, 17), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_86_GPIO3_14)
	{"CON2_86",   GPIO(3, 14), MXC_EXT_GPIO_DIRECTION_INPUT}, /* EXT_GPIO17 */
#endif
#if defined(CONFIG_ARMADILLO410_CON2_60_GPIO_2_31)
	{"CON2_60", GPIO(2, 31), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_59_GPIO_3_0)
	{"CON2_59", GPIO(3, 0),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_58_GPIO_3_1)
	{"CON2_58", GPIO(3, 1),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_57_GPIO_3_2)
	{"CON2_57", GPIO(3, 2),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_56_GPIO_3_3)
	{"CON2_56", GPIO(3, 3),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_55_GPIO_3_4)
	{"CON2_55", GPIO(3, 4),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_54_GPIO1_0)
	{"CON2_54", GPIO(1, 0),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_53_GPIO1_1)
	{"CON2_53", GPIO(1, 1),  MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_65_GPIO1_2)
	{"CON2_65",   GPIO(1, 2), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_64_GPIO1_3)
	{"CON2_64",   GPIO(1, 3), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_85_GPIO1_14)
	{"CON2_85",   GPIO(1, 14), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_84_GPIO1_27)
	{"CON2_84",   GPIO(1, 27), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_83_GPIO1_15)
	{"CON2_83",   GPIO(1, 15), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_82_GPIO1_28)
	{"CON2_82",   GPIO(1, 28), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_63_GPIO_2_20)
	{"CON2_63",   GPIO(2, 20), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_62_GPIO_2_29)
	{"CON2_62",   GPIO(2, 29), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_61_GPIO_2_30)
	{"CON2_61",   GPIO(2, 30), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
#if defined(CONFIG_ARMADILLO410_CON2_43_GPIO3_30)
	{"CON2_43",   GPIO(3, 30), MXC_EXT_GPIO_DIRECTION_INPUT},
#endif
};

#define ARMADILLO2X0_GPIO_PIN_INPUT(_no, _gpio, _can_interrupt)         \
	{                                                               \
		.no = _no,                                              \
		.gpio = _gpio,                                          \
		.can_interrupt = _can_interrupt,                        \
		.default_direction = ARMDILLO2X0_GPIO_DIRECTION_INPUT,  \
	}

#define ARMADILLO2X0_GPIO_PIN_OUTPUT_LOW(_no, _gpio, _can_interrupt)    \
	{                                                               \
		.no = _no,                                              \
		.gpio = _gpio,                                          \
		.can_interrupt = _can_interrupt,                        \
		.default_direction = ARMDILLO2X0_GPIO_DIRECTION_OUTPUT, \
		.default_value = 0,                                     \
	}

static struct armadillo2x0_gpio_info armadillo400_a2x0_compat_gpio_info[] = {
#if defined(CONFIG_ARMADILLO400_CON9_21_GPIO1_8)
	ARMADILLO2X0_GPIO_PIN_INPUT(GPIO0,  GPIO(1, 8), 1),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_22_GPIO1_9)
	ARMADILLO2X0_GPIO_PIN_INPUT(GPIO1,  GPIO(1, 9), 1),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_23_GPIO1_10)
	ARMADILLO2X0_GPIO_PIN_INPUT(GPIO2,  GPIO(1, 10), 1),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_24_GPIO1_11)
	ARMADILLO2X0_GPIO_PIN_INPUT(GPIO3,  GPIO(1, 11), 1),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_25_GPIO1_16)
	ARMADILLO2X0_GPIO_PIN_INPUT(GPIO4,  GPIO(1, 16), 1),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_26_GPIO)
	ARMADILLO2X0_GPIO_PIN_INPUT(GPIO5,  GPIO(2, 22), 1),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_27_GPIO2_21)
	ARMADILLO2X0_GPIO_PIN_OUTPUT_LOW(GPIO6,  GPIO(2, 21), 1),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_28_GPIO3_15)
	ARMADILLO2X0_GPIO_PIN_OUTPUT_LOW(GPIO7,  GPIO(3, 15), 1),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_11_GPIO1_17)
	ARMADILLO2X0_GPIO_PIN_INPUT(GPIO8,  GPIO(1, 17), 1),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_12_GPIO1_29)
	ARMADILLO2X0_GPIO_PIN_INPUT(GPIO9,  GPIO(1, 29), 1),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_13_GPIO1_18)
	ARMADILLO2X0_GPIO_PIN_INPUT(GPIO10, GPIO(1, 18), 1),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_14_GPIO1_30)
	ARMADILLO2X0_GPIO_PIN_INPUT(GPIO11, GPIO(1, 30), 1),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_15_GPIO1_7)
	ARMADILLO2X0_GPIO_PIN_INPUT(GPIO12, GPIO(1, 7), 1),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_16_GPIO1_31)
	ARMADILLO2X0_GPIO_PIN_INPUT(GPIO13, GPIO(1, 31), 1),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_17_GPIO4_21)
	ARMADILLO2X0_GPIO_PIN_INPUT(GPIO14, GPIO(4, 21), 1),
#endif
#if defined(CONFIG_ARMADILLO400_CON9_18_GPIO1_6)
	ARMADILLO2X0_GPIO_PIN_INPUT(GPIO15, GPIO(1, 6), 1),
#endif
};

static struct armadillo2x0_gpio_platform_data armadillo400_a2x0_compat_gpio_plat_data = {
	.gpio_num = ARRAY_SIZE(armadillo400_a2x0_compat_gpio_info),
	.gpio_info = armadillo400_a2x0_compat_gpio_info,
};


void __init armadillo400_ext_gpio_init(void)
{
#if defined(CONFIG_GPIO_SYSFS)
	switch (system_rev & ARMADILLO_400_BOARD_REV_MAJOR_MASK) {
	case ARMADILLO_400_BOARD_REV_B:
		gpio_activate_ext_gpio(gpio_list_revb, ARRAY_SIZE(gpio_list_revb));
		break;
	default:
		pr_warning("Unknown Board Revision 0x%x\n", system_rev);
		/* fall through */
	case ARMADILLO_400_BOARD_REV_C:
		gpio_activate_ext_gpio(gpio_list_revc, ARRAY_SIZE(gpio_list_revc));
		break;
	}
#endif

	mxc_register_device(&mx25_a2x0_compat_gpio_device,
			    &armadillo400_a2x0_compat_gpio_plat_data);
}

#if defined(CONFIG_I2C_MXC_SELECT1)
static struct mxc_i2c_platform_data armadillo400_i2c1_data = {
	.i2c_clk = 40000,
};

static struct i2c_board_info armadillo400_i2c1_board_info[] __initdata = {
	{
		.type = "mc34704",
		.addr = 0x54,
	},
};
#endif /* defined(CONFIG_I2C_MXC_SELECT1) */

#if defined(CONFIG_I2C_MXC_SELECT2)
#if defined(CONFIG_ARMADILLO400_CON9_2_RTC_ALM_INT) || defined(CONFIG_ARMADILLO410_CON2_86_RTC_ALM_INT)
extern void gpio_rtc_alarm_int_active(void);

static struct s35390a_platform_data armadillo400_i2c2_s35390a_plat_data = {
	.alarm_irq = GPIO_TO_IRQ(GPIO(3, 14)),
	.alarm_irq_init = gpio_rtc_alarm_int_active,
	.alarm_is_wakeup_src = CONFIG_ARMADILLO400_RTC_ALM_INT_IS_WAKE_SRC,
};
#endif

static struct i2c_board_info armadillo400_i2c2_board_info[] __initdata = {
#if defined(CONFIG_ARMADILLO400_I2C2_CON14_S35390A) || defined(CONFIG_ARMADILLO410_I2C2_CON2_S35390A)
	{
		.type = "s35390a",
		.addr = 0x30,
#if defined(CONFIG_ARMADILLO400_CON9_2_RTC_ALM_INT) || defined(CONFIG_ARMADILLO410_CON2_86_RTC_ALM_INT)
		.platform_data = &armadillo400_i2c2_s35390a_plat_data,
#endif
	},
#endif
};

static struct mxc_i2c_platform_data armadillo400_i2c2_data = {
	.i2c_clk = 40000,
};
#endif /* defined(CONFIG_I2C_MXC_SELECT2) */

#if defined(CONFIG_I2C_MXC_SELECT3)
static struct mxc_i2c_platform_data armadillo400_i2c3_data = {
	.i2c_clk = 40000,
};

static struct i2c_board_info armadillo400_i2c3_board_info[] __initdata = {
#if defined(CONFIG_ARMADILLO400_I2C3_CON11_S35390A) || defined(CONFIG_ARMADILLO410_I2C3_CON2_S35390A)
	{
		.type = "s35390a",
		.addr = 0x30,
	},
#endif
};
#endif /* defined(CONFIG_I2C_MXC_SELECT3) */

static void __init armadillo400_init_i2c(void)
{
#if defined(CONFIG_I2C_MXC_SELECT1)
	mxc_register_device(&mx25_i2c1_device,
			    &armadillo400_i2c1_data);
	i2c_register_board_info(0, armadillo400_i2c1_board_info,
				ARRAY_SIZE(armadillo400_i2c1_board_info));
#endif /* defined(CONFIG_I2C_MXC_SELECT1) */

#if defined(CONFIG_I2C_MXC_SELECT2)
	mxc_register_device(&mx25_i2c2_device,
			    &armadillo400_i2c2_data);
	i2c_register_board_info(1, armadillo400_i2c2_board_info,
				ARRAY_SIZE(armadillo400_i2c2_board_info));
#endif /* defined(CONFIG_I2C_MXC_SELECT2) */

#if defined(CONFIG_I2C_MXC_SELECT3)
	mxc_register_device(&mx25_i2c3_device,
			    &armadillo400_i2c3_data);
	i2c_register_board_info(2, armadillo400_i2c3_board_info,
				ARRAY_SIZE(armadillo400_i2c3_board_info));
#endif /* defined(CONFIG_I2C_MXC_SELECT3) */
}

#if defined(CONFIG_SPI_MXC_SELECT1)
extern void gpio_spi1_cs_active(int cspi_mode, int chipselect);
extern void gpio_spi1_cs_inactive(int cspi_mode, int chipselect);

static struct mxc_spi_master armadillo400_spi1_data = {
	.maxchipselect = 4,
	.spi_version = 7,
	.chipselect_active = gpio_spi1_cs_active,
	.chipselect_inactive = gpio_spi1_cs_inactive,
};

static struct spi_board_info armadillo400_spi1_board_info[] __initdata = {
};
#endif

#if defined(CONFIG_SPI_MXC_SELECT2)
static struct mxc_spi_master armadillo400_spi2_data = {
	.maxchipselect = 4,
	.spi_version = 7,
};

static struct spi_board_info armadillo400_spi2_board_info[] __initdata = {
};
#endif

#if defined(CONFIG_SPI_MXC_SELECT3)
extern void gpio_spi3_cs_active(int cspi_mode, int chipselect);
extern void gpio_spi3_cs_inactive(int cspi_mode, int chipselect);

static struct mxc_spi_master armadillo400_spi3_data = {
	.maxchipselect = 4,
	.spi_version = 7,
	.chipselect_active = gpio_spi3_cs_active,
	.chipselect_inactive = gpio_spi3_cs_inactive,
};

static struct spi_board_info armadillo400_spi3_board_info[] __initdata = {
};
#endif

static void __init armadillo400_init_spi(void)
{
#if defined(CONFIG_SPI_MXC_SELECT1)
	mxc_register_device(&mx25_spi1_device, &armadillo400_spi1_data);
	spi_register_board_info(armadillo400_spi1_board_info, ARRAY_SIZE(armadillo400_spi1_board_info));
#else
#endif
#if defined(CONFIG_SPI_MXC_SELECT2)
	mxc_register_device(&mx25_spi2_device, &armadillo400_spi2_data);
	spi_register_board_info(armadillo400_spi2_board_info, ARRAY_SIZE(armadillo400_spi2_board_info));
#endif
#if defined(CONFIG_SPI_MXC_SELECT3)
	mxc_register_device(&mx25_spi3_device, &armadillo400_spi3_data);
	spi_register_board_info(armadillo400_spi3_board_info, ARRAY_SIZE(armadillo400_spi3_board_info));
#endif
}

extern void gpio_fec_suspend(void);
extern void gpio_fec_resume(void);

static int armadillo400_fec_suspend(struct platform_device *pdev, pm_message_t state)
{
	gpio_fec_suspend();

	return 0;
}

static int armadillo400_fec_resume(struct platform_device *pdev)
{
	gpio_fec_resume();

	return 0;
}

static struct fec_platform_data armadillo400_fec_data = {
	.suspend = armadillo400_fec_suspend,
	.resume  = armadillo400_fec_resume,
};

static int __init armadillo400_init_fec(void)
{
	mxc_register_device(&mx25_fec_device, &armadillo400_fec_data);
	return 0;
}
late_initcall(armadillo400_init_fec);

extern unsigned int sdhc_get_card_det_status(struct device *dev);
extern int sdhc_get_write_protect(struct device *dev);

#if defined(CONFIG_MMC_MXC_SELECT1) | defined(CONFIG_MMC_MXC_SELECT2)
extern void gpio_sdhc_active(int module);
extern void gpio_sdhc_inactive(int module);
#endif

#if defined(CONFIG_MMC_MXC_SELECT1)

static struct mxc_mmc_platform_data armadillo400_sdhc1_data = {
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
static struct mxc_mmc_platform_data armadillo400_sdhc2_data = {
	.ocr_mask = MMC_VDD_29_30 | MMC_VDD_32_33,
	.caps = MMC_CAP_4_BIT_DATA,
	.min_clk = 400000,
	.max_clk = 52000000,
	.card_inserted_state = 0,
	.status = sdhc_get_card_det_status,
	.wp_status = sdhc_get_write_protect,
	.clock_mmc = "esdhc_clk",
#if defined(CONFIG_ARMADILLO400_SDHC2_PWREN_CON9_1) || defined(CONFIG_ARMADILLO410_SDHC2_PWREN_CON2_87)
	.power_mmc = "SDHC2",
#endif
};
#endif /* defined(CONFIG_MMC_MXC_SELECT2) */

static void __init armadillo400_init_sdhc(void)
{
#if defined(CONFIG_MMC_MXC_SELECT1)
	if (machine_is_armadillo460())
		sdhc1_wp_gpio = SDHC1_WP_GPIO_A460;
	mx25_sdhc1_device.resource[2].start = GPIO_TO_IRQ(SDHC1_CD_GPIO);
	mx25_sdhc1_device.resource[2].end = GPIO_TO_IRQ(SDHC1_CD_GPIO);
	mxc_register_device(&mx25_sdhc1_device,
			    &armadillo400_sdhc1_data);
	gpio_sdhc_active(0);
	gpio_sdhc_inactive(0);
#endif

#if defined(CONFIG_MMC_MXC_SELECT2)
	mx25_sdhc2_device.resource[2].start = GPIO_TO_IRQ(SDHC2_CD_GPIO);
	mx25_sdhc2_device.resource[2].end = GPIO_TO_IRQ(SDHC2_CD_GPIO);
	mxc_register_device(&mx25_sdhc2_device,
			    &armadillo400_sdhc2_data);
	gpio_sdhc_active(1);
	gpio_sdhc_inactive(1);
#endif
}

#if defined(CONFIG_ARMADILLO400_CON9_26_W1) || defined(CONFIG_ARMADILLO410_CON2_68_W1)
static struct w1_gpio_platform_data armadillo400_gpio_w1_data = {
	.pin		= GPIO(2, 22),
	.is_open_drain	= 0,
};
#endif

static void __init armadillo400_init_gpio_w1(void)
{
#if defined(CONFIG_ARMADILLO400_CON9_26_W1) || defined(CONFIG_ARMADILLO410_CON2_68_W1)
	mxc_register_device(&mx25_gpio_w1_device,
			    &armadillo400_gpio_w1_data);
#endif
}

static struct mxc_w1_config armadillo400_mxc_w1_data __maybe_unused = {
	.search_rom_accelerator = 0,
};

static void __init armadillo400_init_mxc_w1(void)
{
#if defined(CONFIG_ARMADILLO400_CON9_2_W1) || defined(CONFIG_ARMADILLO410_CON2_86_W1)
	mxc_register_device(&mx25_mxc_w1_device,
			    &armadillo400_mxc_w1_data);
#endif
}

extern void gpio_flexcan_active(int id);
extern void gpio_flexcan_inactive(int id);

#if defined(CONFIG_FLEXCAN_SELECT1)
struct flexcan_platform_data armadillo400_flexcan1_data = {
        .core_reg = NULL,
        .io_reg = NULL,
        .active = gpio_flexcan_active,
        .inactive = gpio_flexcan_inactive,
};
#endif

#if defined(CONFIG_FLEXCAN_SELECT2)
struct flexcan_platform_data armadillo400_flexcan2_data = {
        .core_reg = NULL,
        .io_reg = NULL,
        .active = gpio_flexcan_active,
        .inactive = gpio_flexcan_inactive,
};
#endif

static void __init armadillo400_init_flexcan(void)
{
#if defined(CONFIG_FLEXCAN_SELECT1)
	mxc_register_device(&mx25_flexcan1_device,
			    &armadillo400_flexcan1_data);
#endif

#if defined(CONFIG_FLEXCAN_SELECT2)
	mxc_register_device(&mx25_flexcan2_device,
			    &armadillo400_flexcan2_data);
#endif
}

static void __init armadillo440_init_fb(void)
{
#if defined(CONFIG_FB_MXC) || defined(CONFIG_FB_MXC_MODULE)
	mxc_register_device(&mx25_fb_device, CONFIG_FB_MXC_DEFAULT_VIDEOMODE);
#endif
}

static struct platform_pwm_data armadillo400_pwm1_data __maybe_unused = {
	.name = "pwm_bl",
	.invert = 1,
	.export = 0,
};

static struct platform_pwm_data armadillo400_pwm2_data __maybe_unused = {
#if !defined(CONFIG_ARMADILLO410_CON2_69_PWMO2)
	.name = "CON9_25",
#else
	.name = "CON2_69",
#endif
	.invert = 0,
	.export = 1,
};

static struct platform_pwm_data armadillo400_pwm4_data __maybe_unused = {
#if !defined(CONFIG_ARMADILLO410_CON2_65_PWMO4)
	.name = "CON14_3",
#else
	.name = "CON2_65",
#endif
	.invert = 0,
	.export = 1,
};

static void armadillo400_init_pwm(void)
{
#if defined(CONFIG_MXC_PWM_SELECT1)
	mxc_register_device(&mx25_pwm1_device, &armadillo400_pwm1_data);
#endif
#if defined(CONFIG_MXC_PWM_SELECT2)
	mxc_register_device(&mx25_pwm2_device, &armadillo400_pwm2_data);
#endif
#if defined(CONFIG_MXC_PWM_SELECT3)
	mxc_register_device(&mx25_pwm3_device, &armadillo400_pwm3_data);
#endif
#if defined(CONFIG_MXC_PWM_SELECT4)
	mxc_register_device(&mx25_pwm4_device, &armadillo400_pwm4_data);
#endif
}

static struct platform_pwm_backlight_data armadillo440_pwm_backlight_data = {
	.pwm_id = 0,
	.max_brightness = 255,
	.dft_brightness = 255,
	.default_on = 1,
	.pwm_period_ns = 10*1000*1000,
};

static void armadillo440_init_pwm_backlight(void)
{
	mxc_register_device(&mx25_pwm_backlight_device,
			    &armadillo440_pwm_backlight_data);
}

static void armadillo440_init_audio(void)
{
	mxc_register_device(&armadillo440_wm8978_audio_device, NULL);
}

static struct platform_imx_adc_data armadillo440_adc_data = {
	.is_wake_src = 0,
};

static void __init armadillo440_init_adc(void)
{
	mxc_register_device(&mx25_adc_device, &armadillo440_adc_data);
}

static struct platform_imx_adc_ts_data armadillo440_adc_ts_data = {
	.is_wake_src = CONFIG_ARMADILLO400_TOUCHSCREEN_IS_WAKE_SRC,
	.regu_name = "REG5",
};

static void __init armadillo440_init_touchscreen(void)
{
	mxc_register_device(&mx25_adc_ts_device, &armadillo440_adc_ts_data);
}

#if defined(CONFIG_KEYBOARD_MXC) || defined(CONFIG_KEYBOARD_MXC_MODULE)
static u16 armadillo440_keymapping[] = {
	KEY_A,	KEY_B,	KEY_C,
	KEY_D,	KEY_E,	KEY_F,
	KEY_G,	KEY_H,	KEY_I,
	KEY_J,	KEY_K,	KEY_L,
	KEY_M,	KEY_N,	KEY_O,
	KEY_P,	KEY_Q,	KEY_R,
	KEY_S,	KEY_T,	KEY_U,
	KEY_V,	KEY_W,	KEY_X,
};

static struct keypad_data armadillo440_keypad_data = {
	.row_first = 0,
	.row_last = 5,
	.col_first = 0,
	.col_last = 3,
	.matrix = armadillo440_keymapping,
};
#endif

static void __init armadiloo440_init_kpp(void)
{
#if defined(CONFIG_KEYBOARD_MXC) || defined(CONFIG_KEYBOARD_MXC_MODULE)
	mxc_register_device(&mx25_keypad_device,
			    &armadillo440_keypad_data);
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
static void __init fixup_armadillo400_board(struct machine_desc *desc, struct tag *tags,
				   char **cmdline, struct meminfo *mi)
{
	mxc_cpu_init();
}

/*!
 * Armadillo-400 common initialization.
 */
static void __init armadillo400_init(void)
{
	mxc_cpu_common_init();
	mxc_clocks_init();
	armadillo400_gpio_init();
	mxc_gpio_init();
#if defined(CONFIG_GENERIC_GPIO)
	mx25_generic_gpio_init();
#endif
	early_console_setup(saved_command_line);

	armadillo400_init_regulator();
	armadillo400_init_i2c();
	armadillo400_init_spi();
	armadillo400_mtd_nor_init();
	armadillo400_led_init();
	armadillo400_key_init();
	armadillo400_ext_gpio_init();
	armadillo400_init_sdhc();
	armadillo400_init_gpio_w1();
	armadillo400_init_pwm();
	armadillo400_init_mxc_w1();
	armadillo400_init_flexcan();
}

/*!
 * Board specific initialization.
 */
#if defined(CONFIG_MACH_ARMADILLO420)
static void __init armadillo420_init(void)
{
	armadillo400_init();

#if defined(CONFIG_ARMADILLO400_AUD6_CON9) || defined(CONFIG_ARMADILLO410_AUD6_CON2)
	armadillo440_init_audio();
#endif
}
#endif

/*!
 * Board specific initialization.
 */
#if defined(CONFIG_MACH_ARMADILLO410) || defined(CONFIG_MACH_ARMADILLO440) || defined(CONFIG_MACH_ARMADILLO460)
static void __init armadillo440_init(void)
{
	armadillo400_init();

	armadillo440_init_fb();
	armadillo440_init_pwm_backlight();
	armadillo440_init_audio();
	armadillo440_init_adc();
	armadillo440_init_touchscreen();
	armadiloo440_init_kpp();
}
#endif

/*!
 * Board specific initialization.
 */
#if defined(CONFIG_MACH_ARMADILLO460)
extern void armadillo460_init_cpld(void);
static void __init armadillo460_init(void)
{
	armadillo440_init();

	armadillo460_init_cpld();
	armadillo460_init_ext_i2c();
}
#endif

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
	.fixup = fixup_armadillo400_board,
	.map_io = mxc_map_io,
	.init_irq = mxc_init_irq,
	/* Initialization for Armadillo-410 is same as initialization for armadillo-440. */
	.init_machine = armadillo440_init,
	.timer = &mxc_timer,
MACHINE_END
#endif

#if defined(CONFIG_MACH_ARMADILLO420)
/*
 * The following uses standard kernel macros define in arch.h in order to
 * initialize __mach_desc_ARMADILLO420 data structure.
 */
MACHINE_START(ARMADILLO420, "Armadillo-420")
	/* Maintainer: Atmark Techno, Inc. */
	.phys_io = AIPS1_BASE_ADDR,
	.io_pg_offst = ((AIPS1_BASE_ADDR_VIRT) >> 18) & 0xfffc,
	.boot_params = PHYS_OFFSET + 0x100,
	.fixup = fixup_armadillo400_board,
	.map_io = mxc_map_io,
	.init_irq = mxc_init_irq,
	.init_machine = armadillo420_init,
	.timer = &mxc_timer,
MACHINE_END
#endif

#if defined(CONFIG_MACH_ARMADILLO440)
/*
 * The following uses standard kernel macros define in arch.h in order to
 * initialize __mach_desc_ARMADILLO420 data structure.
 */
MACHINE_START(ARMADILLO440, "Armadillo-440")
	/* Maintainer: Atmark Techno, Inc. */
	.phys_io = AIPS1_BASE_ADDR,
	.io_pg_offst = ((AIPS1_BASE_ADDR_VIRT) >> 18) & 0xfffc,
	.boot_params = PHYS_OFFSET + 0x100,
	.fixup = fixup_armadillo400_board,
	.map_io = mxc_map_io,
	.init_irq = mxc_init_irq,
	.init_machine = armadillo440_init,
	.timer = &mxc_timer,
MACHINE_END
#endif

#if defined(CONFIG_MACH_ARMADILLO460)
/*
 * The following uses standard kernel macros define in arch.h in order to
 * initialize __mach_desc_ARMADILLO460 data structure.
 */
MACHINE_START(ARMADILLO460, "Armadillo-460")
	/* Maintainer: Atmark Techno, Inc. */
	.phys_io = AIPS1_BASE_ADDR,
	.io_pg_offst = ((AIPS1_BASE_ADDR_VIRT) >> 18) & 0xfffc,
	.boot_params = PHYS_OFFSET + 0x100,
	.fixup = fixup_armadillo400_board,
	.map_io = mxc_map_io,
	.init_irq = mxc_init_irq,
	.init_machine = armadillo460_init,
	.timer = &mxc_timer,
MACHINE_END
#endif

