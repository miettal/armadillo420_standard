/*
 *  Copyright 2008-2010 Atmark Techno, Inc. All Rights Reserved.
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
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/s353xxa.h>

#define DRIVER_NAME "rtc-s353xxa"
#define AUTHOR      "Atmark Techno, Inc."
#define DESCRIPTION "S-353XXA Real Time Clock class driver"

#undef DEBUG
#if defined(DEBUG)
#define DEBUG_FUNC()         printk(DRIVER_NAME ": %s()\n", __FUNCTION__)
#define DEBUG_INFO(args...)  printk(DRIVER_NAME ": " args)
#else
#define DEBUG_FUNC()
#define DEBUG_INFO(args...)
#endif
#define PRINT_INFO(args...)  printk(KERN_INFO  DRIVER_NAME ": " args)
#define PRINT_DEBUG(args...) printk(KERN_DEBUG DRIVER_NAME ": " args)
#define PRINT_ERR(args...)   printk(KERN_ERR   DRIVER_NAME ": " args)

#if defined(CONFIG_RTC_DRV_S353XXA_SYNC_ENABLE)
#define ENABLE_SYNC
#endif

static int devtype = 0;

#define REVERSE(x) ((((x) << 7) & 0x80) | (((x) << 5) & 0x40) | \
                    (((x) << 3) & 0x20) | (((x) << 1) & 0x10) | \
                    (((x) >> 1) & 0x08) | (((x) >> 3) & 0x04) | \
                    (((x) >> 5) & 0x02) | (((x) >> 7) & 0x01))

typedef struct s353xxa_private_data {
	struct rtc_device *rtc;
	u8 status1, status2;
} s353xxa_priv_t;

static s353xxa_priv_t s353xxa_priv;

static inline void
set_i2c_msg(struct i2c_msg *msg, u16 addr, u16 flags, u16 len, u8 *buf)
{
	msg->addr = addr;
	msg->flags = flags;
	msg->len = len;
	msg->buf = buf;
}

#if defined(ENABLE_SYNC)
static inline int s3531a_sync(struct i2c_client *client)
{
	struct i2c_msg msgs[1];
	u8 buf[3];
	u8 sec;
	u32 end = jiffies + HZ * 2;
	int ret;

	set_i2c_msg(&msgs[0], client->addr | 0x3, I2C_M_RD, 3, buf);
	ret = i2c_transfer(client->adapter, msgs, 1);
	sec = buf[2];

	while ((ret == 1) && (jiffies < end)) {
		if (sec != buf[2])
			return 0;

		ret = i2c_transfer(client->adapter, msgs, 1);
	}

	return -EIO;
}
#else
#define s3531a_sync(client) (0)
#endif

static int s353xxa_rtc_open(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	s353xxa_priv_t *priv = i2c_get_clientdata(client);
	struct i2c_msg msgs[1];
	u8 buf[3];
	int ret;

#if defined(ENABLE_SYNC)
	if (s3531a_sync(client)) {
		DEBUG_INFO("Device stopped.\n");
		msgs[0].addr = client->addr | 0x0;
		msgs[0].flags = I2C_M_RD;
		msgs[0].len = 1;
		if (i2c_transfer(client->adapter, msgs, 1) == 1) {
			msgs[0].flags = 0;
			buf[0] |= S353X0A_STATUS1_RESET;
			if (i2c_transfer(client->adapter, msgs, 1) == 1) {
				DEBUG_INFO("Device Reset.\n");
			}
		}
		if (!devtype) {
			DEBUG_INFO("Device Reset.\n");
		}
	} else
#endif
		devtype = DEVTYPE_S3531A;

	set_i2c_msg(&msgs[0], client->addr | 0x1, I2C_M_RD, 1, buf);
	ret = i2c_transfer(client->adapter, msgs, 1);
	if (ret != 1)
		goto err_i2c_tx;

	buf[0] &= (S353X0A_STATUS2_INT2ME | S353X0A_STATUS2_32KE |
		    S353X0A_STATUS2_INT1ME | S353X0A_STATUS2_INT1FE);
	buf[0] |= S353X0A_STATUS2_INT1AE;
	set_i2c_msg(&msgs[0], client->addr | 0x1, 0, 1, buf);
	ret = i2c_transfer(client->adapter, msgs, 1);
	if (ret != 1)
		goto err_i2c_tx;

	set_i2c_msg(&msgs[0], client->addr | 0x1, I2C_M_RD, 1, buf);
	ret = i2c_transfer(client->adapter, msgs, 1);
	if (ret != 1)
		goto err_i2c_tx;

	if (!(buf[0] & (S353X0A_STATUS2_INT2ME | S353X0A_STATUS2_32KE |
			S353X0A_STATUS2_INT1ME | S353X0A_STATUS2_INT1FE)) &&
	    (buf[0] & S353X0A_STATUS2_INT1AE)) {
		priv->status2 = buf[0];
		buf[0] = buf[1] = buf[2] = 0;
		set_i2c_msg(&msgs[0], client->addr | 0x4, 0, 3, buf);
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret == 1) {
			buf[0] = buf[1] = buf[2] = 0xff;
			set_i2c_msg(&msgs[0], client->addr | 0x4, I2C_M_RD, 3, buf);
			ret = i2c_transfer(client->adapter, msgs, 1);
			if (ret == 1) {
				if (!buf[0] && !buf[1] && !buf[2]) {
					devtype = DEVTYPE_S353X0A;
				}
			}
		}
		buf[0] = priv->status2;
	}
	if (devtype == DEVTYPE_S3531A) {
		buf[0] &= (STATUS_INT1AE | STATUS_INT1ME | STATUS_INT1FE);
		buf[0] |= STATUS_12_24;
	} else if (devtype == DEVTYPE_S353X0A) {
		buf[0] &= (S353X0A_STATUS2_INT2AE | S353X0A_STATUS2_INT2ME |
			    S353X0A_STATUS2_INT2FE | S353X0A_STATUS2_32KE |
			    S353X0A_STATUS2_INT1AE | S353X0A_STATUS2_INT1ME |
			    S353X0A_STATUS2_INT1FE);
	}
	set_i2c_msg(&msgs[0], client->addr | 0x1, 0, 1, buf);
	ret = i2c_transfer(client->adapter, msgs, 1);
	if (ret != 1)
		goto err_i2c_tx;

	set_i2c_msg(&msgs[0], client->addr | 0x1, I2C_M_RD, 1, buf);
	ret = i2c_transfer(client->adapter, msgs, 1);
	if (ret != 1)
		goto err_i2c_tx;

	priv->status2 = buf[0];
	if (devtype == DEVTYPE_S353X0A) {
		set_i2c_msg(&msgs[0], client->addr | 0x0, I2C_M_RD, 1, buf);
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret != 1)
			goto err_i2c_tx;

		if ((buf[0] & (S353X0A_STATUS1_POC | S353X0A_STATUS1_BLD)) ||
		    (priv->status2 & S353X0A_STATUS2_TEST)) {
			buf[0] |= S353X0A_STATUS1_RESET;
			set_i2c_msg(&msgs[0], client->addr | 0x0, 0, 1, buf);
			ret = i2c_transfer(client->adapter, msgs, 1);
			if (ret != 1)
				goto err_i2c_tx;

			DEBUG_INFO("Device Reset.\n");

			set_i2c_msg(&msgs[0], client->addr | 0x0, I2C_M_RD, 1, buf);
			ret = i2c_transfer(client->adapter, msgs, 1);
			if (ret != 1)
				goto err_i2c_tx;
		}
		priv->status1 = buf[0];
		if (priv->status2 & S353X0A_STATUS2_TEST) {
			buf[0] = priv->status2 & S353X0A_STATUS2_TEST;
			set_i2c_msg(&msgs[0], client->addr | 0x1, 0, 1, buf);
			ret = i2c_transfer(client->adapter, msgs, 1);
			if (ret != 1)
				goto err_i2c_tx;

			set_i2c_msg(&msgs[0], client->addr | 0x1, I2C_M_RD, 1, buf);
			ret = i2c_transfer(client->adapter, msgs, 1);
			if (ret != 1)
				goto err_i2c_tx;

			priv->status2 = buf[0];
		}
		if (!(priv->status1 & S353X0A_STATUS1_12_24)) {
			buf[0] = priv->status1 | S353X0A_STATUS1_12_24;
			set_i2c_msg(&msgs[0], client->addr | 0x0, 0, 1, buf);
			ret = i2c_transfer(client->adapter, msgs, 1);
			if (ret != 1)
				goto err_i2c_tx;

			set_i2c_msg(&msgs[0], client->addr | 0x0, I2C_M_RD, 1, buf);
			ret = i2c_transfer(client->adapter, msgs, 1);
			if (ret != 1)
				goto err_i2c_tx;

			priv->status1 = buf[0];
		}
	}
	DEBUG_INFO("Device Type [%s]\n",
		   (devtype == DEVTYPE_S3531A) ? "S-3531A" : "S-353x0A");

	return 0;

err_i2c_tx:
	return -EIO;
}

static int s353xxa_rtc_read_time(struct device *dev, struct rtc_time *t)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct i2c_msg msgs[1];
	u8 buf[7];
	int ret;

	DEBUG_FUNC();

	set_i2c_msg(&msgs[0], client->addr | 0x2, I2C_M_RD, 7, buf);
	ret = i2c_transfer(client->adapter, msgs, 1);
	if (ret != 1)
		return -EIO;

	t->tm_year = BCD2BIN(REVERSE(buf[0]));
	t->tm_mon = BCD2BIN(REVERSE(buf[1]));
	t->tm_mday = BCD2BIN(REVERSE(buf[2]));
	t->tm_wday = BCD2BIN(REVERSE(buf[3]));
	t->tm_hour = BCD2BIN(REVERSE(buf[4]) & 0x3f);
	t->tm_min = BCD2BIN(REVERSE(buf[5]));
	t->tm_sec = BCD2BIN(REVERSE(buf[6]));

	t->tm_year += 100;
	t->tm_mon -= 1;

	DEBUG_INFO("READ: %d/%d/%d(%d) %d:%d:%d\n",
		   t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_wday,
		   t->tm_hour, t->tm_min, t->tm_sec);

	return 0;
}

static int s353xxa_rtc_set_time(struct device *dev, struct rtc_time *t)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct i2c_msg msgs[1];
	u8 buf[7];
	int ret;

	DEBUG_FUNC();

	DEBUG_INFO("SET : %d/%d/%d(%d) %d:%d:%d\n",
		   t->tm_year + 2000, t->tm_mon + 1, t->tm_mday, t->tm_wday,
		   t->tm_hour, t->tm_min, t->tm_sec);

	t->tm_year -= 100;
	t->tm_mon += 1;

	buf[0] = REVERSE(BIN2BCD(t->tm_year));
	buf[1] = REVERSE(BIN2BCD(t->tm_mon));
	buf[2] = REVERSE(BIN2BCD(t->tm_mday));
	buf[3] = REVERSE(BIN2BCD(t->tm_wday));
	buf[4] = REVERSE(BIN2BCD(t->tm_hour));
	buf[5] = REVERSE(BIN2BCD(t->tm_min));
	buf[6] = REVERSE(BIN2BCD(t->tm_sec));

	set_i2c_msg(&msgs[0], client->addr | 0x2, 0, 7, buf);
	ret = i2c_transfer(client->adapter, msgs, 1);
	if (ret != 1) {
		return -EIO;
	}

	return 0;
}

static struct rtc_class_ops s353xxa_rtc_ops = {
	.open = s353xxa_rtc_open,
	.read_time = s353xxa_rtc_read_time,
	.set_time = s353xxa_rtc_set_time,
};

static int s353xxa_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	s353xxa_priv_t *priv = &s353xxa_priv;
	struct i2c_msg msgs[1];
	u8 buf[3];
	int ret;

	memset(priv, 0, sizeof(s353xxa_priv_t));
	i2c_set_clientdata(client, priv);

	set_i2c_msg(&msgs[0], client->addr | 0x0, I2C_M_RD, 1, buf);
	ret = i2c_transfer(client->adapter, msgs, 1);
	if (ret != 1)
		return -ENODEV;

	priv->rtc = rtc_device_register(client->name, &client->dev,
					&s353xxa_rtc_ops, THIS_MODULE);
	if (IS_ERR(priv->rtc)) {
		PRINT_ERR("unable to register rtc clas device\n");
		goto err_client_free;
	}

	return 0;

      err_client_free:
	PRINT_ERR("I2C Error.\n");
	kfree(client);

	return -EIO;
}

static int s353xxa_remove(struct i2c_client *client)
{
	s353xxa_priv_t *priv = i2c_get_clientdata(client);

	DEBUG_FUNC();

	if (priv->rtc)
		rtc_device_unregister(priv->rtc);

	kfree(client);

	return 0;
}

static const struct i2c_device_id s353xxa_ids[] = {
	{DRIVER_NAME, 0},
	{},
};

static struct i2c_driver s353xxa_driver = {
	.driver = {
		   .name = DRIVER_NAME,
		   },
	.probe = s353xxa_probe,
	.remove = s353xxa_remove,
	.id_table = s353xxa_ids,
};

static int __init rtc_s353xxa_init(void)
{
	int ret;

	DEBUG_FUNC();

	ret = i2c_add_driver(&s353xxa_driver);
	if (ret) {
		PRINT_ERR("Unable to register %s driver.\n", DRIVER_NAME);
		return ret;
	}

	PRINT_INFO(DESCRIPTION ", (C) 2008 " AUTHOR "\n");

	return 0;
}

static void __exit rtc_s353xxa_exit(void)
{
	DEBUG_FUNC();
	i2c_del_driver(&s353xxa_driver);
}

module_init(rtc_s353xxa_init);
module_exit(rtc_s353xxa_exit);

MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESCRIPTION);
MODULE_LICENSE("GPL");
