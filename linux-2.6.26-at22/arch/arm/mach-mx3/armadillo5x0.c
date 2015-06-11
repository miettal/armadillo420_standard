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
extern void armadillo5x0_gpio_init(void);

static struct resource armadillo5x0_smc911x_resources[] = {
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
	.irq_type	= SMSC911X_IRQ_TYPE_PUSH_PULL,
	.flags		= SMSC911X_USE_32BIT,
};
static struct platform_device armadillo5x0_smc911x_device = {
	.name		= "smsc911x",
	.id		= 0,
	.dev		= {
		.platform_data = &smsc911x_config,
	},
	.num_resources	= ARRAY_SIZE(armadillo5x0_smc911x_resources),  
	.resource	= armadillo5x0_smc911x_resources,
};

static void
armadillo5x0_eth_init(void)
{
	platform_device_register(&armadillo5x0_smc911x_device);
}

#if defined(CONFIG_PCMCIA_ARMADILLO5X0) || \
    defined(CONFIG_PCMCIA_ARMADILLO5X0_MODULE)
#include <asm/arch/pcmcia.h>
static struct resource armadillo5x0_pcmcia_resources[] = {
	[0] = {
		.name	= "pcmcia",
		.start	= PCMCIA_MEM_BASE_ADDR,
		.end	= PCMCIA_MEM_BASE_ADDR + 6 * SZ_1K - 1,
		.flags	= IORESOURCE_MEM | IORESOURCE_MEM_16BIT,
	},
	[1] = {
		.name	= "pcmcia:attr",
		.start	= PCMCIA_MEM_BASE_ADDR,
		.end	= PCMCIA_MEM_BASE_ADDR + 2 * SZ_1K - 1,
		.flags	= IORESOURCE_MEM | IORESOURCE_MEM_16BIT,
	},
	[2] = {
		.name	= "pcmcia:mem",
		.start	= PCMCIA_MEM_BASE_ADDR + 2 * SZ_1K,
		.end	= PCMCIA_MEM_BASE_ADDR + 4 * SZ_1K - 1,
		.flags	= IORESOURCE_MEM | IORESOURCE_MEM_16BIT,
	},
	[3] = {
		.name	= "pcmcia:io",
		.start	= PCMCIA_MEM_BASE_ADDR + 4 * SZ_1K,
		.end	= PCMCIA_MEM_BASE_ADDR + 6 * SZ_1K - 1,
		.flags	= IORESOURCE_MEM | IORESOURCE_MEM_16BIT,
	},
	[4] = {
		.start	= MXC_INT_PCMCIA,
		.end	= MXC_INT_PCMCIA,
		.flags	= IORESOURCE_IRQ,
	},
};

extern void gpio_pcmcia_power_on(void);
extern void gpio_pcmcia_power_off(void);
static struct pcmcia_platform_data armadillo5x0_pcmcia_platform_data = {
	.card_power_on	= gpio_pcmcia_power_on,
	.card_power_off	= gpio_pcmcia_power_off,
};

static struct platform_device armadillo5x0_pcmcia_device = {
	.name		= "armadillo5x0_pcmcia",
	.id		= 0,
	.dev		= {
		.platform_data	= &armadillo5x0_pcmcia_platform_data,
	},
	.num_resources	= ARRAY_SIZE(armadillo5x0_pcmcia_resources),
	.resource	= armadillo5x0_pcmcia_resources,
};

static inline void armadillo5x0_pcmcia_init(void)
{
	platform_device_register(&armadillo5x0_pcmcia_device);
}
#endif

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

static struct armadillo_flash_private_data mtd_armadillo5x0_private = {
	.plat = {
		.map_name	= "cfi_probe",
		.width		= 2,
		.parts		= mtd_nor_partitions_16MB,
		.nr_parts	= ARRAY_SIZE(mtd_nor_partitions_16MB),
	},
	.update_partitions = mtd_update_partitions,
	.map_name = "armadillo5x0-nor",
};

static struct resource mtd_armadillo5x0_nor_resource = {
	.start		= CS0_BASE_ADDR,
	.end		= CS0_BASE_ADDR + SZ_64M - 1,
	.flags		= IORESOURCE_MEM,
};

static struct platform_device mtd_armadillo5x0_nor_device = {
	.name		= "armadillo-nor",
	.id		= 0,
	.dev		= {
		.platform_data = &mtd_armadillo5x0_private,
	},
	.num_resources	= 1,
	.resource	= &mtd_armadillo5x0_nor_resource,
};

static void
armadillo5x0_mtd_nor_init(void)
{
	platform_device_register(&mtd_armadillo5x0_nor_device);
}
#endif /* CONFIG_MTD_ARMADILLO5X0 */

#if defined(CONFIG_MTD_NAND_MXC_V1) || defined(CONFIG_MTD_NAND_MXC_V1_MODULE)
static struct mtd_partition mtd_armadillo5x0_nand_partitions[] = {
	MTD_PART("nand.ipl",           1* 128*1024, 0, 0),
	MTD_PART("nand.kernel",        4*1024*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nand.userland",     22*1024*1024, MTDPART_OFS_APPEND, 0),
	MTD_PART("nand.free",     MTDPART_SIZ_FULL, MTDPART_OFS_APPEND, 0),
};

static struct flash_platform_data mtd_armadillo5x0_nand_data = {
	.parts = mtd_armadillo5x0_nand_partitions,
	.nr_parts = ARRAY_SIZE(mtd_armadillo5x0_nand_partitions),
	.width = 1,
};

static struct platform_device mtd_armadillo5x0_nand_device = {
	.name = "mxc_nand_flash",
	.id = 0,
	.dev = {
		.platform_data = &mtd_armadillo5x0_nand_data,
	},
};

static void
armadillo5x0_mtd_nand_init(void)
{
	if (__raw_readl(MXC_CCM_RCSR) & MXC_CCM_RCSR_NF16B) {
		mtd_armadillo5x0_nand_data.width = 2;
	}
	platform_device_register(&mtd_armadillo5x0_nand_device);
}
#endif /* CONFIG_MTD_NAND_MXC_V1 */

static struct platform_device armadillo5x0_fb_device = {
	.name	= "mxc_sdc_fb",
	.id	= 0,
	.dev	= {
		.coherent_dma_mask	= 0xFFFFFFFF,
	},
};

static void
armadillo5x0_fb_init(void)
{
	platform_device_register(&armadillo5x0_fb_device);
}

#define GPIO_PORT(_name, _pin, _irq) \
	{                            \
		.name = (_name),     \
		.pin  = (_pin),      \
		.irq  = (_irq),      \
	}

struct armadillo5x0_gpio_port armadillo5x0_led_ports[] = {
	GPIO_PORT("led1", MX31_PIN_SVEN0, 0),
	GPIO_PORT("led2", MX31_PIN_STX0, 0),
	GPIO_PORT("led3", MX31_PIN_SRX0, 0),
	GPIO_PORT("led4", MX31_PIN_SIMPD0, 0),
	GPIO_PORT("led5", MX31_PIN_BATT_LINE, 0),
};

static struct armadillo5x0_gpio_private armadillo5x0_led_priv = {
	.ports	= armadillo5x0_led_ports,
};

static struct platform_device armadillo5x0_led_device = {
	.name	= "armadillo5x0_led",
	.id	= 0,
	.dev = {
		.platform_data = &armadillo5x0_led_priv,
	},
};

static void
armadillo5x0_led_device_init(void)
{
	armadillo5x0_led_priv.nr_gpio = ARRAY_SIZE(armadillo5x0_led_ports);
	platform_device_register(&armadillo5x0_led_device);
}

struct armadillo5x0_gpio_port armadillo5x0_gpio_ports[] = {
	GPIO_PORT("gpio0", MX31_PIN_KEY_ROW4, MXC_INT_GPIO_P2(18)),
	GPIO_PORT("gpio1", MX31_PIN_KEY_ROW5, MXC_INT_GPIO_P2(19)),
	GPIO_PORT("gpio2", MX31_PIN_KEY_ROW6, MXC_INT_GPIO_P2(20)),
	GPIO_PORT("gpio3", MX31_PIN_KEY_ROW7, MXC_INT_GPIO_P2(21)),
	GPIO_PORT("gpio4", MX31_PIN_KEY_COL4, MXC_INT_GPIO_P2(22)),
	GPIO_PORT("gpio5", MX31_PIN_KEY_COL5, MXC_INT_GPIO_P2(23)),
	GPIO_PORT("gpio6", MX31_PIN_KEY_COL6, MXC_INT_GPIO_P2(24)),
	GPIO_PORT("gpio7", MX31_PIN_KEY_COL7, MXC_INT_GPIO_P2(25)),
};

static struct armadillo5x0_gpio_private armadillo5x0_gpio_priv = {
	.ports	= armadillo5x0_gpio_ports,
};

static struct platform_device armadillo5x0_gpio_device = {
	.name = "armadillo5x0_gpio",
	.id = 0,
	.dev = {
		.platform_data = &armadillo5x0_gpio_priv,
	},
};

static void
armadillo5x0_gpio_device_init(void)
{
	armadillo5x0_gpio_priv.nr_gpio = ARRAY_SIZE(armadillo5x0_gpio_ports);
	platform_device_register(&armadillo5x0_gpio_device);
}

struct armadillo5x0_gpio_port armadillo5x0_tactsw_ports[] = {
	GPIO_PORT("tactsw1", MX31_PIN_SCLK0, MXC_INT_GPIO_P3(2)),
	GPIO_PORT("tactsw2", MX31_PIN_SRST0, MXC_INT_GPIO_P3(3)),
};

static struct armadillo5x0_gpio_private armadillo5x0_tactsw_priv = {
	.ports	= armadillo5x0_tactsw_ports,
};

static struct platform_device armadillo5x0_tactsw_device = {
	.name = "armadillo5x0_tactsw",
	.id = 0,
	.dev = {
		.platform_data = &armadillo5x0_tactsw_priv,
	},
};

static void
armadillo5x0_tactsw_device_init(void)
{
	armadillo5x0_tactsw_priv.nr_gpio =
		ARRAY_SIZE(armadillo5x0_tactsw_ports);
	platform_device_register(&armadillo5x0_tactsw_device);
}

static struct i2c_board_info armadillo5x0_i2c2_info[] = {
	{
		.type = "rtc-s353xxa",
		.addr = 0x30,
	},
};

static void
armadillo5x0_i2c_device_init(void)
{
	i2c_register_board_info(1, armadillo5x0_i2c2_info,
				ARRAY_SIZE(armadillo5x0_i2c2_info));
}

static void __init
armadillo5x0_init(void)
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

	armadillo5x0_gpio_init();
	armadillo5x0_eth_init();
	armadillo5x0_pcmcia_init();
	armadillo5x0_mtd_nor_init();
	armadillo5x0_mtd_nand_init();
	armadillo5x0_fb_init();
	armadillo5x0_led_device_init();
	armadillo5x0_gpio_device_init();
	armadillo5x0_tactsw_device_init();
	armadillo5x0_i2c_device_init();
}

#if defined(CONFIG_MACH_ARMADILLO500)
MACHINE_START(ARMADILLO5X0, "Armadillo-500")
	/* Maintainer: Atmark Techno, Inc. */
	.phys_io	= AIPS1_BASE_ADDR,
	.io_pg_offst	= ((AIPS1_BASE_ADDR_VIRT) >> 18) & 0xfffc,
	.boot_params	= PHYS_OFFSET + 0x00000100,
	.map_io		= mxc_map_io,
	.init_irq	= mxc_init_irq,
	.timer		= &mxc_timer,
	.init_machine	= armadillo5x0_init,
MACHINE_END
#endif
