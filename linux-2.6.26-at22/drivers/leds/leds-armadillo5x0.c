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

#include <linux/platform_device.h>
#include <linux/leds.h>

#include <asm/arch/gpio.h>

struct armadillo5x0_led_info {
	struct led_classdev cdev;
	struct armadillo5x0_gpio_port *port;
};

static void
armadillo5x0_led_set(struct led_classdev *cdev,
		     enum led_brightness value)
{
	struct armadillo5x0_led_info *info = dev_get_drvdata(cdev->dev);

	mxc_set_gpio_direction(info->port->pin, 0/*OUTPUT*/);
	if (value)
		mxc_set_gpio_dataout(info->port->pin, 1/*HIGH*/);
	else
		mxc_set_gpio_dataout(info->port->pin, 0/*LOW*/);
}

static int
armadillo5x0_led_probe(struct platform_device *pdev)
{
	struct armadillo5x0_gpio_private *priv = pdev->dev.platform_data;
	struct armadillo5x0_led_info *info;
	int ret;
	int i;

	info = kzalloc(sizeof(struct armadillo5x0_led_info) * priv->nr_gpio,
		       GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	for (i=0; i<priv->nr_gpio; i++) {
		info[i].port = &priv->ports[i];
		info[i].cdev.name = priv->ports[i].name;
		info[i].cdev.brightness_set = armadillo5x0_led_set;
		info[i].cdev.default_trigger = "timer";

		ret = led_classdev_register(&pdev->dev, &info[i].cdev);
		if (ret < 0) {
			while (i-- >= 0)
				led_classdev_unregister(&info[i].cdev);

			kfree(info);
			return ret;
		}
	}

	platform_set_drvdata(pdev, info);

	return 0;
}

static int
armadillo5x0_led_remove(struct platform_device *pdev)
{
	struct armadillo5x0_gpio_private *priv = pdev->dev.platform_data;
	struct armadillo5x0_led_info *info = platform_get_drvdata(pdev);
	int i;

	platform_set_drvdata(pdev, NULL);

	for (i=0; i<priv->nr_gpio; i++)
		led_classdev_unregister(&info[i].cdev);

	kfree(info);

	return 0;
}

static struct platform_driver armadillo5x0_led_driver = {
	.probe	= armadillo5x0_led_probe,
	.remove	= __devexit_p(armadillo5x0_led_remove),
	.driver	= {
		.name	= "armadillo5x0_led",
	},
};

static int __init
armadillo5x0_led_init(void)
{
	return platform_driver_register(&armadillo5x0_led_driver);
}

static void __exit
armadillo5x0_led_exit(void)
{
	platform_driver_unregister(&armadillo5x0_led_driver);
}

module_init(armadillo5x0_led_init);
module_exit(armadillo5x0_led_exit);

MODULE_AUTHOR("Atmark Techno, Inc.");
MODULE_DESCRIPTION("Armadillo-5x0 LED driver");
MODULE_LICENSE("GPL v2");
