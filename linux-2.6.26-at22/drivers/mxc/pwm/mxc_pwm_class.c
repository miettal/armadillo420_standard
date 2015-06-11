/*
 * Copyright (C) 2010 Atmark Techno, Inc. All Rights Reserved.
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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/sysdev.h>
#include <linux/err.h>
#include <linux/ctype.h>
#include <linux/pwm.h>

extern void gpio_activate_pwm(int id);

static struct class *mxc_pwm_class;

static int _strtoul(const char *buf, ssize_t size, unsigned long *val)
{
	int ret = -EINVAL;
	char *after;
	unsigned long value = simple_strtoul(buf, &after, 10);
	int count = after - buf;

	if (*after && isspace(*after))
		count++;

	if (count == size) {
		*val = value;
		ret = count;
	}

	return ret;
}

static int _strtobool(const char *buf, ssize_t size, int *val)
{
	unsigned long value;
	unsigned long ret = _strtoul(buf, size, &value);

	if (ret)
		*val = (value) ? 1 : 0;

	return ret;
}

static ssize_t mxc_pwm_class_period_ns_show(struct device *dev,
					 struct device_attribute *attr, char *buf)
{
	struct mxc_pwm_classdev *pwm_cdev = dev_get_drvdata(dev);
	ssize_t ret = 0;

	ret = sprintf(buf, "%ld\n", pwm_cdev->period_ns);

	return ret;
}

static ssize_t mxc_pwm_class_period_ns_store(struct device *dev,
					  struct device_attribute *attr, const char *buf, size_t size)
{
	struct mxc_pwm_classdev *pwm_cdev = dev_get_drvdata(dev);
	unsigned long period_ns;
	int ret;

	ret = _strtoul(buf, size, &period_ns);
	if (0 < ret) {
		if (pwm_cdev->enable) {
			if (pwm_config(pwm_cdev->pwm, pwm_cdev->duty_ns, period_ns) != 0)
				return -EINVAL;
		}
		pwm_cdev->period_ns = period_ns;
	}

	return ret;
}

static DEVICE_ATTR(period_ns, 0644, mxc_pwm_class_period_ns_show, mxc_pwm_class_period_ns_store);

static ssize_t mxc_pwm_class_duty_ns_show(struct device *dev,
					 struct device_attribute *attr, char *buf)
{
	struct mxc_pwm_classdev *pwm_cdev = dev_get_drvdata(dev);
	ssize_t ret = 0;

	ret = sprintf(buf, "%ld\n", pwm_cdev->duty_ns);

	return ret;
}

static ssize_t mxc_pwm_class_duty_ns_store(struct device *dev,
					  struct device_attribute *attr, const char *buf, size_t size)
{
	struct mxc_pwm_classdev *pwm_cdev = dev_get_drvdata(dev);
	unsigned long duty_ns;
	int ret;

	ret = _strtoul(buf, size, &duty_ns);
	if (0 < ret) {
		if (pwm_cdev->enable) {
			if (pwm_config(pwm_cdev->pwm, duty_ns, pwm_cdev->period_ns) != 0)
				return -EINVAL;
		}
		pwm_cdev->duty_ns = duty_ns;
	}

	return ret;
}

static DEVICE_ATTR(duty_ns, 0644, mxc_pwm_class_duty_ns_show, mxc_pwm_class_duty_ns_store);

static ssize_t mxc_pwm_class_invert_show(struct device *dev,
					 struct device_attribute *attr, char *buf)
{
	struct mxc_pwm_classdev *pwm_cdev = dev_get_drvdata(dev);
	ssize_t ret = 0;

	ret = sprintf(buf, "%d\n", pwm_cdev->invert);

	return ret;
}

static ssize_t mxc_pwm_class_invert_store(struct device *dev,
					  struct device_attribute *attr, const char *buf, size_t size)
{
	struct mxc_pwm_classdev *pwm_cdev = dev_get_drvdata(dev);
	int invert;
	int ret;

	ret = _strtobool(buf, size, &invert);
	if (0 < ret) {
		if (pwm_cdev->enable) {
			if (pwm_set_invert(pwm_cdev->pwm, invert) != 0)
				return -EINVAL;
			if (pwm_config(pwm_cdev->pwm, pwm_cdev->duty_ns, pwm_cdev->period_ns) != 0)
				return -EINVAL;
		}
		pwm_cdev->invert = invert;
	}

	return ret;
}

static DEVICE_ATTR(invert, 0644, mxc_pwm_class_invert_show, mxc_pwm_class_invert_store);

static ssize_t mxc_pwm_class_enable_show(struct device *dev,
					 struct device_attribute *attr, char *buf)
{
	struct mxc_pwm_classdev *pwm_cdev = dev_get_drvdata(dev);
	ssize_t ret = 0;

	ret = sprintf(buf, "%d\n", pwm_cdev->enable);

	return ret;
}

static ssize_t mxc_pwm_class_enable_store(struct device *dev,
					  struct device_attribute *attr, const char *buf, size_t size)
{
	struct mxc_pwm_classdev *pwm_cdev = dev_get_drvdata(dev);
	int ret;
	int enable;

 	ret = _strtobool(buf, size, &enable);
	if (0 < ret) {
		if (enable && !pwm_cdev->enable) {
			ret = pwm_set_invert(pwm_cdev->pwm, pwm_cdev->invert);
			if (ret)
				return ret;
			ret = pwm_config(pwm_cdev->pwm, pwm_cdev->duty_ns, pwm_cdev->period_ns);
			if (ret)
				return ret;
			ret = pwm_enable(pwm_cdev->pwm);
			if (ret)
				return ret;
			pwm_cdev->enable = 1;
		} else if (!enable && pwm_cdev->enable) {
			pwm_disable(pwm_cdev->pwm);
			pwm_cdev->enable = 0;
		}
	}

	return ret;
}

static DEVICE_ATTR(enable, 0644, mxc_pwm_class_enable_show, mxc_pwm_class_enable_store);

int mxc_pwm_classdev_register(struct device *parent, int id, struct mxc_pwm_classdev *pwm_cdev)
{
	int ret;

	gpio_activate_pwm(id);

	pwm_cdev->pwm = pwm_request(id, pwm_cdev->name);
	if (IS_ERR(pwm_cdev->pwm))
		return PTR_ERR(pwm_cdev->pwm);

	pwm_cdev->dev = device_create_drvdata(mxc_pwm_class, parent, 0, pwm_cdev,
					      "%s", pwm_cdev->name);
	if (IS_ERR(pwm_cdev->dev)) {
		ret = PTR_ERR(pwm_cdev->dev);
		goto err_device_create;
	}

	/* register the attributes */
	ret = device_create_file(pwm_cdev->dev, &dev_attr_period_ns);
	if (ret) {
		pr_err("device_create_file failed\n");
		goto err_device_create_file_period_ns;
	}

	ret = device_create_file(pwm_cdev->dev, &dev_attr_duty_ns);
	if (ret) {
		pr_err("device_create_file failed\n");
		goto err_device_create_file_duty_ns;
	}

	ret = device_create_file(pwm_cdev->dev, &dev_attr_invert);
	if (ret) {
		pr_err("device_create_file failed\n");
		goto err_device_create_file_invert;
	}

	ret = device_create_file(pwm_cdev->dev, &dev_attr_enable);
	if (ret) {
		pr_err("device_create_file failed\n");
		goto err_device_create_file_enable;
	}

	pr_info("Registered MXC PWM device: %s\n", pwm_cdev->name);

	return 0;

err_device_create_file_enable:
	device_remove_file(pwm_cdev->dev, &dev_attr_invert);
err_device_create_file_invert:
	device_remove_file(pwm_cdev->dev, &dev_attr_duty_ns);
err_device_create_file_duty_ns:
	device_remove_file(pwm_cdev->dev, &dev_attr_period_ns);
err_device_create_file_period_ns:
	device_unregister(pwm_cdev->dev);
err_device_create:
	pwm_free(pwm_cdev->pwm);

	return ret;
}
EXPORT_SYMBOL_GPL(mxc_pwm_classdev_register);

int mxc_pwm_class_suspend(struct mxc_pwm_classdev *pwm_cdev)
{
	if (pwm_cdev->enable)
		pwm_disable(pwm_cdev->pwm);
	return 0;
}
EXPORT_SYMBOL_GPL(mxc_pwm_class_suspend);

int mxc_pwm_class_resume(struct mxc_pwm_classdev *pwm_cdev)
{
	if (pwm_cdev->enable)
		return pwm_enable(pwm_cdev->pwm);
	return 0;
}
EXPORT_SYMBOL_GPL(mxc_pwm_class_resume);

void mxc_pwm_classdev_unregister(struct mxc_pwm_classdev *pwm_cdev)
{
	device_remove_file(pwm_cdev->dev, &dev_attr_enable);
	device_remove_file(pwm_cdev->dev, &dev_attr_invert);
	device_remove_file(pwm_cdev->dev, &dev_attr_duty_ns);
	device_remove_file(pwm_cdev->dev, &dev_attr_period_ns);

	device_unregister(pwm_cdev->dev);

	pwm_free(pwm_cdev->pwm);
}
EXPORT_SYMBOL_GPL(mxc_pwm_classdev_unregister);

int mxc_pwm_class_init(void)
{
	mxc_pwm_class = class_create(THIS_MODULE, "mxc_pwm");
	if (IS_ERR(mxc_pwm_class))
		return PTR_ERR(mxc_pwm_class);
	return 0;
}
EXPORT_SYMBOL_GPL(mxc_pwm_class_init);

void mxc_pwm_class_exit(void)
{
	class_destroy(mxc_pwm_class);
}
EXPORT_SYMBOL_GPL(mxc_pwm_class_exit);

MODULE_AUTHOR("Atmark Techno, Inc.");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MXC PWM Class Interface");
