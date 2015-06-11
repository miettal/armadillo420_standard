/*
 * Armadillo-2x0 compatible GPIO Driver
 * Copyright (C) 2010 Atmark Techno, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <asm/io.h>

#include <linux/armadillo2x0_gpio.h>

#define AUTHOR      "Atmark Techno, Inc."
#define DESCRIPTION "Armadillo-2x0 GPIO driver"

#define GPIO_MINOR   (185)
#define GPIO_DEVICE  "gpio"

#define VERSION     GPIO_DEVICE ": " DESCRIPTION ", (C) 2010 " AUTHOR

struct wait_handler{
	wait_queue_head_t queue;
	unsigned long wait_list;
	unsigned long int_list;

	struct wait_handler *prev;
	struct wait_handler *next;
};

static struct wait_handler *wait_list_head;
static struct armadillo2x0_gpio_info *gpio;
static int gpio_num;

#define change_reg_bit(REG,BIT,VALUE)       \
({                                          \
	unsigned tmp;                       \
	tmp = (inl(REG) & ~BIT);            \
	outl(tmp | (VALUE ? BIT : 0), REG); \
})

#define get_reg_bit(REG,BIT)                \
	inl(REG) & BIT

#if defined(DEBUG)
static void debug_wait_handle(struct wait_handler *head)
{
	int count = 0;
	struct wait_handler *tmp;

	pr_debug("debug_wait_handle");
	pr_debug("display wait_handle_list\n");
	if (!head){
		pr_err("list not found\n");
		return;
	}

	for (tmp = head; tmp != NULL; tmp = tmp->next) {
		pr_debug("chain[%2d]: 0x%08lx\n", count++, (unsigned long)tmp);
		pr_debug("      prev: 0x%08lx\n", (unsigned long)tmp->prev);
		pr_debug("      next: 0x%08lx\n", (unsigned long)tmp->next);
		pr_debug("\n");
	}
}
#else
#define debug_wait_handle(head)
#endif

static struct wait_handler *create_wait_handle(unsigned long wait_list)
{
	struct wait_handler *tmp;
	struct wait_handler *handle;

	pr_debug("create_wait_handle\n");
	handle = (struct wait_handler *)kmalloc(sizeof(struct wait_handler),
						GFP_KERNEL);
	if (!handle)
		return NULL;

	init_waitqueue_head(&handle->queue);
	handle->wait_list = wait_list;
	handle->int_list = 0;
	handle->next = NULL;

	if (wait_list_head) {
		tmp = wait_list_head;

		while(tmp->next)
			tmp = tmp->next;

		tmp->next = handle;
		handle->prev= tmp;
	} else {
		wait_list_head = handle;
		handle->prev = NULL;
	}

	debug_wait_handle(wait_list_head);

	return handle;
}

static void destroy_wait_handle(struct wait_handler **handle)
{
	struct wait_handler *tmp;

	pr_debug("destroy_wait_handle\n");
	tmp = wait_list_head;

	if(!*handle)
		return;

	while (tmp != *handle) {
		tmp = tmp->next;
		if(!tmp)
			return;
	}

	if(tmp->prev)
		tmp->prev->next = tmp->next;

	if(tmp->next)
		tmp->next->prev = tmp->prev;

	if(!tmp->prev)
		wait_list_head = tmp->next;

	kfree(*handle);

	*handle = NULL;

	debug_wait_handle(wait_list_head);
}

static irqreturn_t gpio_isr(int irq, void *dev_id)
{
	struct wait_handler *handle;
	int i;

	pr_debug("gpio_isr\n");

	if (!wait_list_head)
		return IRQ_HANDLED;

	for (handle = wait_list_head; handle != NULL; handle = handle->next) {
		for (i = 0; i < gpio_num; i++) {
			if (handle->wait_list & gpio[i].no &&
			    irq == gpio_to_irq(gpio[i].gpio)) {
				handle->int_list = gpio[i].no;
				wake_up_interruptible(&handle->queue);
			}
		}
	}

	return IRQ_HANDLED;
}

static int gpio_open(struct inode *inode, struct file *file)
{
	pr_debug("gpio_open\n");

	return 0;
}

static int gpio_release(struct inode *inode, struct file *file)
{
	pr_debug("gpio_release\n");

	return 0;
}

static unsigned long get_gpio_num(unsigned long no)
{
	int i;

	for (i = 0; i < gpio_num; i++)
		if (no == gpio[i].no)
			break;

	return i;
}

static int set_param(struct gpio_param *param)
{
	unsigned int num;
	int ret;

	pr_debug("set_param\n");

	num = get_gpio_num(param->no);
	if (num == gpio_num)
		return -1;

	ret = 0;
	switch (param->mode) {
	case MODE_OUTPUT:
		pr_debug("MODE_OUTPUT\n");

		gpio_direction_output(gpio[num].gpio, param->data.o.value);
		gpio[num].direction = ARMDILLO2X0_GPIO_DIRECTION_OUTPUT;

		pr_debug("ditect GPIO%d (%s)\n", num,
		       (param->data.o.value ? "ON" : "OFF"));
		break;

	case MODE_INPUT:
		pr_debug("MODE_INPUT\n");

		gpio_direction_input(gpio[num].gpio);
		gpio[num].direction = ARMDILLO2X0_GPIO_DIRECTION_INPUT;

		if (param->data.i.int_enable && gpio[num].can_interrupt) {
			int irq = gpio_to_irq(gpio[num].gpio);
			unsigned int irqflags[] = {IRQF_TRIGGER_LOW, IRQF_TRIGGER_HIGH,
						   IRQF_TRIGGER_FALLING, IRQF_TRIGGER_RISING};

			if (irq < 0) {
				ret = irq;
				pr_err("Unable to get irq number for gpio%d\n", gpio[num].gpio);
				ret = -1;
				break;
			}

			gpio[num].irq_type = param->data.i.int_type & 0x03;
			ret = request_irq(irq, gpio_isr,
					  irqflags[gpio[num].irq_type],
					  "armadillo2x0_gpio", NULL);
			if (ret) {
				pr_err("Unable to claim irq %d\n", irq);
				ret = -1;
				break;
			}
			gpio[num].interrupt_enabled = 1;

#if defined(CONFIG_ARCH_EP93XX)
			if (param->data.i.int_type & TYPE_DEBOUNCE) {
				if (gpio[num].db_reg) {
					gpio[num].debounce_enabled = 1;
					change_reg_bit(gpio[num].db_reg,
						       gpio[num].db_bit, 1);
					msleep(10);
				}
			}
#endif
		} else {
			int irq = gpio_to_irq(gpio[num].gpio);

			if (gpio[num].can_interrupt && gpio[num].interrupt_enabled) {
				free_irq(irq, NULL);
				gpio[num].interrupt_enabled = 0;

#if defined(CONFIG_ARCH_EP93XX)
				if (gpio[num].db_reg)
					change_reg_bit(gpio[num].db_reg,
						       gpio[num].db_bit, 0);
				gpio[num].debounce_enabled = 0;
#endif
			}
		}

		pr_debug("ditect GPIO%d\n", num);
		break;

	default:
		ret = -1;
	}

	return ret;
}

static int get_param(struct gpio_param *param)
{
	unsigned int num;

	pr_debug("get_param\n");

	num = get_gpio_num(param->no);
	if (num == gpio_num)
		return -EINVAL;

	if (gpio[num].direction == ARMDILLO2X0_GPIO_DIRECTION_OUTPUT) {
		param->mode = MODE_OUTPUT;

		param->data.o.value = gpio_get_value(gpio[num].gpio);

		pr_debug("ditect GPIO%d (%s)\n", num,
		       (param->data.o.value ? "ON" : "OFF"));
	} else {
		param->mode = MODE_INPUT;

		param->data.i.value = gpio_get_value(gpio[num].gpio);

		if (gpio[num].can_interrupt) {
			param->data.i.int_enable = gpio[num].interrupt_enabled;
			if (param->data.i.int_enable) {
				param->data.i.int_type = gpio[num].irq_type;
#if defined(CONFIG_ARCH_EP93XX)
				if (gpio[num].debounce_enabled)
					param->data.i.int_type |= (1 << 2);
#endif
			} else
				param->data.i.int_type = 0;
		}

		pr_debug("ditect GPIO%d\n", num);
	}

	return 0;
}

static int gpio_ioctl(struct inode *inode, struct file *file,
		      unsigned int cmd,unsigned long arg)
{
	struct wait_handler *wait_handle;
	struct wait_param *wait_param;
	struct gpio_param *param_list;
	struct gpio_param *param;
	int ret;

	pr_debug("gpio_ioctl\n");

	switch (cmd) {
	case PARAM_SET:
		pr_debug("PARAM_SET\n");
		param_list = (struct gpio_param *)arg;

		for (param = param_list; param != NULL; param = param->next) {
			ret = set_param(param);
			if (ret)
				return -EINVAL;
		}

		break;
	case PARAM_GET:
		pr_debug("PARAM_GET\n");
		param_list = (struct gpio_param *)arg;


		for (param = param_list; param != NULL; param = param->next) {
			ret = get_param(param);
			if (ret)
				return -EINVAL;
		}

		break;
	case INTERRUPT_WAIT:
		wait_param = (struct wait_param *)arg;
		pr_debug("INTERRUPT_WAIT\n");

		wait_handle = create_wait_handle(wait_param->list);
		if (!wait_handle)
			return -EBUSY;

		if (wait_param->timeout) {
			interruptible_sleep_on_timeout(&wait_handle->queue,
						       msecs_to_jiffies(wait_param->timeout));
		} else
			interruptible_sleep_on(&wait_handle->queue);

		wait_param->list = wait_handle->int_list;

		destroy_wait_handle(&wait_handle);

		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static struct file_operations gpio_fops = {
	.owner   = THIS_MODULE,
	.open    = gpio_open,
	.release = gpio_release,
	.ioctl   = gpio_ioctl,
};

static struct miscdevice gpio_dev = {
	.minor = GPIO_MINOR,
	.name  = GPIO_DEVICE,
	.fops  = &gpio_fops,
};

static int __devinit armadillo2x0_gpio_probe(struct platform_device *pdev)
{
	struct armadillo2x0_gpio_platform_data *pdata = pdev->dev.platform_data;
	int ret;
	int i;

	pr_debug("gpio_init\n");

	if (!pdata)
		return -ENODEV;

	gpio_num = pdata->gpio_num;
	gpio = pdata->gpio_info;

	for (i = 0; i < gpio_num; i++) {
		ret = gpio_request(gpio[i].gpio, "armadillo2x0_gpio");
		if (ret < 0) {
			pr_err("failed to request gpio%d\n", gpio[i].gpio);
			goto err;
		}

		if (gpio[i].default_direction == ARMDILLO2X0_GPIO_DIRECTION_INPUT) {
			gpio_direction_input(gpio[i].gpio);
			gpio[i].direction = ARMDILLO2X0_GPIO_DIRECTION_INPUT;
		} else {
			gpio_direction_output(gpio[i].gpio, gpio[i].default_value);
			gpio[i].direction = ARMDILLO2X0_GPIO_DIRECTION_OUTPUT;
		}
	}

	if (misc_register(&gpio_dev)) {
		ret = -ENODEV;
		goto err;
	}

	pr_info(VERSION "\n");

	return 0;
err:
	while(--i >= 0) {
		gpio_free(gpio[i].gpio);
	}

	return ret;
}

static int __devexit armadillo2x0_gpio_remove(struct platform_device *pdev)
{
	int i;

	pr_debug("gpio_exit\n");

	misc_deregister(&gpio_dev);

	for (i = 0; i < gpio_num; i++)
		gpio_free(gpio[i].gpio);

	return 0;
}

static struct platform_driver armadillo2x0_gpio_driver = {
	.driver = {
		.name = "armadillo2x0-gpio",
	},
	.probe = armadillo2x0_gpio_probe,
	.remove = __devexit_p(armadillo2x0_gpio_remove),
};

static int __init armadillo2x0_gpio_init(void)
{
	return platform_driver_register(&armadillo2x0_gpio_driver);
}
module_init(armadillo2x0_gpio_init);

static void __exit armadillo2x0_gpio_exit(void)
{
	platform_driver_unregister(&armadillo2x0_gpio_driver);
}
module_exit(armadillo2x0_gpio_exit);

MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESCRIPTION);
MODULE_LICENSE("GPL");
