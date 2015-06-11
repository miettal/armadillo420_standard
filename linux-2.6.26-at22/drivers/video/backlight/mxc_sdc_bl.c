/*
 * Backlight Driver for Freescale MXC/iMX boards
 *
 * Copyright (c) 2008 Atmark Techno, Inc
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/backlight.h>

#include <asm/arch/ipu.h>

#define MXCBL_BRIGHTNESS_MAX		(255)
#define MXCBL_BRIGHTNESS_DEFAULT	(150)

static int mxcbl_suspended;
static int current_brightness;
static struct backlight_device *mxcbl_device;

static void mxcbl_set_brightness(struct backlight_device *bd)
{
	int brightness = bd->props.brightness;

	if (bd->props.power != FB_BLANK_UNBLANK)
		brightness = 0;
	if (bd->props.fb_blank != FB_BLANK_UNBLANK)
		brightness = 0;
	if (mxcbl_suspended)
		brightness = 0;

	ipu_sdc_set_brightness(brightness);
	current_brightness = brightness;
}

#ifdef CONFIG_PM
static int mxcbl_suspend(struct platform_device *dev, pm_message_t state)
{
	mxcbl_suspended = 1;
	mxcbl_set_brightness(mxcbl_device);
	return 0;
}

static int mxcbl_resume(struct platform_device *dev)
{
	mxcbl_suspended = 0;
	mxcbl_set_brightness(mxcbl_device);
	return 0;
}
#else
#define mxcbl_suspend	NULL
#define mxcbl_resume	NULL
#endif

static int mxcbl_update_brightness(struct backlight_device *bd)
{
	mxcbl_set_brightness(bd);
	return 0;
}

static int mxcbl_get_brightness(struct backlight_device *bd)
{
	return current_brightness;
}

static struct backlight_ops mxcbl_ops = {
	.get_brightness = mxcbl_get_brightness,
	.update_status  = mxcbl_update_brightness,
};

static int __init mxcbl_probe(struct platform_device *pdev)
{
	mxcbl_device = backlight_device_register("mxc-bl", &pdev->dev, NULL,
			&mxcbl_ops);
	if (IS_ERR(mxcbl_device))
		return PTR_ERR(mxcbl_device);

	mxcbl_device->props.max_brightness = MXCBL_BRIGHTNESS_MAX;
	mxcbl_device->props.brightness = MXCBL_BRIGHTNESS_DEFAULT;
	mxcbl_set_brightness(mxcbl_device);

	printk("MXC/iMX Backlight Driver\n");

	return 0;
}

static int mxcbl_remove(struct platform_device *dev)
{
	backlight_device_unregister(mxcbl_device);

	return 0;
}

static struct platform_driver mxcbl_driver = {
	.probe		= mxcbl_probe,
	.remove		= mxcbl_remove,
	.suspend	= mxcbl_suspend,
	.resume		= mxcbl_resume,
	.driver		= {
		.name	= "mxc-bl",
	},
};

static int __init mxcbl_init(void)
{
	return platform_driver_register(&mxcbl_driver);
}

static void __exit mxcbl_exit(void)
{
 	platform_driver_unregister(&mxcbl_driver);
}

module_init(mxcbl_init);
module_exit(mxcbl_exit);

MODULE_AUTHOR("Atmark Techno, Inc");
MODULE_DESCRIPTION("Freescale MXC/iMX Backlight Driver");
MODULE_LICENSE("GPL");

