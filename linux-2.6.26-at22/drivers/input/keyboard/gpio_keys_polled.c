/*
 *  Driver for buttons on GPIO lines not capable of generating interrupts
 *
 *  Copyright (C) 2014 Atmark Techno, Inc. All Rights Reserved.
 *
 *  This file was backported from mainline(linux-3.17.4)
 *	Copyright (C) 2007-2010 Gabor Juhos <juhosg@openwrt.org>
 *	Copyright (C) 2010 Nuno Goncalves <nunojpg@gmail.com>
 *
 *  also was based on: /drivers/input/misc/cobalt_btns.c
 *	Copyright (C) 2007 Yoichi Yuasa <yoichi_yuasa@tripeaks.co.jp>
 *
 *  also was based on: /drivers/input/keyboard/gpio_keys.c
 *	Copyright 2005 Phil Blundell
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>

#define DRV_NAME	"gpio-keys-polled"

struct gpio_keys_button_data {
	int last_state;
	int count;
	int threshold;
	int can_sleep;
};

struct gpio_keys_polled_dev {
	struct input_polled_dev *poll_dev;
	struct device *dev;
	const struct gpio_keys_platform_data *pdata;
	struct gpio_keys_button_data data[0];
};

static void gpio_keys_polled_check_state(struct input_dev *input,
					 struct gpio_keys_button *button,
					 struct gpio_keys_button_data *bdata)
{
	int state;

	if (bdata->can_sleep)
		state = !!gpio_get_value_cansleep(button->gpio);
	else
		state = !!gpio_get_value(button->gpio);

	if (state != bdata->last_state) {
		unsigned int type = button->type ?: EV_KEY;

		input_event(input, type, button->code,
			    !!(state ^ button->active_low));
		input_sync(input);
		bdata->count = 0;
		bdata->last_state = state;
	}
}

static void gpio_keys_polled_poll(struct input_polled_dev *dev)
{
	struct gpio_keys_polled_dev *bdev = dev->private;
	const struct gpio_keys_platform_data *pdata = bdev->pdata;
	struct input_dev *input = dev->input;
	int i;

	for (i = 0; i < pdata->nbuttons; i++) {
		struct gpio_keys_button_data *bdata = &bdev->data[i];

		if (bdata->count < bdata->threshold)
			bdata->count++;
		else
			gpio_keys_polled_check_state(input, &pdata->buttons[i],
						     bdata);
	}
}

static int gpio_keys_polled_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const struct gpio_keys_platform_data *pdata = pdev->dev.platform_data;
	struct gpio_keys_polled_dev *bdev;
	struct input_polled_dev *poll_dev;
	struct input_dev *input;
	size_t size;
	int error;
	int i;

	if (!pdata->poll_interval) {
		dev_err(dev, "missing poll_interval value\n");
		return -EINVAL;
	}

	size = sizeof(struct gpio_keys_polled_dev) +
			pdata->nbuttons * sizeof(struct gpio_keys_button_data);
	bdev = kzalloc(size, GFP_KERNEL);
	if (!bdev) {
		dev_err(dev, "no memory for private data\n");
		return -ENOMEM;
	}

	poll_dev = input_allocate_polled_device();
	if (!poll_dev) {
		dev_err(dev, "no memory for polled device\n");
		return -ENOMEM;
	}

	poll_dev->private = bdev;
	poll_dev->poll = gpio_keys_polled_poll;
	poll_dev->poll_interval = pdata->poll_interval;

	input = poll_dev->input;

	input->name = pdev->name;
	input->phys = DRV_NAME"/input0";

	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;

	__set_bit(EV_KEY, input->evbit);

	for (i = 0; i < pdata->nbuttons; i++) {
		struct gpio_keys_button *button = &pdata->buttons[i];
		struct gpio_keys_button_data *bdata = &bdev->data[i];
		unsigned int gpio = button->gpio;
		unsigned int type = button->type ?: EV_KEY;

		if (button->wakeup) {
			dev_err(dev, DRV_NAME " does not support wakeup\n");
			return -EINVAL;
		}

		error = gpio_request(gpio, button->desc ? : DRV_NAME);
		if (error) {
			dev_err(dev, "unable to claim gpio %u, err=%d\n",
				gpio, error);
			return error;
		}

		bdata->can_sleep = gpio_cansleep(gpio);
		bdata->last_state = -1;
		bdata->threshold = DIV_ROUND_UP(button->debounce_interval,
						pdata->poll_interval);

		input_set_capability(input, type, button->code);
	}

	bdev->poll_dev = poll_dev;
	bdev->dev = dev;
	bdev->pdata = pdata;
	platform_set_drvdata(pdev, bdev);

	error = input_register_polled_device(poll_dev);
	if (error) {
		dev_err(dev, "unable to register polled device, err=%d\n",
			error);
		return error;
	}

	/* report initial state of the buttons */
	for (i = 0; i < pdata->nbuttons; i++)
		gpio_keys_polled_check_state(input, &pdata->buttons[i],
					     &bdev->data[i]);

	return 0;
}

static int __devexit gpio_keys_polled_remove(struct platform_device *pdev)
{
	struct gpio_keys_platform_data *pdata = pdev->dev.platform_data;
	struct gpio_keys_polled_dev *bdev = platform_get_drvdata(pdev);
	struct input_polled_dev *poll_dev = bdev->poll_dev;
	int i;

	input_unregister_polled_device(poll_dev);
	input_free_polled_device(poll_dev);

	for (i = 0; i < pdata->nbuttons; i++) {
		gpio_free(pdata->buttons[i].gpio);
	}

	kfree(bdev);

	return 0;
}

static struct platform_driver gpio_keys_polled_driver = {
	.probe	= gpio_keys_polled_probe,
	.remove	= __devexit_p(gpio_keys_polled_remove),
	.driver	= {
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init gpio_keys_polled_init(void)
{
	return platform_driver_register(&gpio_keys_polled_driver);
}

static void __exit gpio_keys_polled_exit(void)
{
	platform_driver_unregister(&gpio_keys_polled_driver);
}
module_init(gpio_keys_polled_init);
module_exit(gpio_keys_polled_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Daisuke Mizobuchi <mizo@atmark-techno.com>");
MODULE_DESCRIPTION("Polled GPIO Buttons driver");
MODULE_ALIAS("platform:" DRV_NAME);
