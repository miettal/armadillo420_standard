/*
 * Seiko Instruments S-35390A RTC Driver
 *
 * Copyright (c) 2007 Byron Bradley
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/rtc.h>
#include <linux/i2c.h>
#include <linux/bitrev.h>
#include <linux/bcd.h>
#include <linux/slab.h>
#include <linux/rtc/rtc-s35390a.h>

#define S35390A_CMD_STATUS1	0
#define S35390A_CMD_STATUS2	1
#define S35390A_CMD_TIME1	2
#define S35390A_CMD_INT1	4
#define S35390A_CMD_INT2	5

#define S35390A_BYTE_YEAR	0
#define S35390A_BYTE_MONTH	1
#define S35390A_BYTE_DAY	2
#define S35390A_BYTE_WDAY	3
#define S35390A_BYTE_HOURS	4
#define S35390A_BYTE_MINS	5
#define S35390A_BYTE_SECS	6

#define S35390A_INT_BYTE_WDAY	0
#define S35390A_INT_BYTE_HOURS	1
#define S35390A_INT_BYTE_MINS	2

#define S35390A_FLAG_POC	0x01
#define S35390A_FLAG_BLD	0x02
#define S35390A_FLAG_INT2	0x04
#define S35390A_FLAG_INT1	0x08
#define S35390A_FLAG_24H	0x40
#define S35390A_FLAG_RESET	0x80

#define S35390A_FLAG_TEST	0x01
#define S35390A_FLAG_INT2AE	0x02
#define S35390A_FLAG_INT2ME	0x04
#define S35390A_FLAG_INT2FE	0x08
#define S35390A_FLAG_32KE	0x10
#define S35390A_FLAG_INT1AE	0x20
#define S35390A_FLAG_INT1ME	0x40
#define S35390A_FLAG_INT1FE	0x80

#define S35390A_ALARM_IRQ_IS_AVAILABLE(s35390a) ((s35390a)->alarm_irq < NR_IRQS)

static const struct i2c_device_id s35390a_id[] = {
	{ "s35390a", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, s35390a_id);

struct s35390a {
	struct i2c_client *client[8];
	struct rtc_device *rtc;
	unsigned int alarm_irq;
	struct rtc_time alrm_time;
	struct work_struct interrupt_work;
	struct mutex irq_lock;
	unsigned twentyfourhour: 1;
	unsigned alarm_enabled: 1;
	unsigned time_is_invalid: 1;
};

static int s35390a_rtc_set_alarm_internal(struct s35390a *s35390a);
static int s35390a_rtc_read_alarm_internal(struct s35390a *s35390a);

static int s35390a_set_reg(struct s35390a *s35390a, int reg, char *buf, int len)
{
	struct i2c_client *client = s35390a->client[reg];
	struct i2c_msg msg[] = {
		{ client->addr, 0, len, buf },
	};

	if ((i2c_transfer(client->adapter, msg, 1)) != 1)
		return -EIO;

	return 0;
}

static int s35390a_get_reg(struct s35390a *s35390a, int reg, char *buf, int len)
{
	struct i2c_client *client = s35390a->client[reg];
	struct i2c_msg msg[] = {
		{ client->addr, I2C_M_RD, len, buf },
	};

	if ((i2c_transfer(client->adapter, msg, 1)) != 1)
		return -EIO;

	return 0;
}

static int s35390a_reset(struct s35390a *s35390a)
{
	char buf[1];

	if (s35390a_get_reg(s35390a, S35390A_CMD_STATUS1, buf, sizeof(buf)) < 0)
		return -EIO;

	if (!(buf[0] & (S35390A_FLAG_POC | S35390A_FLAG_BLD)))
		return 0;

	/*
	 *S35390A sets internal timer to 00:00:00 Jan 1, 2000 at
	 *reset. and also, the day of the week is set to 0. S35390A
	 *does not enforce any policy on how the value is used.
	 *
	 * However, Linux kernel is using 0 as Sunday. And that
	 * conflicts with the actual day of Jan 1, 2000, which is
	 * Saturday or "6" in the Linux way of expression.
	 *
	 * Alarm function of S35390A requires to match not only date
	 * and time but also the days of the week.  So, if we do not
	 * initialize the day of the week to the right value, the
	 * alarm never goes off.  we have changed the code so that
	 * once the RTC is reset, the alarm functionality is
	 * disabled. if user want to use the functionality, he must
	 * re-initialize the internal timer beforehand.
	 */
	s35390a->time_is_invalid = 1;

	buf[0] |= (S35390A_FLAG_RESET | S35390A_FLAG_24H);
	buf[0] &= 0xf0;
	return s35390a_set_reg(s35390a, S35390A_CMD_STATUS1, buf, sizeof(buf));
}

static int s35390a_disable_test_mode(struct s35390a *s35390a)
{
	char buf[1];

	if (s35390a_get_reg(s35390a, S35390A_CMD_STATUS2, buf, sizeof(buf)) < 0)
		return -EIO;

	if (!(buf[0] & S35390A_FLAG_TEST))
		return 0;

	buf[0] &= ~S35390A_FLAG_TEST;
	return s35390a_set_reg(s35390a, S35390A_CMD_STATUS2, buf, sizeof(buf));
}

static char s35390a_hr2reg(struct s35390a *s35390a, int hour)
{
	if (hour < 12)
		return BIN2BCD(hour);

	if (!s35390a->twentyfourhour)
		hour -= 12;

	return 0x40 | BIN2BCD(hour);
}

static int s35390a_reg2hr(struct s35390a *s35390a, char reg)
{
	unsigned hour;

	if (s35390a->twentyfourhour)
		return BCD2BIN(reg & 0x3f);

	hour = BCD2BIN(reg & 0x3f);
	if (reg & 0x40)
		hour += 12;

	return hour;
}

static int s35390a_is_alarm_mode(struct s35390a *s35390a)
{
	char buf[1];
	int err;

	if (!S35390A_ALARM_IRQ_IS_AVAILABLE(s35390a))
		return 0;

	err = s35390a_get_reg(s35390a, S35390A_CMD_STATUS2, buf, 1);
	if (err < 0)
		return 0;

	if ((buf[0] & (S35390A_FLAG_32KE | S35390A_FLAG_INT1ME | S35390A_FLAG_INT1FE | S35390A_FLAG_INT1AE)) ==
		S35390A_FLAG_INT1AE)
		return 1;
	else
		return 0;
}

static int s35390a_enable_alarm_mode(struct s35390a *s35390a)
{
	char buf[1];
	int err;

	if (!S35390A_ALARM_IRQ_IS_AVAILABLE(s35390a))
		return 0;

	err = s35390a_get_reg(s35390a, S35390A_CMD_STATUS2, buf, 1);
	if (err < 0)
		return -EIO;

	buf[0] &= ~(S35390A_FLAG_32KE |
		    S35390A_FLAG_INT1ME | S35390A_FLAG_INT1FE |
		    S35390A_FLAG_INT2ME | S35390A_FLAG_INT2FE);
	buf[0] |= (S35390A_FLAG_INT1AE | S35390A_FLAG_INT2AE);

	err = s35390a_set_reg(s35390a, S35390A_CMD_STATUS2, buf, 1);
	if (err < 0)
		return -EIO;

	return 0;
}

static int s35390a_disable_alarm_mode(struct s35390a *s35390a)
{
	char buf[1];
	int err;

	if (!S35390A_ALARM_IRQ_IS_AVAILABLE(s35390a))
		return 0;

	err = s35390a_get_reg(s35390a, S35390A_CMD_STATUS2, buf, 1);
	if (err < 0)
		return -EIO;

	buf[0] &= ~(S35390A_FLAG_32KE |
		    S35390A_FLAG_INT1ME | S35390A_FLAG_INT1FE | S35390A_FLAG_INT1AE |
		    S35390A_FLAG_INT2ME | S35390A_FLAG_INT2FE | S35390A_FLAG_INT2AE);

	err = s35390a_set_reg(s35390a, S35390A_CMD_STATUS2, buf, 1);
	if (err < 0)
		return -EIO;

	return 0;
}

static int s35390a_enable_alarm_irq_unlocked(struct s35390a *s35390a)
{
	int ret;

	if (!S35390A_ALARM_IRQ_IS_AVAILABLE(s35390a))
		return 0;

	if (s35390a->alarm_enabled)
		return 0;

	ret = s35390a_rtc_set_alarm_internal(s35390a);
	if (ret)
		return ret;

	enable_irq(s35390a->alarm_irq);
	s35390a->alarm_enabled = 1;

	return 0;
}

static int s35390a_enable_alarm_irq(struct s35390a *s35390a)
{
	int ret = 0;

	mutex_lock(&s35390a->irq_lock);
	ret = s35390a_enable_alarm_irq_unlocked(s35390a);
	mutex_unlock(&s35390a->irq_lock);

	return ret;
}

static int s35390a_disable_alarm_irq(struct s35390a *s35390a)
{
	int ret = 0;

	if (!S35390A_ALARM_IRQ_IS_AVAILABLE(s35390a))
		return 0;

	mutex_lock(&s35390a->irq_lock);

	if (!s35390a->alarm_enabled)
		goto done;

	if (s35390a_is_alarm_mode(s35390a)) {
		ret = s35390a_disable_alarm_mode(s35390a);
		if (ret)
			goto done;
	}

	disable_irq(s35390a->alarm_irq);
	s35390a->alarm_enabled = 0;

done:
	mutex_unlock(&s35390a->irq_lock);

	return ret;
}

static int s35390a_set_datetime(struct i2c_client *client, struct rtc_time *tm)
{
	struct s35390a	*s35390a = i2c_get_clientdata(client);
	int i, err;
	char buf[7];

	dev_dbg(&client->dev, "%s: tm is secs=%d, mins=%d, hours=%d mday=%d, "
		"mon=%d, year=%d, wday=%d\n", __func__, tm->tm_sec,
		tm->tm_min, tm->tm_hour, tm->tm_mday, tm->tm_mon, tm->tm_year,
		tm->tm_wday);

	buf[S35390A_BYTE_YEAR] = BIN2BCD(tm->tm_year - 100);
	buf[S35390A_BYTE_MONTH] = BIN2BCD(tm->tm_mon + 1);
	buf[S35390A_BYTE_DAY] = BIN2BCD(tm->tm_mday);
	buf[S35390A_BYTE_WDAY] = BIN2BCD(tm->tm_wday);
	buf[S35390A_BYTE_HOURS] = s35390a_hr2reg(s35390a, tm->tm_hour);
	buf[S35390A_BYTE_MINS] = BIN2BCD(tm->tm_min);
	buf[S35390A_BYTE_SECS] = BIN2BCD(tm->tm_sec);

	/* This chip expects the bits of each byte to be in reverse order */
	for (i = 0; i < 7; ++i)
		buf[i] = bitrev8(buf[i]);

	err = s35390a_set_reg(s35390a, S35390A_CMD_TIME1, buf, sizeof(buf));

	return err;
}

static int s35390a_get_datetime(struct i2c_client *client, struct rtc_time *tm)
{
	struct s35390a *s35390a = i2c_get_clientdata(client);
	char buf[7];
	int i, err;

	err = s35390a_get_reg(s35390a, S35390A_CMD_TIME1, buf, sizeof(buf));
	if (err < 0)
		return err;

	dev_dbg(&client->dev, "%s: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
		__func__, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);

	/* This chip returns the bits of each byte in reverse order */
	for (i = 0; i < 7; ++i)
		buf[i] = bitrev8(buf[i]);

	tm->tm_sec = BCD2BIN(buf[S35390A_BYTE_SECS]);
	tm->tm_min = BCD2BIN(buf[S35390A_BYTE_MINS]);
	tm->tm_hour = s35390a_reg2hr(s35390a, buf[S35390A_BYTE_HOURS]);
	tm->tm_wday = BCD2BIN(buf[S35390A_BYTE_WDAY]);
	tm->tm_mday = BCD2BIN(buf[S35390A_BYTE_DAY]);
	tm->tm_mon = BCD2BIN(buf[S35390A_BYTE_MONTH]) - 1;
	tm->tm_year = BCD2BIN(buf[S35390A_BYTE_YEAR]) + 100;

	dev_dbg(&client->dev, "%s: tm is secs=%d, mins=%d, hours=%d, mday=%d, "
		"mon=%d, year=%d, wday=%d\n", __func__, tm->tm_sec,
		tm->tm_min, tm->tm_hour, tm->tm_mday, tm->tm_mon, tm->tm_year,
		tm->tm_wday);

	return rtc_valid_tm(tm);
}

static int s35390a_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	return s35390a_get_datetime(to_i2c_client(dev), tm);
}

static int s35390a_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct s35390a	*s35390a = i2c_get_clientdata(client);
	int ret;

	ret =  s35390a_set_datetime(client, tm);

	if (s35390a->time_is_invalid && ret == 0)
		s35390a->time_is_invalid = 0;

	return ret;
}

static irqreturn_t s35390a_interrupt_handler(int irq, void *data)
{
	struct s35390a *s35390a = data;

	schedule_work(&s35390a->interrupt_work);

	return IRQ_HANDLED;
}

static void s35390a_interrupt_work(struct work_struct *work)
{
	struct s35390a *s35390a;
	unsigned long events = 0;
	char buf[1];
	int  err;

	s35390a = container_of(work, struct s35390a, interrupt_work);

	err = s35390a_get_reg(s35390a, S35390A_CMD_STATUS1, buf, 1);
	if (err < 0)
		return;

	if (buf[0] & (S35390A_FLAG_INT1 | S35390A_FLAG_INT2)) {
		events |= RTC_AF | RTC_IRQF;
		rtc_update_irq(s35390a->rtc, 1, events);
		s35390a_disable_alarm_irq(s35390a);
	}
}

static int s35390a_rtc_read_alarm_internal(struct s35390a *s35390a)
{
	struct rtc_time *tm = &s35390a->alrm_time;
	char buf[3];
	int  i, err;

	/* The S35390A is not be capable of secs/day/month/year fields for alarm. */
	tm->tm_sec = tm->tm_mday = tm->tm_mon = tm->tm_year = -1;

	if (!s35390a_is_alarm_mode(s35390a)) {
		tm->tm_min = tm->tm_hour = tm->tm_wday = -1;
		return 0;
	}

	err = s35390a_get_reg(s35390a, S35390A_CMD_INT1, buf, sizeof(buf));
	if (err < 0)
		return -EIO;

	dev_dbg(&s35390a->rtc->dev, "%s: 0x%x 0x%x 0x%x\n",
		__func__, buf[0], buf[1], buf[2]);

	/* This chip returns the bits of each byte in reverse order */
	for (i = 0; i < 3; ++i)
		buf[i] = bitrev8(buf[i]);

	if (buf[S35390A_INT_BYTE_MINS] & 0x80)
		tm->tm_min = BCD2BIN(buf[S35390A_INT_BYTE_MINS] & 0x7f);
	else
		tm->tm_min = -1;
	if (buf[S35390A_INT_BYTE_HOURS] & 0x80)
		tm->tm_hour = s35390a_reg2hr(s35390a, buf[S35390A_INT_BYTE_HOURS] & 0x7f);
	else
		tm->tm_hour = -1;
	if (buf[S35390A_INT_BYTE_WDAY] & 0x80)
		tm->tm_wday = BCD2BIN(buf[S35390A_INT_BYTE_WDAY] & 0x7f);
	else
		tm->tm_wday = -1;
	if ((tm->tm_min == -1) && (tm->tm_hour == -1) && (tm->tm_wday == -1))
		tm->tm_sec = -1;
	else
		tm->tm_sec = 0;

	dev_dbg(&s35390a->rtc->dev, "%s: tm is secs=%d, mins=%d, hours=%d, mday=%d, "
		"mon=%d, year=%d, wday=%d\n", __func__, tm->tm_sec,
		tm->tm_min, tm->tm_hour, tm->tm_mday, tm->tm_mon, tm->tm_year,
		tm->tm_wday);

	return 0;
}

static int s35390a_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{

	struct i2c_client *client = to_i2c_client(dev);
	struct s35390a *s35390a = i2c_get_clientdata(client);
	struct rtc_time *tm = &alrm->time;

	/* The S35390A is not be capable of secs/day/month/year fields for alarm. */
	tm->tm_mday = tm->tm_mon = tm->tm_year = -1;

	tm->tm_min = s35390a->alrm_time.tm_min;
	tm->tm_hour = s35390a->alrm_time.tm_hour;
	tm->tm_wday = s35390a->alrm_time.tm_wday;
	if ((tm->tm_min == -1) && (tm->tm_hour == -1) && (tm->tm_wday == -1))
		tm->tm_sec = -1;
	else
		tm->tm_sec = 0;

	alrm->enabled = s35390a->alarm_enabled;

	return 0;
}

static int s35390a_rtc_set_alarm_internal(struct s35390a *s35390a)
{
	struct rtc_time *tm = &s35390a->alrm_time;
	char buf[3];
	int i, err;

	if (tm->tm_wday == -1 && tm->tm_hour == -1 && tm->tm_min == -1)
		return -EINVAL;

	if (!s35390a_is_alarm_mode(s35390a)) {
		err = s35390a_enable_alarm_mode(s35390a);
		if (err != 0)
			return err;
	}

	if (tm->tm_wday != -1)
		buf[S35390A_INT_BYTE_WDAY] = BIN2BCD(tm->tm_wday) | 0x80;
	else
		buf[S35390A_INT_BYTE_WDAY] = 0;
	if (tm->tm_hour != -1)
		buf[S35390A_INT_BYTE_HOURS] = s35390a_hr2reg(s35390a, tm->tm_hour) | 0x80;
	else
		buf[S35390A_INT_BYTE_HOURS] = 0;
	if (tm->tm_min != -1)
		buf[S35390A_INT_BYTE_MINS] = BIN2BCD(tm->tm_min) | 0x80;
	else
		buf[S35390A_INT_BYTE_MINS] = 0;

	/* This chip expects the bits of each byte to be in reverse order */
	for (i = 0; i < 3; ++i)
		buf[i] = bitrev8(buf[i]);

	dev_dbg(&s35390a->rtc->dev, "%s: 0x%x 0x%x 0x%x\n",
		__func__, buf[0], buf[1], buf[2]);

	err = s35390a_set_reg(s35390a, S35390A_CMD_INT1, buf, sizeof(buf));
	if (!err)
		err = s35390a_set_reg(s35390a, S35390A_CMD_INT2, buf, sizeof(buf));
	return err;
}

static int s35390a_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct s35390a	*s35390a = i2c_get_clientdata(client);
	struct rtc_time *tm = &alrm->time;

	if (s35390a->time_is_invalid) {
		dev_warn(&client->dev, "RTC time must be set before setting the alarm time.\n");
		return -EINVAL;
	}

	if (tm->tm_wday == -1 && tm->tm_hour == -1 && tm->tm_min == -1)
		return -EINVAL;

	s35390a->alrm_time.tm_wday = alrm->time.tm_wday;
	s35390a->alrm_time.tm_hour = alrm->time.tm_hour;
	s35390a->alrm_time.tm_min = alrm->time.tm_min;

	if (alrm->enabled)
		return s35390a_enable_alarm_irq(s35390a);
	else
		return s35390a_disable_alarm_irq(s35390a);
}

static int s35390a_rtc_ioctl(struct device *dev, unsigned int cmd, unsigned long arg)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct s35390a	*s35390a = i2c_get_clientdata(client);
	int ret;

	switch (cmd) {
	case RTC_AIE_OFF:		/* alarm off */
		ret = s35390a_disable_alarm_irq(s35390a);
		break;
	case RTC_AIE_ON:		/* alarm on */
		ret = s35390a_enable_alarm_irq(s35390a);
		break;
	default:
		ret = -ENOIOCTLCMD;
		break;
	}

	return ret;
}

static const struct rtc_class_ops s35390a_rtc_ops = {
	.read_time	= s35390a_rtc_read_time,
	.set_time	= s35390a_rtc_set_time,
	.read_alarm	= s35390a_rtc_read_alarm,
	.set_alarm	= s35390a_rtc_set_alarm,
	.ioctl		= s35390a_rtc_ioctl
};

static struct i2c_driver s35390a_driver;

static int s35390a_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	int err;
	unsigned int i;
	struct s35390a *s35390a;
	struct s35390a_platform_data *plat_data = client->dev.platform_data;
	struct rtc_time tm;
	char buf[1];

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit;
	}

	s35390a = kzalloc(sizeof(struct s35390a), GFP_KERNEL);
	if (!s35390a) {
		err = -ENOMEM;
		goto exit;
	}

	mutex_init(&s35390a->irq_lock);
	s35390a->client[0] = client;
	i2c_set_clientdata(client, s35390a);

	client->dev.power.can_wakeup = 1;

	/* This chip uses multiple addresses, use dummy devices for them */
	for (i = 1; i < 8; ++i) {
		s35390a->client[i] = i2c_new_dummy(client->adapter,
					client->addr + i);
		if (!s35390a->client[i]) {
			dev_err(&client->dev, "Address %02x unavailable\n",
						client->addr + i);
			err = -EBUSY;
			goto exit_dummy;
		}
	}

	err = s35390a_reset(s35390a);
	if (err < 0) {
		dev_err(&client->dev, "error resetting chip\n");
		goto exit_dummy;
	}

	err = s35390a_disable_test_mode(s35390a);
	if (err < 0) {
		dev_err(&client->dev, "error disabling test mode\n");
		goto exit_dummy;
	}

	err = s35390a_get_reg(s35390a, S35390A_CMD_STATUS1, buf, sizeof(buf));
	if (err < 0) {
		dev_err(&client->dev, "error checking 12/24 hour mode\n");
		goto exit_dummy;
	}
	if (buf[0] & S35390A_FLAG_24H)
		s35390a->twentyfourhour = 1;
	else
		s35390a->twentyfourhour = 0;

	if (s35390a_get_datetime(client, &tm) < 0)
		dev_warn(&client->dev, "clock needs to be set\n");

	s35390a->rtc = rtc_device_register(s35390a_driver.driver.name,
				&client->dev, &s35390a_rtc_ops, THIS_MODULE);

	if (IS_ERR(s35390a->rtc)) {
		err = PTR_ERR(s35390a->rtc);
		goto exit_dummy;
	}

	s35390a->rtc->dev.power.can_wakeup = 1;

	mutex_lock(&s35390a->irq_lock);
	if (plat_data) {
		if (plat_data->alarm_irq_init)
			plat_data->alarm_irq_init();
		s35390a->alarm_irq = plat_data->alarm_irq;
		if (plat_data->alarm_is_wakeup_src)
			s35390a->rtc->dev.power.should_wakeup = 1;
	} else
		s35390a->alarm_irq = -1;

	if (S35390A_ALARM_IRQ_IS_AVAILABLE(s35390a)) {
		INIT_WORK(&s35390a->interrupt_work, s35390a_interrupt_work);
		err = request_irq(s35390a->alarm_irq, s35390a_interrupt_handler,
				  IRQF_DISABLED | IRQF_TRIGGER_FALLING, s35390a->rtc->dev.bus_id,
				  s35390a);
		if (err) {
			dev_err(&client->dev, "error request_irq for %d\n", s35390a->alarm_irq);
			rtc_device_unregister(s35390a->rtc);
			mutex_unlock(&s35390a->irq_lock);
			goto exit_dummy;
		}
		disable_irq(s35390a->alarm_irq);

		s35390a_rtc_read_alarm_internal(s35390a);
		if (s35390a_is_alarm_mode(s35390a))
			s35390a_enable_alarm_irq_unlocked(s35390a);
	}
	mutex_unlock(&s35390a->irq_lock);

	return 0;

exit_dummy:
	for (i = 1; i < 8; ++i)
		if (s35390a->client[i])
			i2c_unregister_device(s35390a->client[i]);
	kfree(s35390a);
	i2c_set_clientdata(client, NULL);

exit:
	return err;
}

static int s35390a_remove(struct i2c_client *client)
{
	unsigned int i;

	struct s35390a *s35390a = i2c_get_clientdata(client);

	if (S35390A_ALARM_IRQ_IS_AVAILABLE(s35390a)) {
		s35390a_disable_alarm_irq(s35390a);
		mutex_lock(&s35390a->irq_lock);
		free_irq(s35390a->alarm_irq, s35390a);
		mutex_unlock(&s35390a->irq_lock);
	}

	for (i = 1; i < 8; ++i)
		if (s35390a->client[i])
			i2c_unregister_device(s35390a->client[i]);

	rtc_device_unregister(s35390a->rtc);
	mutex_destroy(&s35390a->irq_lock);
	kfree(s35390a);
	i2c_set_clientdata(client, NULL);

	return 0;
}

#if defined(CONFIG_PM)
int s35390a_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct s35390a *s35390a = i2c_get_clientdata(client);

	if (S35390A_ALARM_IRQ_IS_AVAILABLE(s35390a)) {
		mutex_lock(&s35390a->irq_lock);
		if (device_may_wakeup(&s35390a->rtc->dev))
			enable_irq_wake(s35390a->alarm_irq);

		if (s35390a->alarm_enabled)
			disable_irq(s35390a->alarm_irq);
		mutex_unlock(&s35390a->irq_lock);
	}

	return 0;
}

int s35390a_resume(struct i2c_client *client)
{
	struct s35390a *s35390a = i2c_get_clientdata(client);

	if (S35390A_ALARM_IRQ_IS_AVAILABLE(s35390a)) {
		mutex_lock(&s35390a->irq_lock);
		if (s35390a->alarm_enabled)
			enable_irq(s35390a->alarm_irq);

		if (device_may_wakeup(&s35390a->rtc->dev))
			disable_irq_wake(s35390a->alarm_irq);
		mutex_unlock(&s35390a->irq_lock);

		schedule_work(&s35390a->interrupt_work);
	}

	return 0;
}
#else
#define s35390a_suspend NULL
#define s35390a_resume NULL
#endif /* defined(CONFIG_PM) */

static struct i2c_driver s35390a_driver = {
	.driver		= {
		.name	= "rtc-s35390a",
	},
	.probe		= s35390a_probe,
	.remove		= s35390a_remove,
	.suspend	= s35390a_suspend,
	.resume		= s35390a_resume,
	.id_table	= s35390a_id,
};

static int __init s35390a_rtc_init(void)
{
	return i2c_add_driver(&s35390a_driver);
}

static void __exit s35390a_rtc_exit(void)
{
	i2c_del_driver(&s35390a_driver);
}

MODULE_AUTHOR("Byron Bradley <byron.bbradley@gmail.com>");
MODULE_DESCRIPTION("S35390A RTC driver");
MODULE_LICENSE("GPL");

module_init(s35390a_rtc_init);
module_exit(s35390a_rtc_exit);
