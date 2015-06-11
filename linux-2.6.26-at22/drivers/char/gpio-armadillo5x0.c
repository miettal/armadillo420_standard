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

#include <asm/arch/gpio.h>

#define ATTR_NAME_SIZE		(16)
#define ATTR_NAME_SUFFIX_DIR	"_dir"
#define ATTR_NAME_SUFFIX_DATA	"_data"

struct armadillo5x0_gpio_port_info {
	char attr_dir_name[ATTR_NAME_SIZE];
	char attr_data_name[ATTR_NAME_SIZE];
	struct device_attribute attr_dir;
	struct device_attribute attr_data;
	struct armadillo5x0_gpio_port *port;
};

struct armadillo5x0_gpio_drv_info {
	struct attribute_group attr_group;
	struct attribute **gpio_attrs;

	int nr_port;
	struct armadillo5x0_gpio_port_info *pinfo;
};

extern int mxc_jtag_type;
extern void gpio_keypad_active(void);
extern void gpio_keypad_inactive(void);
extern void gpio_sensor_active(void);
extern void gpio_sensor_inactive(void);

static struct armadillo5x0_gpio_port *
get_gpio_port(struct device *dev, struct device_attribute *attr)
{
	struct armadillo5x0_gpio_drv_info *dinfo = dev_get_drvdata(dev);
	int ret;
	int i, suf_off, name_len = 0;

	suf_off = strlen(attr->attr.name) - strlen(ATTR_NAME_SUFFIX_DIR);
	if (strcmp(attr->attr.name + suf_off, ATTR_NAME_SUFFIX_DIR) == 0)
		name_len = suf_off;

	suf_off = strlen(attr->attr.name) - strlen(ATTR_NAME_SUFFIX_DATA);
	if (strcmp(attr->attr.name + suf_off, ATTR_NAME_SUFFIX_DATA) == 0)
		name_len = suf_off;

	if (name_len == 0)
		return NULL;

	for (i=0; i<dinfo->nr_port; i++) {
		ret = strncmp(attr->attr.name, dinfo->pinfo[i].port->name,
			      name_len);
		if (ret == 0)
			return dinfo->pinfo[i].port;
	}

	return NULL;
}


static ssize_t
gpio_dir_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct armadillo5x0_gpio_port *port = get_gpio_port(dev, attr);
	int dir;

	dir = mxc_get_gpio_direction(port->pin);
  
	return snprintf(buf, PAGE_SIZE, "%d\n", (dir ? 0 : 1));
}

static ssize_t
gpio_data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct armadillo5x0_gpio_port *port = get_gpio_port(dev, attr);
	int data;

	data = mxc_get_gpio_datain(port->pin);

	return snprintf(buf, PAGE_SIZE, "%d\n", data);
}

static ssize_t
gpio_dir_store(struct device *dev, struct device_attribute *attr,
	       const char *buf, size_t count)
{
	struct armadillo5x0_gpio_port *port = get_gpio_port(dev, attr);
	int dir;

	if (port->dir_ro)
		return -EPERM;

	dir = (int)simple_strtol(buf, NULL, 0);
	mxc_set_gpio_direction(port->pin, (dir ? 0 : 1));

	return count;
}

static ssize_t
gpio_data_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct armadillo5x0_gpio_port *port = get_gpio_port(dev, attr);
	int data;

	data = (int)simple_strtol(buf, NULL, 0);
	mxc_set_gpio_dataout(port->pin, (data ? 1 : 0));

	return count;
}

static int
armadillo5x0_gpio_create_sysfs_node(struct platform_device *pdev,
				    struct armadillo5x0_gpio_drv_info *dinfo)
{
	struct armadillo5x0_gpio_private *priv = pdev->dev.platform_data;
	struct device *dev = &pdev->dev;
	int ret;
	int i;

	dinfo->pinfo = kzalloc(sizeof(struct armadillo5x0_gpio_port_info) * 
			       priv->nr_gpio, GFP_KERNEL);
	if (!dinfo->pinfo)
		return -ENOMEM;

	dinfo->gpio_attrs = kzalloc(sizeof(struct attribute *) * 
				    priv->nr_gpio * 2 + 1, GFP_KERNEL);
	if (!dinfo->gpio_attrs) {
		kfree(dinfo->pinfo);
		return -ENOMEM;
	}

	for (i=0; i<(priv->nr_gpio * 2); i+=2) {
		snprintf(dinfo->pinfo[i/2].attr_dir_name, ATTR_NAME_SIZE,
			 "%s"ATTR_NAME_SUFFIX_DIR, priv->ports[i/2].name);
		dinfo->pinfo[i/2].attr_dir.attr.name =
			dinfo->pinfo[i/2].attr_dir_name;
		if (priv->ports[i/2].dir_ro)
			dinfo->pinfo[i/2].attr_dir.attr.mode = S_IRUGO;
		else
			dinfo->pinfo[i/2].attr_dir.attr.mode = 
					S_IRUGO | S_IWUSR | S_IWGRP;
		dinfo->pinfo[i/2].attr_dir.show = gpio_dir_show;
		dinfo->pinfo[i/2].attr_dir.store = gpio_dir_store;

		snprintf(dinfo->pinfo[i/2].attr_data_name, ATTR_NAME_SIZE,
			 "%s"ATTR_NAME_SUFFIX_DATA, priv->ports[i/2].name);
		dinfo->pinfo[i/2].attr_data.attr.name =
			dinfo->pinfo[i/2].attr_data_name;
		dinfo->pinfo[i/2].attr_data.attr.mode = 
				S_IRUGO | S_IWUSR | S_IWGRP;
		dinfo->pinfo[i/2].attr_data.show = gpio_data_show;
		dinfo->pinfo[i/2].attr_data.store = gpio_data_store;

		dinfo->pinfo[i/2].port = &priv->ports[i/2];

		dinfo->gpio_attrs[ i ] =
			(struct attribute *)&dinfo->pinfo[i/2].attr_dir;
		dinfo->gpio_attrs[i+1] =
			(struct attribute *)&dinfo->pinfo[i/2].attr_data;
	}

	dinfo->nr_port = priv->nr_gpio;
	dinfo->attr_group.name = "ports";
	dinfo->attr_group.attrs = dinfo->gpio_attrs;

	if (get_device(dev)) {
		ret = sysfs_create_group(&dev->kobj, &dinfo->attr_group);
		if (ret < 0) {
			kfree(dinfo->pinfo);
			kfree(dinfo->gpio_attrs);
			return ret;
		}
		put_device(dev);
	}

	return 0;
}

static int
armadillo5x0_gpio_remove_sysfs_node(struct platform_device *pdev,
				    struct armadillo5x0_gpio_drv_info *dinfo)
{
	struct device *dev = &pdev->dev;

	sysfs_remove_group(&dev->kobj, &dinfo->attr_group);

	kfree(dinfo->pinfo);
	kfree(dinfo->gpio_attrs);

	return 0;
}

static int
armadillo5x0_gpio_probe(struct platform_device *pdev)
{
	struct armadillo5x0_gpio_drv_info *dinfo;
	int ret;

	dinfo = kzalloc(sizeof(struct armadillo5x0_gpio_drv_info),
			GFP_KERNEL);
	if (!dinfo)
		return -ENOMEM;

#ifdef CONFIG_MACH_ARMADILLO500FX
	gpio_sensor_inactive();
#if !defined(CONFIG_KEYBOARD_MXC) && !defined(CONFIG_KEYBOARD_MXC_MODULE)
	gpio_keypad_inactive();
#endif
#else
	gpio_keypad_inactive();
#endif

	ret = armadillo5x0_gpio_create_sysfs_node(pdev, dinfo);
	if (ret < 0) {
		kfree(dinfo);
		return ret;
	}

	dev_set_drvdata(&pdev->dev, dinfo);

	return 0;
}

static int
armadillo5x0_gpio_remove(struct platform_device *pdev)
{
	struct armadillo5x0_gpio_drv_info *dinfo = dev_get_drvdata(&pdev->dev);

	dev_set_drvdata(&pdev->dev, NULL);

	armadillo5x0_gpio_remove_sysfs_node(pdev, dinfo);
	kfree(dinfo);

	return 0;
}

static struct platform_driver armadillo5x0_gpio_driver = {
	.probe	= armadillo5x0_gpio_probe,
	.remove	= __devexit_p(armadillo5x0_gpio_remove),
	.driver	= {
		.name = "armadillo5x0_gpio",
	},
};

static int __init
armadillo5x0_gpio_init(void)
{
	if (mxc_jtag_type >= 2) {
		pr_info("ETM is in use. GPIO is disabled.\n");
		return -EBUSY;
	}

	return platform_driver_register(&armadillo5x0_gpio_driver);
}

static void __exit
armadillo5x0_gpio_exit(void)
{
	platform_driver_unregister(&armadillo5x0_gpio_driver);
}

module_init(armadillo5x0_gpio_init);
module_exit(armadillo5x0_gpio_exit);

MODULE_AUTHOR("Atmark Techno, Inc.");
MODULE_DESCRIPTION("Armadillo-5x0 Sample GPIO driver");
MODULE_LICENSE("GPL v2");
