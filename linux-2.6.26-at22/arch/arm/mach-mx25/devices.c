/*
 * Copyright 2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#include <linux/platform_device.h>

#include <asm/arch/spba.h>
#include <asm/arch/sdma.h>

#include "sdma_script_code.h"
#include "devices.h"

void mxc_sdma_get_script_info(sdma_script_start_addrs * sdma_script_addr)
{
	sdma_script_addr->mxc_sdma_ap_2_ap_addr = ap_2_ap_ADDR;
	sdma_script_addr->mxc_sdma_ap_2_bp_addr = -1;
	sdma_script_addr->mxc_sdma_bp_2_ap_addr = -1;
	sdma_script_addr->mxc_sdma_loopback_on_dsp_side_addr = -1;
	sdma_script_addr->mxc_sdma_mcu_interrupt_only_addr = -1;

	sdma_script_addr->mxc_sdma_firi_2_per_addr = -1;
	sdma_script_addr->mxc_sdma_firi_2_mcu_addr = -1;
	sdma_script_addr->mxc_sdma_per_2_firi_addr = -1;
	sdma_script_addr->mxc_sdma_mcu_2_firi_addr = -1;

	sdma_script_addr->mxc_sdma_uart_2_per_addr = uart_2_per_ADDR;
	sdma_script_addr->mxc_sdma_uart_2_mcu_addr = uart_2_mcu_ADDR;
	sdma_script_addr->mxc_sdma_per_2_app_addr = per_2_app_ADDR;
	sdma_script_addr->mxc_sdma_mcu_2_app_addr = mcu_2_app_ADDR;

	sdma_script_addr->mxc_sdma_per_2_per_addr = -1;

	sdma_script_addr->mxc_sdma_uartsh_2_per_addr = uartsh_2_per_ADDR;
	sdma_script_addr->mxc_sdma_uartsh_2_mcu_addr = uartsh_2_mcu_ADDR;
	sdma_script_addr->mxc_sdma_per_2_shp_addr = per_2_shp_ADDR;
	sdma_script_addr->mxc_sdma_mcu_2_shp_addr = mcu_2_shp_ADDR;

	sdma_script_addr->mxc_sdma_ata_2_mcu_addr = ata_2_mcu_ADDR;
	sdma_script_addr->mxc_sdma_mcu_2_ata_addr = mcu_2_ata_ADDR;

	sdma_script_addr->mxc_sdma_app_2_per_addr = app_2_per_ADDR;
	sdma_script_addr->mxc_sdma_app_2_mcu_addr = app_2_mcu_ADDR;
	sdma_script_addr->mxc_sdma_shp_2_per_addr = shp_2_per_ADDR;
	sdma_script_addr->mxc_sdma_shp_2_mcu_addr = shp_2_mcu_ADDR;

	sdma_script_addr->mxc_sdma_mshc_2_mcu_addr = -1;
	sdma_script_addr->mxc_sdma_mcu_2_mshc_addr = -1;

	sdma_script_addr->mxc_sdma_spdif_2_mcu_addr = -1;
	sdma_script_addr->mxc_sdma_mcu_2_spdif_addr = -1;

	sdma_script_addr->mxc_sdma_asrc_2_mcu_addr = -1;

	sdma_script_addr->mxc_sdma_dptc_dvfs_addr = -1;
	sdma_script_addr->mxc_sdma_ext_mem_2_ipu_addr = ext_mem__ipu_ram_ADDR;
	sdma_script_addr->mxc_sdma_descrambler_addr = -1;

	sdma_script_addr->mxc_sdma_start_addr = (unsigned short *)sdma_code;
	sdma_script_addr->mxc_sdma_ram_code_size = RAM_CODE_SIZE;
	sdma_script_addr->mxc_sdma_ram_code_start_addr = RAM_CODE_START_ADDR;
}

static void mx25_nop_release(struct device *dev)
{
	/* Nothing */
}

/*!
 * Resource definition for the I2C1
 */
static struct resource mx25_i2c1_resources[] = {
	[0] = {
		.start = I2C_BASE_ADDR,
		.end = I2C_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = MXC_INT_I2C,
		.end = MXC_INT_I2C,
		.flags = IORESOURCE_IRQ,
	},
};

/*! Device Definition for MXC I2C1 */
struct platform_device mx25_i2c1_device = {
	.name = "mxc_i2c",
	.id = MX25_I2C1_ID,
	.dev = {
		.release = mx25_nop_release,
	},
	.num_resources = ARRAY_SIZE(mx25_i2c1_resources),
	.resource = mx25_i2c1_resources,
};

/*!
 * Resource definition for the I2C2
 */
static struct resource mx25_i2c2_resources[] = {
	[0] = {
		.start = I2C2_BASE_ADDR,
		.end = I2C2_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = MXC_INT_I2C2,
		.end = MXC_INT_I2C2,
		.flags = IORESOURCE_IRQ,
	},
};

/*! Device Definition for MXC I2C2 */
struct platform_device mx25_i2c2_device = {
	.name = "mxc_i2c",
	.id = MX25_I2C2_ID,
	.dev = {
		.release = mx25_nop_release,
	},
	.num_resources = ARRAY_SIZE(mx25_i2c2_resources),
	.resource = mx25_i2c2_resources,
};

/*!
 * Resource definition for the I2C3
 */
static struct resource mx25_i2c3_resources[] = {
	[0] = {
		.start = I2C3_BASE_ADDR,
		.end = I2C3_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = MXC_INT_I2C3,
		.end = MXC_INT_I2C3,
		.flags = IORESOURCE_IRQ,
	},
};

/*! Device Definition for MXC I2C3 */
struct platform_device mx25_i2c3_device = {
	.name = "mxc_i2c",
	.id = MX25_I2C3_ID,
	.dev = {
		.release = mx25_nop_release,
	},
	.num_resources = ARRAY_SIZE(mx25_i2c3_resources),
	.resource = mx25_i2c3_resources,
};

static struct resource mx25_spi1_resources[] = {
	[0] = {
		.start = CSPI1_BASE_ADDR,
		.end = CSPI1_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = MXC_INT_CSPI1,
		.end = MXC_INT_CSPI1,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mx25_spi1_device = {
	.name = "mxc_spi",
	.id = 0,
	.dev = {
		.release = mx25_nop_release,
	},
	.num_resources = ARRAY_SIZE(mx25_spi1_resources),
	.resource = mx25_spi1_resources,
};

static struct resource mx25_spi2_resources[] = {
	[0] = {
		.start = CSPI2_BASE_ADDR,
		.end = CSPI2_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = MXC_INT_CSPI2,
		.end = MXC_INT_CSPI2,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mx25_spi2_device = {
	.name = "mxc_spi",
	.id = 1,
	.dev = {
		.release = mx25_nop_release,
	},
	.num_resources = ARRAY_SIZE(mx25_spi2_resources),
	.resource = mx25_spi2_resources,
};

static struct resource mx25_spi3_resources[] = {
	[0] = {
		.start = CSPI3_BASE_ADDR,
		.end = CSPI3_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = MXC_INT_CSPI3,
		.end = MXC_INT_CSPI3,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mx25_spi3_device = {
	.name = "mxc_spi",
	.id = 2,
	.dev = {
		.release = mx25_nop_release,
	},
	.num_resources = ARRAY_SIZE(mx25_spi3_resources),
	.resource = mx25_spi3_resources,
};

struct mxc_gpio_port mxc_gpio_ports[GPIO_PORT_NUM] = {
	{
		.num = 0,
		.base = IO_ADDRESS(GPIO1_BASE_ADDR),
		.irq = MXC_INT_GPIO1,
		.virtual_irq_start = MXC_GPIO_INT_BASE,
	},
	{
		.num = 1,
		.base = IO_ADDRESS(GPIO2_BASE_ADDR),
		.irq = MXC_INT_GPIO2,
		.virtual_irq_start = MXC_GPIO_INT_BASE + GPIO_NUM_PIN,
	},
	{
		.num = 2,
		.base = IO_ADDRESS(GPIO3_BASE_ADDR),
		.irq = MXC_INT_GPIO3,
		.virtual_irq_start = MXC_GPIO_INT_BASE + GPIO_NUM_PIN * 2,
	},
	{
		.num = 3,
		.base = IO_ADDRESS(GPIO4_BASE_ADDR),
		.irq = MXC_INT_GPIO4,
		.virtual_irq_start = MXC_GPIO_INT_BASE + GPIO_NUM_PIN * 3,
	},
};

struct platform_device mx25_fb_device = {
	.name = "mxc_sdc_fb",
	.id = 0,
	.dev = {
		.release = mx25_nop_release,
		.coherent_dma_mask = 0xFFFFFFFF,
	},
};

static struct resource mx25_adc_resources[] = {
	[0] = {
		.start = MXC_INT_TSC,
		.end = MXC_INT_TSC,
		.flags = IORESOURCE_IRQ,
	},
	[1] = {
		.start = TSC_BASE_ADDR,
		.end = TSC_BASE_ADDR + PAGE_SIZE,
		.flags = IORESOURCE_MEM,
	}
};

struct platform_device mx25_adc_device = {
	.name = "imx_adc",
	.id = 0,
	.num_resources = ARRAY_SIZE(mx25_adc_resources),
	.resource = mx25_adc_resources,
	.dev = {
		.release = NULL,
	},
};

struct platform_device mx25_adc_ts_device = {
	.name = "imx_adc_ts",
	.id = 0,
};

extern unsigned int sdhc_get_card_det_status(struct device *dev);

#if defined(CONFIG_MMC_MXC_SELECT1)
/*!
 * Resource definition for the SDHC1
 */
static struct resource mx25_sdhc1_resources[] = {
	[0] = {
		.start = MMC_SDHC1_BASE_ADDR,
		.end = MMC_SDHC1_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = MXC_INT_SDHC1,
		.end = MXC_INT_SDHC1,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		/* The irq for card detect is necessary to set in the
		 * platform */
		.flags = IORESOURCE_IRQ,
	},
};

/*! Device Definition for MXC SDHC1 */
struct platform_device mx25_sdhc1_device = {
	.name = "mxsdhci",
	.id = 0,
	.dev = {
		.release = mx25_nop_release,
	},
	.num_resources = ARRAY_SIZE(mx25_sdhc1_resources),
	.resource = mx25_sdhc1_resources,
};
#endif /* defined(CONFIG_MMC_MXC_SELECT1) */

#if defined(CONFIG_MMC_MXC_SELECT2)
/*!
 * Resource definition for the SDHC2
 */
static struct resource mx25_sdhc2_resources[] = {
	[0] = {
		.start = MMC_SDHC2_BASE_ADDR,
		.end = MMC_SDHC2_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = MXC_INT_SDHC2,
		.end = MXC_INT_SDHC2,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		/* The irq for card detect is necessary to set in the
		 * platform */
		.flags = IORESOURCE_IRQ,
	},
};

/*! Device Definition for MXC SDHC2 */
struct platform_device mx25_sdhc2_device = {
	.name = "mxsdhci",
	.id = 1,
	.dev = {
		.release = mx25_nop_release,
	},
	.num_resources = ARRAY_SIZE(mx25_sdhc2_resources),
	.resource = mx25_sdhc2_resources,
};
#endif /* defined(CONFIG_MMC_MXC_SELECT2) */

static struct resource mx25_fec_resources[] = {
	{
		.start = FEC_BASE_ADDR,
		.end   = FEC_BASE_ADDR + 0xfff,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_FEC,
		.end   = MXC_INT_FEC,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mx25_fec_device = {
	.name = "fec",
	.id = 0,
	.num_resources = ARRAY_SIZE(mx25_fec_resources),
	.resource = mx25_fec_resources,
};

static struct resource mx25_pwm1_resources[] = {
	{
		.start = PWM1_BASE_ADDR,
		.end = PWM1_BASE_ADDR + 0x14,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device mx25_pwm1_device = {
	.name = "mxc_pwm",
	.id = 0,
	.num_resources = ARRAY_SIZE(mx25_pwm1_resources),
	.resource = mx25_pwm1_resources,
	.dev = {
		.release = mx25_nop_release,
	},
};

static struct resource mx25_pwm2_resources[] = {
	{
		.start = PWM2_BASE_ADDR,
		.end = PWM2_BASE_ADDR + 0x14,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device mx25_pwm2_device = {
	.name = "mxc_pwm",
	.id = 1,
	.num_resources = ARRAY_SIZE(mx25_pwm2_resources),
	.resource = mx25_pwm2_resources,
	.dev = {
		.release = mx25_nop_release,
	},
};

static struct resource mx25_pwm3_resources[] = {
	{
		.start = PWM3_BASE_ADDR,
		.end = PWM3_BASE_ADDR + 0x14,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device mx25_pwm3_device = {
	.name = "mxc_pwm",
	.id = 2,
	.num_resources = ARRAY_SIZE(mx25_pwm3_resources),
	.resource = mx25_pwm3_resources,
	.dev = {
		.release = mx25_nop_release,
	},
};

static struct resource mx25_pwm4_resources[] = {
	{
		.start = PWM4_BASE_ADDR,
		.end = PWM4_BASE_ADDR + 0x14,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device mx25_pwm4_device = {
	.name = "mxc_pwm",
	.id = 3,
	.num_resources = ARRAY_SIZE(mx25_pwm4_resources),
	.resource = mx25_pwm4_resources,
	.dev = {
		.release = mx25_nop_release,
	},
};

struct platform_device mx25_pwm_backlight_device = {
	.name = "pwm-backlight",
	.id = -1,
	.dev = {
		.release = mx25_nop_release,
	},
};

static struct resource mx25_mtd_nor_resource = {
	.start		= CS0_BASE_ADDR,
	.end		= CS0_BASE_ADDR + SZ_64M - 1,
	.flags		= IORESOURCE_MEM,
};

struct platform_device mx25_mtd_nor_device = {
	.name		= "armadillo-nor",
	.id		= 0,
	.num_resources	= 1,
	.resource	= &mx25_mtd_nor_resource,
};

struct platform_device mx25_gpio_led_device = {
	.name = "leds-gpio",
	.id   = 0,
};

struct platform_device mx25_a2x0_compat_led_device = {
        .name = "armadillo2x0-led",
};

struct platform_device mx25_gpio_key_device = {
	.name = "gpio-keys",
	.id   = 0,
};

struct platform_device mx25_a2x0_compat_gpio_device = {
        .name = "armadillo2x0-gpio",
};

struct platform_device mx25_gpio_w1_device = {
	.name = "w1-gpio",
	.id   = -1,
};

struct platform_device mx25_mxc_w1_device = {
	.name = "mxc_w1",
	.dev = {
		.release = mx25_nop_release,
	},
	.id = 0,
};

struct platform_device armadillo440_wm8978_audio_device = {
	.name = "armadillo440-wm8978-audio",
	.id = 0,
};

static struct resource mx25_flexcan1_resources[] = {
	{
		.start = CAN1_BASE_ADDR,
		.end = CAN1_BASE_ADDR + 0x97F,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_CAN1,
		.end = MXC_INT_CAN1,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mx25_flexcan1_device = {
	.name = "FlexCAN",
	.id = 0,
	.dev = {
		.release = mx25_nop_release,
	},
	.num_resources = ARRAY_SIZE(mx25_flexcan1_resources),
	.resource = mx25_flexcan1_resources,
};

static struct resource mx25_flexcan2_resources[] = {
	{
		.start = CAN2_BASE_ADDR,
		.end = CAN2_BASE_ADDR + 0x97F,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = MXC_INT_CAN2,
		.end = MXC_INT_CAN2,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mx25_flexcan2_device = {
	.name = "FlexCAN",
	.id = 1,
	.dev = {
		.release = mx25_nop_release,
	},
	.num_resources = ARRAY_SIZE(mx25_flexcan2_resources),
	.resource = mx25_flexcan2_resources,
};

static struct resource mx25_keypad_resources[] = {
	[0] = {
		.start = MXC_INT_KPP,
		.end = MXC_INT_KPP,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device mx25_keypad_device = {
	.name = "mxc_keypad",
	.id = 0,
	.num_resources = ARRAY_SIZE(mx25_keypad_resources),
	.resource = mx25_keypad_resources,
	.dev = {
		.release = mx25_nop_release,
	},
};
