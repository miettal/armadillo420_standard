/*
 * Copyright 2009 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2011 Atmark Techno, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file imx_adc_ts.c
 *
 * @brief Driver for the Freescale Semiconductor i.MX ADC touchscreen.
 *
 * This touchscreen driver is designed as a standard input driver.  It is a
 * wrapper around the low level ADC driver. Much of the hardware configuration
 * and touchscreen functionality is implemented in the low level ADC driver.
 * During initialization, this driver creates a kernel thread.  This thread
 * then calls the ADC driver to obtain touchscreen values continously. These
 * values are then passed to the input susbsystem.
 *
 * @ingroup touchscreen
 */

#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/freezer.h>
#include <linux/platform_device.h>
#include <linux/regulator/regulator.h>
#include <asm/arch/imx_adc.h>
#include <asm/arch/imx_adc_ts.h>

#define IMX_ADC_TS_NAME	"imx_adc_ts"

struct imx_adc_ts_driver_data {
	struct input_dev *inputdev;
	struct task_struct *thread;
	struct regulator *regu;
};

static int ts_thread(void *arg)
{
	struct platform_device *pdev = arg;
	struct imx_adc_ts_driver_data *drvdata = platform_get_drvdata(pdev);
	struct t_touch_screen ts_sample;
	int wait = 1;

	daemonize("imx_adc_ts");

	do {
		try_to_freeze();

		memset(&ts_sample, 0, sizeof(ts_sample));
		if (0 != imx_adc_get_touch_sample(&ts_sample, wait))
			continue;

		pr_debug("(%4d, %4d, %4d, %4d, %4d)\n",
			 ts_sample.pos_x,
			 ts_sample.pos_y,
			 ts_sample.pressure,
			 ts_sample.flag,
			 wait);

		wait = (ts_sample.flag & TSF_VALID) ? 0 : 1;

		if (ts_sample.flag) {
			input_report_abs(drvdata->inputdev, ABS_X,
					 (unsigned short)ts_sample.pos_x);
			input_report_abs(drvdata->inputdev, ABS_Y,
					 (unsigned short)ts_sample.pos_y);
			input_report_abs(drvdata->inputdev, ABS_PRESSURE, 1);
			input_report_key(drvdata->inputdev, BTN_TOUCH, 1);
		} else {
			input_report_abs(drvdata->inputdev, ABS_PRESSURE, 0);
			input_report_key(drvdata->inputdev, BTN_TOUCH, 0);
		}
		input_sync(drvdata->inputdev);

		msleep(10);
	} while (!kthread_should_stop());

	return 0;
}

static int imx_adc_ts_probe(struct platform_device *pdev)
{
	int retval;
	struct imx_adc_ts_driver_data *drvdata;
	struct platform_imx_adc_ts_data *plat_data = pdev->dev.platform_data;
	bool should_wakeup = 0;

	if (!is_imx_adc_ready())
		return -ENODEV;

	drvdata = kzalloc(sizeof(struct imx_adc_ts_driver_data), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	platform_set_drvdata(pdev, drvdata);

	if (plat_data)
		should_wakeup = plat_data->is_wake_src;

	device_init_wakeup(&pdev->dev, 1);
	device_set_wakeup_enable(&pdev->dev, should_wakeup);
	imx_adc_register_ts(&pdev->dev);

	if (plat_data && plat_data->regu_name) {
		drvdata->regu = regulator_get(NULL, plat_data->regu_name);
		if (IS_ERR(drvdata->regu)) {
			retval = PTR_ERR(drvdata->regu);
			goto err_regulator_get;
		}
		retval = regulator_enable(drvdata->regu);
		if (retval)
			goto err_regulator_enable;
	}

	drvdata->inputdev = input_allocate_device();
	if (!drvdata->inputdev) {
		pr_err("imx_ts_init: not enough memory for input device\n");
		retval = -ENOMEM;
		goto err_input_allocate_device;
	}

	drvdata->inputdev->name = IMX_ADC_TS_NAME;
	drvdata->inputdev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	drvdata->inputdev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	input_set_abs_params(drvdata->inputdev, ABS_X,
			     100,
			     4000,
			     0, 0);
	input_set_abs_params(drvdata->inputdev, ABS_Y,
			     100,
			     4000,
			     0, 0);
	input_set_abs_params(drvdata->inputdev, ABS_PRESSURE,
			     0, 1, 0, 0);


	retval = input_register_device(drvdata->inputdev);
	if (retval < 0)
		goto err_input_register_device;

	drvdata->thread = kthread_run(ts_thread, pdev, "ts_thread");
	if (IS_ERR(drvdata->thread)) {
		retval = PTR_ERR(drvdata->thread);
		goto err_kthread_run;
	}
	pr_info("i.MX ADC input touchscreen loaded.\n");

	return 0;


err_kthread_run:
	input_unregister_device(drvdata->inputdev);
err_input_register_device:
	input_free_device(drvdata->inputdev);
err_input_allocate_device:
	if (drvdata->regu)
		regulator_disable(drvdata->regu);
err_regulator_enable:
	if (drvdata->regu)
		regulator_put(drvdata->regu, NULL);
err_regulator_get:
	imx_adc_unregister_ts();
	kfree(drvdata);
	platform_set_drvdata(pdev, NULL);

	return retval;
}

static int imx_adc_ts_remove(struct platform_device *pdev)
{
	struct imx_adc_ts_driver_data *drvdata = platform_get_drvdata(pdev);

	kthread_stop(drvdata->thread);
	input_unregister_device(drvdata->inputdev);
	input_free_device(drvdata->inputdev);
	if (drvdata->regu) {
		regulator_disable(drvdata->regu);
		regulator_put(drvdata->regu, NULL);
	}
	imx_adc_unregister_ts();
	kfree(drvdata);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

#if defined(CONFIG_PM)
static int imx_adc_ts_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct imx_adc_ts_driver_data *drvdata = platform_get_drvdata(pdev);

	if (!device_may_wakeup(&pdev->dev))
		if (drvdata->regu)
			regulator_disable(drvdata->regu);

	return 0;
}

static int imx_adc_ts_resume(struct platform_device *pdev)
{
	struct imx_adc_ts_driver_data *drvdata = platform_get_drvdata(pdev);

	if (!device_may_wakeup(&pdev->dev))
		if (drvdata->regu)
			regulator_enable(drvdata->regu);

	return 0;
}
#else
#define imx_adc_ts_suspend NULL
#define imx_adc_ts_resume NULL
#endif /* defined(CONFIG_PM) */

static struct platform_driver imx_adc_ts_driver = {
	.driver = {
		.name = "imx_adc_ts",
	},
	.probe = imx_adc_ts_probe,
	.remove = imx_adc_ts_remove,
	.suspend = imx_adc_ts_suspend,
	.resume = imx_adc_ts_resume,
};

static int __init imx_adc_ts_init(void)
{
	return platform_driver_register(&imx_adc_ts_driver);
}
late_initcall(imx_adc_ts_init);

static void __exit imx_adc_ts_exit(void)
{
	platform_driver_unregister(&imx_adc_ts_driver);
}
module_exit(imx_adc_ts_exit);

MODULE_DESCRIPTION("i.MX ADC input touchscreen driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
