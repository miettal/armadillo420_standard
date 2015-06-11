/*
 * Copyright (C) Atmark Techno, Inc. All Rights Reserved.
 *
 * Based on:
 * Copyright (C) 2012 Avionic Design GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/ti-adc081c.h>

#include <asm/byteorder.h>

#define REG_CONV_RES	(0x00)
#define REG_ALERT_STAT	(0x01)
#define REG_CONFIG	(0x02)
#define REG_LOW_LIM	(0x03)
#define REG_HIGH_LIM	(0x04)
#define REG_HYSTERESIS	(0x05)
#define REG_LOW_CONV	(0x06)
#define REG_HIGH_CONV	(0x07)

#define VREF_MIN_MV	(2700)
#define VREF_MAX_MV	(5500)
#define VREF_DEFAULT_MV	(3300)

#define ADC081C_ALERT_STATE_OVER_RANGE	(1<<1)
#define ADC081C_ALERT_STATE_UNDER_RANGE	(1<<0)

#define ADC081C_CONFIG_CYCLE_32		(0x20)
#define ADC081C_CONFIG_CYCLE_64		(0x40)
#define ADC081C_CONFIG_CYCLE_128	(0x60)
#define ADC081C_CONFIG_CYCLE_256	(0x80)
#define ADC081C_CONFIG_CYCLE_512	(0xa0)
#define ADC081C_CONFIG_CYCLE_1024	(0xc0)
#define ADC081C_CONFIG_CYCLE_2048	(0xe0)
#define ADC081C_CONFIG_CYCLE_MASK	(0xe0)
#define ADC081C_CONFIG_ALERT_HOLD	(1<<4)
#define ADC081C_CONFIG_ALERT_FLAG_EN	(1<<3)
#define ADC081C_CONFIG_ALERT_PIN_EN	(1<<2)
#define ADC081C_CONFIG_POLARITY		(1<<0)

#define DIV_ROUND_CLOSEST(x, divisor)(			\
{							\
	typeof(divisor) __divisor = divisor;		\
	(((x) + ((__divisor) / 2)) / (__divisor));	\
}							\
)

struct adc081c {
	struct i2c_client *i2c;
};

static int adc081c_vref_mv = VREF_DEFAULT_MV;
static unsigned int adc081c_lsb_resol; /* resolution of the ADC sample lsb */

static inline int MV_TO_REG(long mv)
{
	int reg;

	reg = DIV_ROUND_CLOSEST(mv * 1024, adc081c_lsb_resol);

	if (reg > 255)
		reg = 255;
	if (reg < 0)
		reg = 0;

	return reg;
}

static inline int REG_TO_MV(int reg)
{
	return DIV_ROUND_CLOSEST(reg * adc081c_lsb_resol, 1024);
}

static int adc081c_readb(struct i2c_client *client, u8 reg)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0)
		return ret;

	return ret & 0xff;
}

static int adc081c_writeb(struct i2c_client *client, u8 val, u8 reg)
{
	return i2c_smbus_write_byte_data(client, reg, val);
}

static int adc081c_readw(struct i2c_client *client, u8 reg)
{
	int ret;

	ret = i2c_smbus_read_word_data(client, reg);
	if (ret < 0)
		return ret;

	return (swab16(ret) >> 4) & 0xff;
}

static int adc081c_writew(struct i2c_client *client, u16 val, u8 reg)
{
	return i2c_smbus_write_word_data(client, reg, swab16(val << 4));
}

static int __maybe_unused
adc081c_get_conversion_result(struct i2c_client *client)
{
	return adc081c_readw(client, REG_CONV_RES);
}

static int __maybe_unused
adc081c_get_alert_status(struct i2c_client *client)
{
	int ret;

	ret = adc081c_readb(client, REG_ALERT_STAT);
	if (ret < 0)
		return ret;

	return ret & 0x03;
}

static int __maybe_unused
adc081c_clear_alert_status(struct i2c_client *client, u8 alert)
{
	return adc081c_writeb(client, alert, REG_ALERT_STAT);
}

static int __maybe_unused
adc081c_get_configuration(struct i2c_client *client)
{
	int ret;

	ret = adc081c_readb(client, REG_CONFIG);
	if (ret < 0)
		return ret;

	return ret & 0xfd;
}

static int __maybe_unused
adc081c_set_configuration(struct i2c_client *client, u8 config)
{
	return adc081c_writeb(client, config, REG_CONFIG);
}

static int __maybe_unused
adc081c_get_alert_low_limit(struct i2c_client *client)
{
	return adc081c_readw(client, REG_LOW_LIM);
}

static int __maybe_unused
adc081c_set_alert_low_limit(struct i2c_client *client, u8 limit)
{
	return adc081c_writew(client, limit, REG_LOW_LIM);
}

static int __maybe_unused
adc081c_get_alert_high_limit(struct i2c_client *client)
{
	return adc081c_readw(client, REG_HIGH_LIM);
}

static int __maybe_unused
adc081c_set_alert_high_limit(struct i2c_client *client, u8 limit)
{
	return adc081c_writew(client, limit, REG_HIGH_LIM);
}

static int __maybe_unused
adc081c_get_alert_hysteresis(struct i2c_client *client)
{
	return adc081c_readw(client, REG_HYSTERESIS);
}

static int __maybe_unused
adc081c_set_alert_hysteresis(struct i2c_client *client, u8 hyst)
{
	return adc081c_writew(client, hyst, REG_HYSTERESIS);
}

static int __maybe_unused
adc081c_get_lowest_conversion(struct i2c_client *client)
{
	return adc081c_readw(client, REG_LOW_CONV);
}

static int __maybe_unused
adc081c_set_lowest_conversion(struct i2c_client *client, u8 conv)
{
	return adc081c_writew(client, conv, REG_LOW_CONV);
}

static int __maybe_unused
adc081c_get_highest_conversion(struct i2c_client *client)
{
	return adc081c_readw(client, REG_HIGH_CONV);
}

static int __maybe_unused
adc081c_set_highest_conversion(struct i2c_client *client, u8 conv)
{
	return adc081c_writew(client, conv, REG_HIGH_CONV);
}

static ssize_t adc081c_show_value(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int ret;
	struct i2c_client *client = to_i2c_client(dev);

	ret = adc081c_get_conversion_result(client);
	if (ret < 0)
		return ret;

	return sprintf(buf, "%u\n", REG_TO_MV(ret));
}

static ssize_t adc081c_show_high_limit(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	int ret;
	struct i2c_client *client = to_i2c_client(dev);

	ret = adc081c_get_alert_high_limit(client);
	if (ret < 0)
		return ret;

	return sprintf(buf, "%u\n", REG_TO_MV(ret));
}

static ssize_t adc081c_show_low_limit(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	int ret;
	struct i2c_client *client = to_i2c_client(dev);

	ret = adc081c_get_alert_low_limit(client);
	if (ret < 0)
		return ret;

	return sprintf(buf, "%u\n", REG_TO_MV(ret));
}

static ssize_t adc081c_show_hysteresis(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	int ret;
	struct i2c_client *client = to_i2c_client(dev);

	ret = adc081c_get_alert_hysteresis(client);
	if (ret < 0)
		return ret;

	return sprintf(buf, "%u\n", REG_TO_MV(ret));
}

static ssize_t adc081c_show_configuration(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	int ret;
	struct i2c_client *client = to_i2c_client(dev);

	ret = adc081c_get_configuration(client);
	if (ret < 0)
		return ret;

	return sprintf(buf, "%u\n", ret);
}

static ssize_t adc081c_set_high_limit(struct device *dev,
				      struct device_attribute *da,
				      const char *buf, size_t count)
{
	long mv;
	int reg;
	struct i2c_client *client = to_i2c_client(dev);

	mv = simple_strtol(buf, NULL, 10);
	reg = MV_TO_REG(mv);

	adc081c_set_alert_high_limit(client, reg);

	return count;
}

static ssize_t adc081c_set_low_limit(struct device *dev,
				     struct device_attribute *da,
				     const char *buf, size_t count)
{
	long mv;
	int reg;
	struct i2c_client *client = to_i2c_client(dev);

	mv = simple_strtol(buf, NULL, 10);
	reg = MV_TO_REG(mv);

	adc081c_set_alert_low_limit(client, reg);

	return count;
}

static ssize_t adc081c_set_hysteresis(struct device *dev,
				      struct device_attribute *da,
				      const char *buf, size_t count)
{
	long mv;
	int reg;
	struct i2c_client *client = to_i2c_client(dev);

	mv = simple_strtol(buf, NULL, 10);
	reg = MV_TO_REG(mv);

	adc081c_set_alert_hysteresis(client, reg);

	return count;
}

static DEVICE_ATTR(value, S_IRUGO, adc081c_show_value, NULL);
static DEVICE_ATTR(high_limit, S_IWUSR | S_IRUGO,
		   adc081c_show_high_limit, adc081c_set_high_limit);
static DEVICE_ATTR(low_limit, S_IWUSR | S_IRUGO,
		   adc081c_show_low_limit, adc081c_set_low_limit);
static DEVICE_ATTR(hysteresis, S_IWUSR | S_IRUGO,
		   adc081c_show_hysteresis, adc081c_set_hysteresis);
static DEVICE_ATTR(config, S_IRUGO, adc081c_show_configuration, NULL);

static struct attribute *adc081c_attributes[] = {
	&dev_attr_value.attr,
	&dev_attr_high_limit.attr,
	&dev_attr_low_limit.attr,
	&dev_attr_hysteresis.attr,
	&dev_attr_config.attr,
	NULL
};

static const struct attribute_group adc081c_attr_group = {
	.attrs = adc081c_attributes,
};

static void adc081c_init_client(struct i2c_client *client)
{
	int reg;
	struct adc081c_platform_data *plat_data = client->dev.platform_data;

	reg = adc081c_get_configuration(client);
	if (reg >= 0) {
		reg &= ~0xE0;
		reg |= ADC081C_CONFIG_ALERT_PIN_EN | ADC081C_CONFIG_CYCLE_256;
		adc081c_set_configuration(client, reg);
	}

	if ((plat_data->vref_mv <= VREF_MAX_MV) &&
	    (plat_data->vref_mv >= VREF_MIN_MV))
		adc081c_vref_mv = plat_data->vref_mv;
	else
		dev_warn(&client->dev, "Out of range voltage reference \n");

	/* Calculate the LSB resolution */
	adc081c_lsb_resol = (adc081c_vref_mv * 1024) / 256;
}

static int __devinit adc081c_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	struct adc081c *adc081c_data;
	int err;

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_WORD_DATA))
		return -ENODEV;

	adc081c_data = kzalloc(sizeof(struct adc081c), GFP_KERNEL);
	if (!adc081c_data) {
		return -ENOMEM;
	}

	adc081c_data->i2c = client;

	i2c_set_clientdata(client, adc081c_data);

	adc081c_init_client(client);

	/* Register sysfs hooks */
	err = sysfs_create_group(&client->dev.kobj, &adc081c_attr_group);
	if (err)
		goto kfree_adc;

	dev_info(&client->dev, "ADC081C021/027 driver probed\n");

	return 0;

kfree_adc:
	kfree(adc081c_data);

	return err;
}

static int __devexit adc081c_remove(struct i2c_client *client)
{

	sysfs_remove_group(&client->dev.kobj, &adc081c_attr_group);

	kfree(i2c_get_clientdata(client));

	return 0;
}

static const struct i2c_device_id adc081c_id[] = {
	{ "adc081c", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, adc081c_id);

static struct i2c_driver adc081c_driver = {
	.driver = {
		.name = "adc081c",
		.owner = THIS_MODULE,
	},
	.probe = adc081c_probe,
	.remove = __devexit_p(adc081c_remove),
	.id_table = adc081c_id,
};

static int __init adc081c_init(void)
{
	return i2c_add_driver(&adc081c_driver);
}

static void __exit adc081c_exit(void)
{
	i2c_del_driver(&adc081c_driver);
}

MODULE_AUTHOR("Tetsuhisa KOSEKI <koseki@atmark-techno.com>");
MODULE_DESCRIPTION("Texas Instruments ADC081C021/027 driver");
MODULE_LICENSE("GPL v2");

module_init(adc081c_init);
module_exit(adc081c_exit);
