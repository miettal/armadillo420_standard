/*
    lm75b.c - Part of lm_sensors, Linux kernel modules

    Copyright (C) Atmark Techno, Inc. All Rights Reserved.

    Based on:
    Copyright (c) 1998, 1999  Frodo Looijaard <frodol@dds.nl>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/lm75b.h>

/* Many LM75B constants specified below */

/* The LM75B registers */
#define LM75B_REG_CONF		0x01
static const u8 LM75B_REG_TEMP[3] = {
	0x00,		/* input */
	0x03,		/* max */
	0x02,		/* hyst */
};

/* Each client has this additional data */
struct lm75b_data {
	struct i2c_client	*client;
	struct mutex		update_lock;
	char			valid;		/* !=0 if following fields are valid */
	unsigned long		last_updated;	/* In jiffies */
	u16			temp[3];	/* Register values,
						   0 = input
						   1 = max
						   2 = hyst */
};

static int lm75b_probe(struct i2c_client *client, const struct i2c_device_id *id);
static void lm75b_init_client(struct i2c_client *client);
static int lm75b_remove(struct i2c_client *client);
static int lm75b_read_value(struct i2c_client *client, u8 reg);
static int lm75b_write_value(struct i2c_client *client, u8 reg, u16 value);
static struct lm75b_data *lm75b_update_device(struct device *dev);

static const struct i2c_device_id lm75b_id[] = {
	{ "lm75b", 0 },
	{ }
};

/* This is the driver that will be inserted */
static struct i2c_driver lm75b_driver = {
	.driver = {
		.name	= "lm75b",
		.owner = THIS_MODULE,
	},
	.probe = lm75b_probe,
	.remove = __devexit_p(lm75b_remove),
	.id_table = lm75b_id,
};

static ssize_t show_temp_max(struct device *dev, struct device_attribute *da,
			     char *buf)
{
	struct lm75b_data *data = lm75b_update_device(dev);
	return sprintf(buf, "%d\n",
		       LM75B_TOS_FROM_REG(data->temp[1]));
}

static ssize_t show_temp_max_hyst(struct device *dev,
				  struct device_attribute *da, char *buf)
{
	struct lm75b_data *data = lm75b_update_device(dev);
	return sprintf(buf, "%d\n",
		       LM75B_THYST_FROM_REG(data->temp[2]));
}

static ssize_t show_temp_input(struct device *dev,
			       struct device_attribute *da, char *buf)
{
	struct lm75b_data *data = lm75b_update_device(dev);
	return sprintf(buf, "%d\n",
		       LM75B_TEMP_FROM_REG(data->temp[0]));
}

static ssize_t set_temp_max(struct device *dev,
			    struct device_attribute *da,
			    const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct lm75b_data *data = i2c_get_clientdata(client);
	long temp = simple_strtol(buf, NULL, 10);

	mutex_lock(&data->update_lock);
	data->temp[1] = LM75B_TOS_TO_REG(temp);
	lm75b_write_value(client, LM75B_REG_TEMP[1], data->temp[1]);
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t set_temp_max_hyst(struct device *dev,
				 struct device_attribute *da,
				 const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct lm75b_data *data = i2c_get_clientdata(client);
	long temp = simple_strtol(buf, NULL, 10);

	mutex_lock(&data->update_lock);
	data->temp[2] = LM75B_THYST_TO_REG(temp);
	lm75b_write_value(client, LM75B_REG_TEMP[2], data->temp[2]);
	mutex_unlock(&data->update_lock);
	return count;
}

static DEVICE_ATTR(temp1_max, S_IWUSR | S_IRUGO, show_temp_max, set_temp_max);
static DEVICE_ATTR(temp1_max_hyst, S_IWUSR | S_IRUGO, show_temp_max_hyst,
		   set_temp_max_hyst);
static DEVICE_ATTR(temp1_input, S_IRUGO, show_temp_input, NULL);

static struct attribute *lm75b_attributes[] = {
	&dev_attr_temp1_input.attr,
	&dev_attr_temp1_max.attr,
	&dev_attr_temp1_max_hyst.attr,
	NULL
};

static const struct attribute_group lm75b_group = {
	.attrs = lm75b_attributes,
};

/* This function is called by i2c_probe */
static int lm75b_probe(struct i2c_client *client,
		       const struct i2c_device_id *id)
{
	struct lm75b_data *data;
	int err = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA |
				     I2C_FUNC_SMBUS_WORD_DATA)) {
		err = -ENODEV;
		goto exit;
	}

	if (!(data = kzalloc(sizeof(struct lm75b_data), GFP_KERNEL))) {
		err = -ENOMEM;
		goto exit;
	}

	data->client = client;
	data->valid = 0;

	i2c_set_clientdata(client, data);

	mutex_init(&data->update_lock);

	/* Initialize the LM75B chip */
	lm75b_init_client(client);
	
	/* Register sysfs hooks */
	if ((err = sysfs_create_group(&client->dev.kobj, &lm75b_group)))
		goto exit_free;

	return 0;

exit_free:
	kfree(data);
exit:
	return err;
}

static int lm75b_remove(struct i2c_client *client)
{
	struct lm75b_data *data = i2c_get_clientdata(client);

	sysfs_remove_group(&client->dev.kobj, &lm75b_group);

	kfree(data);

	return 0;
}

/* All registers are word-sized, except for the configuration register.
   LM75B uses a high-byte first convention, which is exactly opposite to
   the SMBus standard. */
static int lm75b_read_value(struct i2c_client *client, u8 reg)
{
	int value;

	if (reg == LM75B_REG_CONF)
		return i2c_smbus_read_byte_data(client, reg);

	value = i2c_smbus_read_word_data(client, reg);
	return (value < 0) ? value : swab16(value);
}

static int lm75b_write_value(struct i2c_client *client, u8 reg, u16 value)
{
	if (reg == LM75B_REG_CONF)
		return i2c_smbus_write_byte_data(client, reg, value);
	else
		return i2c_smbus_write_word_data(client, reg, swab16(value));
}

static void lm75b_init_client(struct i2c_client *client)
{
	int reg;

	/* Enable if in shutdown mode */
	reg = lm75b_read_value(client, LM75B_REG_CONF);
	if (reg >= 0 && (reg & 0x01))
		lm75b_write_value(client, LM75B_REG_CONF, reg & 0xfe);
}

static struct lm75b_data *lm75b_update_device(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct lm75b_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->update_lock);

	if (time_after(jiffies, data->last_updated + HZ + HZ / 2)
	    || !data->valid) {
		int i;
		dev_dbg(&client->dev, "Starting lm75b update\n");

		for (i = 0; i < ARRAY_SIZE(data->temp); i++) {
			int status;

			status = lm75b_read_value(client, LM75B_REG_TEMP[i]);
			if (status < 0)
				dev_dbg(&client->dev, "reg %d, err %d\n",
						LM75B_REG_TEMP[i], status);
			else
				data->temp[i] = status;
		}
		data->last_updated = jiffies;
		data->valid = 1;
	}

	mutex_unlock(&data->update_lock);

	return data;
}

static int __init sensors_lm75b_init(void)
{
	return i2c_add_driver(&lm75b_driver);
}

static void __exit sensors_lm75b_exit(void)
{
	i2c_del_driver(&lm75b_driver);
}

MODULE_AUTHOR("Tetsuhisa KOSEKI <koseki@atmark-techno.com>");
MODULE_DESCRIPTION("LM75B driver");
MODULE_LICENSE("GPL");

module_init(sensors_lm75b_init);
module_exit(sensors_lm75b_exit);
