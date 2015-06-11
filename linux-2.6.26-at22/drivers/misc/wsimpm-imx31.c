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

/*
 * Notes:
 *
 *  * This driver at present simply controls the W-SIM power on/off by
 *    responding to the insert line.
 *  * It does not provide any userspace interface, but should once an
 *    appropriate interface can be decided on.
 *  * It does not implement suspend/resume as it is highly likely that the user
 *    would want the W-SIM to be able to wake up the system, something it
 *    could not do if it had no power.
 */ 

#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/interrupt.h>

#include <asm/mach/irq.h>

#include <asm/arch/gpio.h>
#include <asm/arch/wsimpm.h>

#define DRIVER_NAME		"wsim_pm"
#define DRIVER_DESCRIPTION	"W-SIM Power Management for i.MX31"

// #define DEBUG
#ifdef DEBUG
#  define WSIMPM_DEBUG(fmt, arg...) \
	printk(KERN_DEBUG "%s: " fmt, DRIVER_NAME, ##arg)
#else
#  define WSIMPM_DEBUG(fmt, arg...)
#endif

#define WSIMPM_DEBOUNCE_PERIOD	(50 * 1000000)	/* ns delay */
#define WSIMPM_DEBOUNCE_TIMES	(5)
#define WSIMPM_DEBOUNCE_MAX	(20)

#define WSIM_INSERT_ACTIVE	(0)

struct wsimpm_debounce {
	int processing;
	int previous_state;
	int count;
	int ok_count;
	struct hrtimer timer;
	spinlock_t lock;
};

struct wsimpm {
	int ins_gpio;
	int ins_gpio_irq;
	int ins_gpio_state_cur;
	int power_gpio;
	int power_gpio_state_on;
	struct wsimpm_debounce debounce;
};

static inline int wsimpm_ins_state(struct wsimpm *wsimpm)
{
	return mxc_get_gpio_datain(wsimpm->ins_gpio);
}

static inline void wsimpm_power_on(struct wsimpm *wsimpm)
{
	WSIMPM_DEBUG("power on\n");

	mxc_set_gpio_dataout(wsimpm->power_gpio, wsimpm->power_gpio_state_on);
}

static inline void wsimpm_power_off(struct wsimpm *wsimpm)
{
	WSIMPM_DEBUG("power off\n");

	mxc_set_gpio_dataout(wsimpm->power_gpio, !wsimpm->power_gpio_state_on);
}

static void wsimpm_adjust_power(struct wsimpm *wsimpm)
{
	int ins;

	ins = wsimpm_ins_state(wsimpm);
	wsimpm->ins_gpio_state_cur = ins;

	if (ins == WSIM_INSERT_ACTIVE)
		wsimpm_power_on(wsimpm);
	else
		wsimpm_power_off(wsimpm);

	do {
		ins = wsimpm_ins_state(wsimpm);
		if (ins)
			set_irq_type(wsimpm->ins_gpio_irq, IRQT_FALLING);
		else
			set_irq_type(wsimpm->ins_gpio_irq, IRQT_RISING);
	} while (ins != wsimpm_ins_state(wsimpm));
}

static void wsimpm_debounce_reset(struct wsimpm_debounce *db)
{
	WSIMPM_DEBUG("debounce: reset\n");

	db->count = 0;
	db->ok_count = 0;
}

static enum hrtimer_restart wsimpm_debounce(struct hrtimer *handle)
{
	struct wsimpm_debounce *db = container_of(handle,
		struct wsimpm_debounce, timer);
	struct wsimpm *wsimpm = container_of(db, struct wsimpm, debounce);
	unsigned long flags;
	int ins;

	WSIMPM_DEBUG("debounce: count: %d ok: %d\n", db->count, db->ok_count);

	spin_lock_irqsave(&db->lock, flags);

	if (unlikely(!db->processing))
		goto debounce_end;

	db->count++;

	ins = wsimpm_ins_state(wsimpm);

	if (likely(db->previous_state == ins)) {
		db->ok_count++;
		if (db->ok_count >= WSIMPM_DEBOUNCE_TIMES)
			wsimpm_adjust_power(wsimpm);
		else
			goto restart;
	} else if (db->count <= WSIMPM_DEBOUNCE_MAX) {
		db->previous_state = ins;
		db->ok_count = 0;
		WSIMPM_DEBUG("debounce: restarted\n");
		goto restart;
	}

 debounce_end:
	wsimpm_debounce_reset(db);
	db->processing = 0;
	spin_unlock_irqrestore(&db->lock, flags);
	return HRTIMER_NORESTART;

 restart:
	spin_unlock_irqrestore(&db->lock, flags);
	return HRTIMER_RESTART;
}

static irqreturn_t mxc_wsimpm_irq(int irq, void *devid)
{
	struct wsimpm *wsimpm = devid;
	struct wsimpm_debounce *db = &wsimpm->debounce;
	unsigned long flags;

	WSIMPM_DEBUG("irq\n");

	spin_lock_irqsave(&db->lock, flags);

	if (likely(wsimpm_ins_state(wsimpm) != wsimpm->ins_gpio_state_cur)) {
		wsimpm_debounce_reset(db);
		db->processing = 1;
		db->previous_state = wsimpm_ins_state(wsimpm);
		hrtimer_start(&db->timer, ktime_set(0, WSIMPM_DEBOUNCE_PERIOD),
			HRTIMER_MODE_REL);
	}

	spin_unlock_irqrestore(&db->lock, flags);

	return IRQ_HANDLED;
}

static int mxc_wsimpm_probe(struct platform_device *pdev)
{
	struct wsimpm_info *wsimpm_info = pdev->dev.platform_data;
	struct wsimpm *wsimpm;
	int ret;

	if (!wsimpm_info)
		return -ENODEV;

	wsimpm = kzalloc(sizeof(struct wsimpm), GFP_KERNEL);
	if (!wsimpm)
		return -ENOMEM;

	wsimpm->ins_gpio		= wsimpm_info->ins_gpio;
	wsimpm->ins_gpio_irq		= wsimpm_info->ins_gpio_irq;
	wsimpm->power_gpio		= wsimpm_info->power_gpio;
	wsimpm->power_gpio_state_on	= wsimpm_info->power_gpio_state_on;

	wsimpm_adjust_power(wsimpm);

	spin_lock_init(&wsimpm->debounce.lock);

	hrtimer_init(&wsimpm->debounce.timer, CLOCK_MONOTONIC,
		HRTIMER_MODE_REL);
	wsimpm->debounce.timer.function = wsimpm_debounce;

	ret = request_irq(wsimpm->ins_gpio_irq, mxc_wsimpm_irq, 0,
		DRIVER_NAME, wsimpm);
	if (ret) {
		kfree(wsimpm);
		printk(KERN_ERR "%s: failed to obtain IRQ\n", DRIVER_NAME);
		return ret;
	}

	dev_set_drvdata(&pdev->dev, wsimpm);

	printk(KERN_INFO "%s: %s\n", DRIVER_NAME, DRIVER_DESCRIPTION);

	return 0;
}

static int mxc_wsimpm_remove(struct platform_device *pdev)
{
	struct wsimpm *wsimpm = dev_get_drvdata(&pdev->dev);

	wsimpm_power_off(wsimpm);

	dev_set_drvdata(&pdev->dev, NULL);
	free_irq(wsimpm->ins_gpio_irq, wsimpm);
	hrtimer_cancel(&wsimpm->debounce.timer);
	kfree(wsimpm);

	return 0;
}

static struct platform_driver mxc_wsimpm_driver = {
	.probe	= mxc_wsimpm_probe,
	.remove	= __devexit_p(mxc_wsimpm_remove),
	.driver	= {
		.name = "mxc_wsimpm",
	},
};

static int __init mxc_wsimpm_init(void)
{
	return platform_driver_register(&mxc_wsimpm_driver);
}

static void __exit mxc_wsimpm_exit(void)
{
	platform_driver_unregister(&mxc_wsimpm_driver);
}

module_init(mxc_wsimpm_init);
module_exit(mxc_wsimpm_exit);

MODULE_AUTHOR("Atmark Techno, Inc.");
MODULE_DESCRIPTION(DRIVER_DESCRIPTION);
MODULE_LICENSE("GPL v2");

