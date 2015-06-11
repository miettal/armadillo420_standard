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
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/irq.h>

#include <asm/arch/gpio.h>

struct armadillo5x0_tactsw_info {
	struct input_dev *idev;
	struct armadillo5x0_gpio_port *port;
};

extern void gpio_tactsw_active(void);
extern void gpio_tactsw_inactive(void);

static int
armadillo5x0_tactsw_open(struct input_dev *idev)
{
	struct armadillo5x0_tactsw_info *info = dev_get_drvdata(&idev->dev);

	mxc_set_gpio_direction(info->port->pin, 1/*INPUT*/);
	enable_irq(info->port->irq);

	if (mxc_get_gpio_datain(info->port->pin))
		set_irq_type(info->port->irq, IRQT_FALLING);
	else
		set_irq_type(info->port->irq, IRQT_RISING);

	return 0;
}

static void
armadillo5x0_tactsw_close(struct input_dev *idev)
{
	struct armadillo5x0_tactsw_info *info = dev_get_drvdata(&idev->dev);

	disable_irq(info->port->irq);
}

static irqreturn_t
armadillo5x0_tactsw_irq_handler(int irq, void *dev_id)
{
	struct input_dev *idev = (struct input_dev *)dev_id;
	struct armadillo5x0_tactsw_info *info = dev_get_drvdata(&idev->dev);

	if (mxc_get_gpio_datain(info->port->pin)) {
		set_irq_type(info->port->irq, IRQT_FALLING);
		input_event(idev, EV_SW, 0, 0); /* Up! */
	} else {
		set_irq_type(info->port->irq, IRQT_RISING);
		input_event(idev, EV_SW, 0, 1); /* Down! */
	}

	input_sync(idev);

	return IRQ_HANDLED;
}

static int
armadillo5x0_tactsw_probe(struct platform_device *pdev)
{
	struct armadillo5x0_gpio_private *priv = pdev->dev.platform_data;
	struct armadillo5x0_tactsw_info *info;
	int ret;
	int i;

	info = kzalloc(sizeof(struct armadillo5x0_tactsw_info) * priv->nr_gpio,
		       GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	for (i=0; i<priv->nr_gpio; i++) {
		info[i].idev = input_allocate_device();
		if (!info[i].idev) {
			while (i-- >= 0)
				input_free_device(info[i].idev);
			return -ENOMEM;
		}
	}

	for (i=0; i<priv->nr_gpio; i++) {
		ret = request_irq(priv->ports[i].irq,
				  armadillo5x0_tactsw_irq_handler,
				  IRQF_DISABLED,
				  priv->ports[i].name,
				  info[i].idev);
		if (ret < 0) {
			while (i-- >= 0)
				free_irq(priv->ports[i].irq, info[i].idev);
			goto _err_request_irq;
		}
		disable_irq(priv->ports[i].irq);
	}

	for (i=0; i<priv->nr_gpio; i++) {
		info[i].idev->name = priv->ports[i].name;
		info[i].idev->phys = NULL;
		info[i].idev->id.bustype = BUS_HOST;
		info[i].idev->dev.parent = &pdev->dev;

		info[i].idev->open = armadillo5x0_tactsw_open;
		info[i].idev->close = armadillo5x0_tactsw_close;

		info[i].idev->evbit[0] = BIT(EV_SW);
		info[i].idev->swbit[0] = BIT(0);

		ret = input_register_device(info[i].idev);
		if (ret < 0) {
			while (i-- >= 0)
				input_unregister_device(info[i].idev);
			goto _err_input_register_device;
		}

		info[i].port = &priv->ports[i];
		dev_set_drvdata(&info[i].idev->dev, &info[i]);
	}

	gpio_tactsw_active();

	platform_set_drvdata(pdev, info);

	return 0;

_err_input_register_device:
	for (i=0; i<priv->nr_gpio; i++)
		free_irq(priv->ports[i].irq, info[i].idev);

_err_request_irq:
	for (i=0; i<priv->nr_gpio; i++)
		input_free_device(info[i].idev);

	kfree(info);

	return ret;
}

static int
armadillo5x0_tactsw_remove(struct platform_device *pdev)
{
	struct armadillo5x0_gpio_private *priv = pdev->dev.platform_data;
	struct armadillo5x0_tactsw_info *info = platform_get_drvdata(pdev);
	int i;

	gpio_tactsw_active();

	platform_set_drvdata(pdev, NULL);

	for (i=0; i<priv->nr_gpio; i++) {
		input_unregister_device(info[i].idev);
		free_irq(info[i].port->irq, info[i].idev);
		input_free_device(info[i].idev);
	}

	kfree(info);

	return 0;
}

static struct platform_driver armadillo5x0_tactsw_driver = {
	.probe	= armadillo5x0_tactsw_probe,
	.remove = __devexit_p(armadillo5x0_tactsw_remove),
	.driver	= {
		.name = "armadillo5x0_tactsw",
	},
};

static int __init
armadillo5x0_tactsw_init(void)
{
	return platform_driver_register(&armadillo5x0_tactsw_driver);
}

static void __exit
armadillo5x0_tactsw_exit(void)
{
	platform_driver_unregister(&armadillo5x0_tactsw_driver);
}

module_init(armadillo5x0_tactsw_init);
module_exit(armadillo5x0_tactsw_exit);

MODULE_AUTHOR("Atmark Techno, Inc.");
MODULE_DESCRIPTION("Armadillo-5x0 Tact-SW driver");
MODULE_LICENSE("GPL v2");
