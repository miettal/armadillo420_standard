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

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/nodemask.h>
#include <linux/clk.h>
#include <linux/spi/spi.h>
#include <linux/spi/ads7846.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/smsc911x.h>

#include <asm/arch/system.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <asm/mach/keypad.h>
#include <asm/arch/memory.h>
#include <asm/arch/gpio.h>
#include <asm/arch/wsimpm.h>

#if defined(CONFIG_MTD) || defined(CONFIG_MTD_MODULE)
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/cfi.h>
#include <asm/mach/flash.h>
#include <asm/arch/mtd.h>
#endif

#include "crm_regs.h"

extern void mxc_map_io(void);
extern void mxc_init_irq(void);
extern struct sys_timer mxc_timer;

extern void mxc_cpu_common_init(void);
extern int mxc_clocks_init(void);
extern void armadillo500fx_gpio_init(void);

static struct resource armadillo500fx_smc911x_resources[] = {
	[0] = {
		.start	= CS3_BASE_ADDR,
		.end	= CS3_BASE_ADDR + SZ_32M - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= MXC_INT_GPIO_P1(0),
		.end	= MXC_INT_GPIO_P1(0),
		.flags	= IORESOURCE_IRQ,
	},
};
static struct smsc911x_platform_config smsc911x_config = {
	.irq_polarity	= SMSC911X_IRQ_POLARITY_ACTIVE_LOW,
	.irq_type	= SMSC911X_IRQ_TYPE_OPEN_DRAIN,
	.flags		= SMSC911X_USE_32BIT,
};
static struct platform_device armadillo500fx_smc911x_device = {
	.name		= "smsc911x",
	.id		= 0,
	.dev		= {
		.platform_data = &smsc911x_config,
	},
	.num_resources	= ARRAY_SIZE(armadillo500fx_smc911x_resources),  
	.resource	= armadillo500fx_smc911x_resources,
};

static void
armadillo500fx_eth_init(void)
{
	/* auto-mdix enable */
	mxc_set_gpio_direction(MX31_PIN_ATA_CS0, 0 /* OUTPUT */);
	mxc_set_gpio_dataout(MX31_PIN_ATA_CS0, 1 /* HIGH */);

	set_irq_type(IOMUX_TO_IRQ(MX31_PIN_GPIO1_0), IRQT_FALLING);

	platform_device_register(&armadillo500fx_smc911x_device);
}

#if defined(CONFIG_MTD_ARMADILLO) || defined(CONFIG_MTD_ARMADILLO_MODULE)
#define MTD_PART(_name, _size, _offset, _mask_flags) \
	{                                            \
		.name		= _name,             \
		.size		= _size,             \
		.offset		= _offset,           \
		.mask_flags	= _mask_flags,       \
	}

static struct mtd_partition mtd_nor_partitions_8MB[] = {
	MTD_PART("nor.bootloader",   4* 32*1024, 0,
		 MTD_WRITEABLE /* force read-only */ ),
	MTD_PART("nor.kernel",      16*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.userland",    46*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.config",       1*128*1024, MTDPART_OFS_APPEND, 0),
};
static struct mtd_partition mtd_nor_partitions_16MB[] = {
	MTD_PART("nor.bootloader",   4* 32*1024, 0,
		 MTD_WRITEABLE /* force read-only */ ),
	MTD_PART("nor.kernel",      16*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.userland",   110*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.config",       1*128*1024, MTDPART_OFS_APPEND, 0),
};
static struct mtd_partition mtd_nor_partitions_32MB[] = {
	MTD_PART("nor.bootloader",   4* 32*1024, 0,
		 MTD_WRITEABLE /* force read-only */ ),
	MTD_PART("nor.kernel",      16*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.userland",   238*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.config",       1*128*1024, MTDPART_OFS_APPEND, 0),
};
static struct mtd_partition mtd_nor_partitions_64MB[] = {
	MTD_PART("nor.bootloader",   4* 32*1024, 0,
		 MTD_WRITEABLE /* force read-only */ ),
	MTD_PART("nor.kernel",      16*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.userland",   494*128*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nor.config",       1*128*1024, MTDPART_OFS_APPEND, 0),
};

static int
mtd_update_partitions(struct map_info *map,
		      struct flash_platform_data *plat)
{
	struct cfi_private *cfi = map->fldrv_priv;

	switch (cfi->cfiq->DevSize) {
	case 23: /* 8MB */
		plat->parts	= mtd_nor_partitions_8MB;
		plat->nr_parts	= ARRAY_SIZE(mtd_nor_partitions_8MB);
		break;
	case 24: /* 16MB */
		break;
	case 25: /* 32MB */
		plat->parts	= mtd_nor_partitions_32MB;
		plat->nr_parts	= ARRAY_SIZE(mtd_nor_partitions_32MB);
		break;
	case 26: /* 64MB */
		plat->parts	= mtd_nor_partitions_64MB;
		plat->nr_parts	= ARRAY_SIZE(mtd_nor_partitions_64MB);
		break;
	default:
		printk("Not support flash-size.\n");
		return -1;
	}

	return 0;
}

static struct armadillo_flash_private_data mtd_armadillo500fx_private = {
	.plat = {
		.map_name	= "cfi_probe",
		.width		= 2,
		.parts		= mtd_nor_partitions_16MB,
		.nr_parts	= ARRAY_SIZE(mtd_nor_partitions_16MB),
	},
	.update_partitions = mtd_update_partitions,
	.map_name = "armadillo5x0-nor",
};

static struct resource mtd_armadillo500fx_nor_resource = {
	.start		= CS0_BASE_ADDR,
	.end		= CS0_BASE_ADDR + SZ_64M - 1,
	.flags		= IORESOURCE_MEM,
};

static struct platform_device mtd_armadillo500fx_nor_device = {
	.name		= "armadillo-nor",
	.id		= 0,
	.dev		= {
		.platform_data = &mtd_armadillo500fx_private,
	},
	.num_resources	= 1,
	.resource	= &mtd_armadillo500fx_nor_resource,
};

static void
armadillo500fx_mtd_nor_init(void)
{
	platform_device_register(&mtd_armadillo500fx_nor_device);
}
#else
static inline void armadillo500fx_mtd_nor_init(void) { }
#endif /* CONFIG_MTD_ARMADILLO */

#if defined(CONFIG_FB_MXC) || defined(CONFIG_FB_MXC_MODULE)
static struct platform_device armadillo500fx_fb_device = {
	.name	= "mxc_sdc_fb",
	.id	= 0,
	.dev	= {
		.coherent_dma_mask	= 0xFFFFFFFF,
	},
};

static void
armadillo500fx_fb_init(void)
{
	platform_device_register(&armadillo500fx_fb_device);
}
#else
static void armadillo500fx_fb_init(void) { }
#endif /* CONFIG_FB_MXC */

#define GPIO_PORT(_name, _pin, _irq, _dir_ro) \
	{                            \
		.name	= (_name),     \
		.pin	= (_pin),      \
		.irq	= (_irq),      \
		.dir_ro	= (_dir_ro),   \
	}

struct armadillo5x0_gpio_port armadillo500fx_led_ports[] = {
	GPIO_PORT("status", MX31_PIN_SIMPD0, 0, 0),
};

static struct armadillo5x0_gpio_private armadillo500fx_led_priv = {
	.ports	= armadillo500fx_led_ports,
};

static struct platform_device armadillo500fx_led_device = {
	.name	= "armadillo5x0_led",
	.id	= 0,
	.dev = {
		.platform_data = &armadillo500fx_led_priv,
	},
};

static void
armadillo500fx_led_device_init(void)
{
	armadillo500fx_led_priv.nr_gpio = ARRAY_SIZE(armadillo500fx_led_ports);
	platform_device_register(&armadillo500fx_led_device);
}

#if defined(CONFIG_GPIO_ARMADILLO5X0) || \
	defined(CONFIG_GPIO_ARMADILLO5X0_MODULE)
struct armadillo5x0_gpio_port armadillo500fx_gpio_ports[] = {
	GPIO_PORT("gpio1_7", MX31_PIN_CAPTURE, MXC_INT_GPIO_P1(7), 1),
	GPIO_PORT("gpio1_8", MX31_PIN_COMPARE, MXC_INT_GPIO_P1(8), 1),
	GPIO_PORT("gpio2_0", MX31_PIN_SVEN0, MXC_INT_GPIO_P2(0), 1),
	GPIO_PORT("gpio2_1", MX31_PIN_STX0, MXC_INT_GPIO_P2(1), 1),
	GPIO_PORT("gpio2_2", MX31_PIN_SRX0, MXC_INT_GPIO_P2(2), 1),
	GPIO_PORT("gpio3_0", MX31_PIN_GPIO3_0, MXC_INT_GPIO_P3(0), 0),
	GPIO_PORT("gpio3_1", MX31_PIN_GPIO3_1, MXC_INT_GPIO_P3(1), 0),
	GPIO_PORT("gpio3_8", MX31_PIN_CSI_D8, MXC_INT_GPIO_P3(8), 0),
	GPIO_PORT("gpio3_9", MX31_PIN_CSI_D9, MXC_INT_GPIO_P3(9), 0),
	GPIO_PORT("gpio3_10", MX31_PIN_CSI_D10, MXC_INT_GPIO_P3(10), 0),
	GPIO_PORT("gpio3_11", MX31_PIN_CSI_D11, MXC_INT_GPIO_P3(11), 0),
	GPIO_PORT("gpio3_12", MX31_PIN_CSI_D12, MXC_INT_GPIO_P3(12), 0),
	GPIO_PORT("gpio3_13", MX31_PIN_CSI_D13, MXC_INT_GPIO_P3(13), 0),
	GPIO_PORT("gpio3_14", MX31_PIN_CSI_D14, MXC_INT_GPIO_P3(14), 0),
	GPIO_PORT("gpio3_15", MX31_PIN_CSI_D15, MXC_INT_GPIO_P3(15), 0),
	GPIO_PORT("gpio3_16", MX31_PIN_CSI_MCLK, MXC_INT_GPIO_P3(16), 0),
	GPIO_PORT("gpio3_17", MX31_PIN_CSI_VSYNC, MXC_INT_GPIO_P3(17), 0),
	GPIO_PORT("gpio3_18", MX31_PIN_CSI_HSYNC, MXC_INT_GPIO_P3(18), 0),
	GPIO_PORT("gpio3_19", MX31_PIN_CSI_PIXCLK, MXC_INT_GPIO_P3(19), 0),
};

static struct armadillo5x0_gpio_private armadillo500fx_gpio_priv = {
	.ports	= armadillo500fx_gpio_ports,
};

static struct platform_device armadillo500fx_gpio_device = {
	.name = "armadillo5x0_gpio",
	.id = 0,
	.dev = {
		.platform_data = &armadillo500fx_gpio_priv,
	},
};

static void
armadillo500fx_gpio_device_init(void)
{
	armadillo500fx_gpio_priv.nr_gpio = ARRAY_SIZE(armadillo500fx_gpio_ports);
	platform_device_register(&armadillo500fx_gpio_device);
}
#else
static inline void armadillo500fx_gpio_device_init(void) { }
#endif /* CONFIG_GPIO_ARMADILLO5X0 */

#if defined(CONFIG_KEYBOARD_MXC) || defined(CONFIG_KEYBOARD_MXC_MODULE)
static u16 armadillo500fx_keymap[24] = {
	KEY_HOME,	KEY_MENU,	KEY_BACK,
	KEY_F1,		KEY_UP,		KEY_F2,
	KEY_LEFT,	KEY_ENTER,	KEY_RIGHT,
	KEY_SEND,	KEY_DOWN,	KEY_END,
	KEY_1,		KEY_2,		KEY_3,
	KEY_4,		KEY_5,		KEY_6,
	KEY_7,		KEY_8,		KEY_9,
	KEY_F3,		KEY_0,		KEY_F4,
};

static struct resource armadillo500fx_keypad_resource = {
	.start	= MXC_INT_KPP,
	.end	= MXC_INT_KPP,
	.flags	= IORESOURCE_IRQ,
};

static struct keypad_data armadillo500fx_4_by_6_keypad = {
	.row_first = 3,
	.row_last = 6,
	.col_first = 2,
	.col_last = 7,
	.matrix = armadillo500fx_keymap,
};

static struct platform_device armadillo500fx_keypad_device = {
	.name = "mxc_keypad",
	.id = 0,
	.num_resources = 1,
	.resource = &armadillo500fx_keypad_resource,
	.dev = {
		.platform_data = &armadillo500fx_4_by_6_keypad,
	},
};

static void
armadillo500fx_keypad_init(void)
{
	platform_device_register(&armadillo500fx_keypad_device);
}
#else
static inline void armadillo500fx_keypad_init(void) { }
#endif /* CONFIG_KEYBOARD_MXC */

#ifdef CONFIG_BACKLIGHT_MXC
static struct platform_device armadillo500fx_backlight_device = {
	.name = "mxc-bl",
};

static void
armadillo500fx_backlight_init(void)
{
	platform_device_register(&armadillo500fx_backlight_device);        
}
#else
static inline void armadillo500fx_backlight_init(void) { }
#endif /* CONFIG_BACKLIGHT_MXC */

#if defined(CONFIG_SPI_MXC) || defined(CONFIG_SPI_MXC_MODULE)
static int
armadillo500fx_get_pendown_state(void)
{
	return !mxc_get_gpio_datain(MX31_PIN_SCLK0);
}

#define MAX_12BIT ((1 << 12) - 1)
static int armadillo500fx_filter_all_vals(u16 Rt, u16 x, u16 y)
{
	/* Check for AD timing discrepancy over spacer dots */
	if ((Rt && (!x || y == MAX_12BIT)) || (!Rt && (x || y != MAX_12BIT)))
		return -1;
	else
		return 0;
}

static unsigned armadillo500fx_vbatt_adjust(ssize_t v)
{
	return (v * 672) / 100;
}

#define ARMADILLO500FX_X_PLATE_OHMS (600)
static unsigned int armadillo500fx_calc_rt(u16 x, u16 y, u16 z1, u16 z2)
{
	unsigned int Rt;

	Rt = z2;
	Rt -= z1;
	Rt *= x;
	Rt /= z1;
	Rt *= ARMADILLO500FX_X_PLATE_OHMS;
	Rt = (Rt + 2047) >> 12;

	/* invert reading and also drop low pressure readings... they seem to
	 * correspond to inactaute x,y readings */
	return (Rt > 1500) ? 0 : 1500 - Rt;
}

static const struct ads7846_platform_data armadillo500fx_ts_info = {
	.model			= 7846,	/* TSC2046 */
	.vref_delay_usecs	= 100,	/* internal, no capacitor */
	.get_pendown_state	= armadillo500fx_get_pendown_state,
	.x_min			= 140,  /* min/max: depend on spi clock */
	.x_max			= 3920,
	.y_min			= 150,
	.y_max			= 3870,
	.pressure_min		= 0,
	.pressure_max		= 1500,
	.debounce_max           = 10,
	.debounce_tol           = 10,
	.settle_delay_usecs	= 1,
	.penirq_recheck_delay_usecs = 10,
	.filter_all_vals	= armadillo500fx_filter_all_vals,
	.zero_p_ignore_times	= 4,
	.vbatt_adjust		= armadillo500fx_vbatt_adjust,
	.calc_rt		= armadillo500fx_calc_rt,
};

static struct spi_board_info armadillo500fx_spi_board_info[] = {
	{
	.modalias	= "ads7846",
	.platform_data	= &armadillo500fx_ts_info,
	.irq		= IOMUX_TO_IRQ(MX31_PIN_SCLK0),
	.max_speed_hz	= 100000,
	.bus_num	= 3,
	.chip_select	= 1,
	},
};

static void
armadillo500fx_spi_init(void)
{
	/* TSC2046 - PENIRQ */
	mxc_set_gpio_direction(MX31_PIN_SCLK0, 1);

	/* Register all SPI slaves */
	spi_register_board_info(armadillo500fx_spi_board_info,
		ARRAY_SIZE(armadillo500fx_spi_board_info));
}
#else
static inline void armadillo500fx_spi_init(void) { }
#endif /* CONFIG_SPI_MXC */

#if defined(CONFIG_MXC_WSIM_PM) || defined(CONFIG_MXC_WSIM_PM_MODULE)
struct wsimpm_info armadillo500_wsimpm_info = {
	.ins_gpio		= MX31_PIN_DSR_DTE1,
	.ins_gpio_irq		= IOMUX_TO_IRQ(MX31_PIN_DSR_DTE1),
	.power_gpio		= MX31_PIN_ATA_DIOW,
	.power_gpio_state_on	= 0,
};

static struct platform_device armadillo500fx_wsim_device = {
	.name = "mxc_wsimpm",
	.dev = {
		.platform_data = &armadillo500_wsimpm_info,
	},
};

static void
armadillo500fx_wsim_init(void)
{
	/* W-SIM - power */
	mxc_set_gpio_direction(MX31_PIN_ATA_DIOW, 0);

	platform_device_register(&armadillo500fx_wsim_device);        
}
#else
static inline void armadillo500fx_wsim_init(void) { }
#endif /* CONFIG_MXC_WSIM_PM */

static struct i2c_board_info armadillo500fx_i2c2_info[] = {
	{
		.type = "rtc-s353xxa",
		.addr = 0x30,
	},
};

static void
armadillo500fx_i2c_device_init(void)
{
	i2c_register_board_info(1, armadillo500fx_i2c2_info,
				ARRAY_SIZE(armadillo500fx_i2c2_info));
}

static void __init
armadillo500fx_init(void)
{
	/* setup cpu_rev */
	if (readb(SYSTEM_SREV_REG) >= 0x20) {
		mxc_set_cpu_rev(0x31, CHIP_REV_2_0);
	} else {
		mxc_set_cpu_rev(0x31, CHIP_REV_1_2);
	}

	mxc_cpu_common_init();
	mxc_clocks_init();
	mxc_gpio_init();

	armadillo500fx_gpio_init();
	armadillo500fx_spi_init();
	armadillo500fx_eth_init();
	armadillo500fx_mtd_nor_init();
	armadillo500fx_fb_init();
	armadillo500fx_backlight_init();
	armadillo500fx_led_device_init();
	armadillo500fx_gpio_device_init();
	armadillo500fx_keypad_init();
	armadillo500fx_wsim_init();
	armadillo500fx_i2c_device_init();
}

#if defined(CONFIG_MACH_ARMADILLO500FX)
MACHINE_START(ARMADILLO500FX, "Armadillo-500 FX")
	/* Maintainer: Atmark Techno, Inc. */
	.phys_io	= AIPS1_BASE_ADDR,
	.io_pg_offst	= ((AIPS1_BASE_ADDR_VIRT) >> 18) & 0xfffc,
	.boot_params	= PHYS_OFFSET + 0x00000100,
	.map_io		= mxc_map_io,
	.init_irq	= mxc_init_irq,
	.timer		= &mxc_timer,
	.init_machine	= armadillo500fx_init,
MACHINE_END
#endif
