/*
 * Armadillo-2x0 compatible LED Driver
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

#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/timer.h>

#include <linux/armadillo2x0_led.h>

#define AUTHOR      "Atmark Techno, Inc."
#define DESCRIPTION "Armadillo-2x0 LED driver"

#define LED_MINOR   (215)
#define LED_DEVICE  "led"

#define VERSION     LED_DEVICE ": " DESCRIPTION ", (C) 2010 " AUTHOR

#define LED_IS_OPEN    (0x01)
#define LED_READ_COMP  (0x02)
#define LED_WRITE_COMP (0x04)

#define ARMADILLO2X0_LED_ON       (1 << 0)
#define ARMADILLO2X0_LED_OFF      (1 << 1)
#define ARMADILLO2X0_LED_STATUS   (1 << 2)

struct armadillo2x0_led_info {
	unsigned gpio;
	struct timer_list timer;
	unsigned int status;
	unsigned int blinkstatus;
	unsigned int active_low;
};

static spinlock_t led_lock = SPIN_LOCK_UNLOCKED;
static unsigned long led_rivalstatus;

static struct armadillo2x0_led_info led_red;
static struct armadillo2x0_led_info led_green;

static int led_cont(struct armadillo2x0_led_info *info, int mode)
{
	pr_debug("led_cont\n");

	switch (mode) {
	case ARMADILLO2X0_LED_ON:
		gpio_direction_output(info->gpio, !info->active_low);
		return 1;

	case ARMADILLO2X0_LED_OFF:
		gpio_direction_output(info->gpio, !!info->active_low);
		return 0;

	case ARMADILLO2X0_LED_STATUS:
		/* value    active_low return
		 * 0 (low)  false      0 (OFF)
		 * 0 (low)  true       1 (ON)
		 * 1 (high) false      1 (ON)
		 * 1 (high) true       0 (OFF)
		 */
		return (!!gpio_get_value(info->gpio)) ^ (!!info->active_low);
	}

	return 0;
}

static inline int led_on(struct armadillo2x0_led_info *info)
{
	if (info->blinkstatus == ARMADILLO2X0_LED_ON)
		info->status = ARMADILLO2X0_LED_ON;

	return led_cont(info, ARMADILLO2X0_LED_ON);
}

static inline int led_off(struct armadillo2x0_led_info *info)
{
	if (info->blinkstatus == ARMADILLO2X0_LED_ON)
		info->status = ARMADILLO2X0_LED_OFF;

	return led_cont(info, ARMADILLO2X0_LED_OFF);
}

static inline int led_status(struct armadillo2x0_led_info *info)
{
	if (info->blinkstatus == ARMADILLO2X0_LED_ON)
		return info->status == ARMADILLO2X0_LED_ON;

	return led_cont(info, ARMADILLO2X0_LED_STATUS);
}

static void led_blinkon(struct armadillo2x0_led_info *info)
{
	if (info->blinkstatus != ARMADILLO2X0_LED_ON) {
		info->blinkstatus = ARMADILLO2X0_LED_ON;
		led_cont(info, led_cont(info, ARMADILLO2X0_LED_STATUS) ? ARMADILLO2X0_LED_OFF : ARMADILLO2X0_LED_ON);
		mod_timer(&info->timer, jiffies + HZ/2);
	}
}

static void led_blinkoff(struct armadillo2x0_led_info *info)
{
	if (info->blinkstatus != ARMADILLO2X0_LED_OFF) {
		info->blinkstatus = ARMADILLO2X0_LED_OFF;
		del_timer_sync(&info->timer);
		led_cont(info, info->status);
	}
}

static inline int led_blinkstatus(struct armadillo2x0_led_info *info)
{
	return info->blinkstatus == ARMADILLO2X0_LED_ON;
}

static void armadillo2x0_led_timerfunc(unsigned long data)
{
	int tmp;
	struct armadillo2x0_led_info *info = (struct armadillo2x0_led_info *)data;

	tmp = led_cont(info, ARMADILLO2X0_LED_STATUS) ? ARMADILLO2X0_LED_OFF : ARMADILLO2X0_LED_ON;
	led_cont(info, tmp);
	mod_timer(&info->timer, jiffies + HZ/2);
}

static int led_open(struct inode *inode, struct file *file)
{
	pr_debug("led_open\n");

	spin_lock_irq(&led_lock);

	if (led_rivalstatus & LED_IS_OPEN) {
		spin_unlock_irq(&led_lock);
		return -EBUSY;
	}

	led_rivalstatus = LED_IS_OPEN;
	spin_unlock_irq(&led_lock);

	return 0;
}

static int led_release(struct inode *inode, struct file *file)
{
	pr_debug("led_release\n");

	spin_lock_irq(led_lock);
	led_rivalstatus = 0;
	spin_unlock_irq(led_lock);

	return 0;
}

static ssize_t led_read(struct file *file, char __user *buf,
                        size_t count, loff_t *ppos)
{
	unsigned long tmp;
	pr_debug("led_read\n");

	if (count < 1)
		return -EINVAL;

	spin_lock_irq(&led_lock);
	if (led_rivalstatus & LED_READ_COMP) {
		spin_unlock_irq(&led_lock);
		return 0;
	}

	led_rivalstatus |= LED_READ_COMP;
	spin_unlock_irq(&led_lock);

	tmp =  (led_status(&led_red) ? LED_RED : 0);
	tmp |= (led_status(&led_green) ? LED_GREEN : 0);

	tmp = sprintf(buf, "%1lx", tmp);
	return tmp;
}

static ssize_t led_write(struct file *file, const char __user *buf,
			 size_t count, loff_t *ppos)
{
	unsigned long tmp;
	pr_debug("led_write\n");

	if (count < 1)
		return -EINVAL;

	spin_lock_irq(&led_lock);
	if (led_rivalstatus & LED_WRITE_COMP) {
		spin_unlock_irq(&led_lock);
		return -EFBIG;
	}

	led_rivalstatus |= LED_WRITE_COMP;
	spin_unlock_irq(&led_lock);

	tmp = simple_strtol(buf, NULL, 16);

	if (tmp & LED_RED)
		led_on(&led_red);
	else
		led_off(&led_red);

	if (tmp & LED_GREEN)
		led_on(&led_green);
	else
		led_off(&led_green);

	return count;
}

static int led_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
                     unsigned long arg)
{
	char *buf = (char *)arg;

	pr_debug("led_ioctl\n");

	switch (cmd) {
	case LED_RED_ON:
		led_on(&led_red);
		break;
	case LED_RED_OFF:
		led_off(&led_red);
		break;
	case LED_RED_BLINKON:
		led_blinkon(&led_red);
		break;
	case LED_RED_BLINKOFF:
		led_blinkoff(&led_red);
		break;
	case LED_GREEN_ON:
		led_on(&led_green);
		break;
	case LED_GREEN_OFF:
		led_off(&led_green);
		break;
	case LED_GREEN_BLINKON:
		led_blinkon(&led_green);
		break;
	case LED_GREEN_BLINKOFF:
		led_blinkoff(&led_green);
		break;
	default:
		if (!(cmd & (LED_RED_STATUS | LED_GREEN_STATUS |
			     LED_RED_BLINK | LED_GREEN_BLINK)))
			return -EINVAL;
		buf[0] = 0;
		if (cmd & LED_RED_STATUS)
			buf[0] |= (led_status(&led_red) ? LED_RED : 0);
		if (cmd & LED_GREEN_STATUS)
			buf[0] |= (led_status(&led_green) ? LED_GREEN : 0);
		if (cmd & LED_RED_BLINKSTATUS)
			buf[0] |= (led_blinkstatus(&led_red) ? LED_RED_BLINK : 0);
		if (cmd & LED_RED_BLINKSTATUS)
			buf[0] |= (led_blinkstatus(&led_green) ? LED_GREEN_BLINK : 0);
	}

	return 0;
}


static struct file_operations led_fops = {
	.owner   = THIS_MODULE,
	.open    = led_open,
	.release = led_release,
	.read    = led_read,
	.write   = led_write,
	.ioctl   = led_ioctl,
};

static struct miscdevice led_dev = {
	.minor = LED_MINOR,
	.name  = LED_DEVICE,
	.fops  = &led_fops,
};

static int __devinit armadillo2x0_led_probe(struct platform_device *pdev)
{
	struct armadillo2x0_led_platform_data *pdata = pdev->dev.platform_data;
	int ret;

	pr_debug("led_init\n");

	led_red.gpio = pdata->led_red_data.gpio;
	led_red.active_low = pdata->led_red_data.active_low;
	led_red.status = ARMADILLO2X0_LED_OFF;
	led_red.blinkstatus = ARMADILLO2X0_LED_OFF;
	setup_timer(&led_red.timer, armadillo2x0_led_timerfunc,
		    (unsigned long)&led_red);

	led_green.gpio = pdata->led_green_data.gpio;
	led_green.active_low = pdata->led_green_data.active_low;
	led_green.status = ARMADILLO2X0_LED_OFF;
	led_green.blinkstatus = ARMADILLO2X0_LED_OFF;
	setup_timer(&led_green.timer, armadillo2x0_led_timerfunc,
		    (unsigned long)&led_green);

	ret = gpio_request(led_red.gpio, "armadillo2x0_led");
	if (ret < 0) {
		pr_err("failed to request gpio%d\n", led_red.gpio);
		return -ENODEV;
	}

	ret = gpio_request(led_green.gpio, "armadillo2x0_led");
	if (ret < 0) {
		pr_err("failed to request gpio%d\n", led_green.gpio);
		return -ENODEV;
	}

	if (misc_register(&led_dev))
		return -ENODEV;

	pr_info(VERSION "\n");

	return 0;
}

static int __devexit armadillo2x0_led_remove(struct platform_device *pdev)
{
	pr_debug("led_exit\n");

	led_blinkoff(&led_red);
	led_blinkoff(&led_green);

	misc_deregister(&led_dev);

	gpio_free(led_red.gpio);
	gpio_free(led_green.gpio);

	return 0;
}

static struct platform_driver armadillo2x0_led_driver = {
	.driver = {
		.name = "armadillo2x0-led",
	},
	.probe = armadillo2x0_led_probe,
	.remove = __devexit_p(armadillo2x0_led_remove),
};

static int __init armadillo2x0_led_init(void)
{
	return platform_driver_register(&armadillo2x0_led_driver);
}
module_init(armadillo2x0_led_init);

static void __exit armadillo2x0_led_exit(void)
{
	platform_driver_unregister(&armadillo2x0_led_driver);
}
module_exit(armadillo2x0_led_exit);

MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESCRIPTION);
MODULE_LICENSE("GPL");
