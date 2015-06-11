/*
 * simple driver for PWM (Pulse Width Modulator) controller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Derived from pxa PWM driver by eric miao <eric.miao@marvell.com>
 * Copyright 2009 Freescale Semiconductor, Inc. All Rights Reserved.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/ctype.h>
#include <linux/device.h>
#include <linux/pwm.h>
#include <asm/arch/hardware.h>


/* i.MX1 and i.MX21 share the same PWM function block: */

#define MX1_PWMC    0x00   /* PWM Control Register */
#define MX1_PWMS    0x04   /* PWM Sample Register */
#define MX1_PWMP    0x08   /* PWM Period Register */


/* i.MX27, i.MX25, i.MX31, i.MX35, i.MX51 share the same PWM function block: */

#define MXC_PWMCR                 0x00    /* PWM Control Register */
#define MXC_PWMSAR                0x0C    /* PWM Sample Register */
#define MXC_PWMPR                 0x10    /* PWM Period Register */
#define MXC_PWMCR_PRESCALER(x)    (((x - 1) & 0xFFF) << 4)

#define MXC_PWMCR_STOPEN		(1 << 25)
#define MXC_PWMCR_DOZEEN                (1 << 24)
#define MXC_PWMCR_WAITEN                (1 << 23)
#define MXC_PWMCR_DBGEN			(1 << 22)
#define MXC_PWMCR_CLKSRC_IPG		(1 << 16)
#define MXC_PWMCR_CLKSRC_IPG_HIGH	(2 << 16)
#define MXC_PWMCR_CLKSRC_IPG_32k	(3 << 16)
#define MXC_PWMCR_EN			(1 << 0)
#define MXC_PWMCR_POUTC_INVERT		(1 << 18)

struct pwm_device {
	struct list_head	node;
	struct platform_device *pdev;

	const char	*label;
	struct clk	*clk;

	int		clk_enabled;
	void __iomem	*mmio_base;

	unsigned int	use_count;
	unsigned int	pwm_id;

	int		invert;
};

int pwm_config(struct pwm_device *pwm, int duty_ns, int period_ns)
{
	if (pwm == NULL || period_ns == 0 || duty_ns > period_ns)
		return -EINVAL;

	if (cpu_is_mx27() || cpu_is_mx25() || cpu_is_mx31() || cpu_is_mx51()) {
		unsigned long long c, clock;
		unsigned long period_cycles, duty_cycles, prescale;
		unsigned long cr;
		clock = c = clk_get_rate(pwm->clk);
		c = c * period_ns;
		do_div(c, 1000000000);
		period_cycles = c;

		prescale = period_cycles / 0x10000 + 1;

		period_cycles /= prescale;
		c = (unsigned long long)period_cycles * duty_ns;
		do_div(c, period_ns);
		duty_cycles = c;

		pr_debug("%s: clock=%lld, period cycles=%ld, duty cycle=%ld, prescale=%ld\n",
			 __func__, clock, period_cycles, duty_cycles, prescale);

		writel(duty_cycles, pwm->mmio_base + MXC_PWMSAR);
		writel(period_cycles, pwm->mmio_base + MXC_PWMPR);
		cr = readl(pwm->mmio_base + MXC_PWMCR) & MXC_PWMCR_EN;
		cr |= MXC_PWMCR_PRESCALER(prescale) |
			MXC_PWMCR_CLKSRC_IPG_HIGH |
			MXC_PWMCR_STOPEN | MXC_PWMCR_DOZEEN |
			MXC_PWMCR_WAITEN | MXC_PWMCR_DBGEN;
		if (pwm->invert) cr |= MXC_PWMCR_POUTC_INVERT;
		writel(cr, pwm->mmio_base + MXC_PWMCR);
	} else if (cpu_is_mx21()) {
		/* The PWM subsystem allows for exact frequencies. However,
		 * I cannot connect a scope on my device to the PWM line and
		 * thus cannot provide the program the PWM controller
		 * exactly. Instead, I'm relying on the fact that the
		 * Bootloader (u-boot or WinCE+haret) has programmed the PWM
		 * function group already. So I'll just modify the PWM sample
		 * register to follow the ratio of duty_ns vs. period_ns
		 * accordingly.
		 *
		 * This is good enought for programming the brightness of
		 * the LCD backlight.
		 *
		 * The real implementation would divide PERCLK[0] first by
		 * both the prescaler (/1 .. /128) and then by CLKSEL
		 * (/2 .. /16).
		 */
		u32 max = readl(pwm->mmio_base + MX1_PWMP);
		u32 p = max * duty_ns / period_ns;
		writel(max - p, pwm->mmio_base + MX1_PWMS);
	} else {
		BUG();
	}

	return 0;
}
EXPORT_SYMBOL(pwm_config);

int pwm_set_invert(struct pwm_device *pwm, int invert)
{
	if (pwm == NULL)
		return -EINVAL;

	if (cpu_is_mx27() || cpu_is_mx25() || cpu_is_mx31() || cpu_is_mx51())
		pwm->invert = invert;

	return 0;
}
EXPORT_SYMBOL(pwm_set_invert);

int pwm_enable(struct pwm_device *pwm)
{
	unsigned long reg;
	int rc = 0;

	if (!pwm->clk_enabled) {
		rc = clk_enable(pwm->clk);
		if (!rc)
			pwm->clk_enabled = 1;
	}

	reg = readl(pwm->mmio_base + MXC_PWMCR);
	reg |= MXC_PWMCR_EN;
	writel(reg, pwm->mmio_base + MXC_PWMCR);
	return rc;
}
EXPORT_SYMBOL(pwm_enable);

void pwm_disable(struct pwm_device *pwm)
{
	unsigned long reg;

	if (pwm->clk_enabled) {
		clk_disable(pwm->clk);
		pwm->clk_enabled = 0;
	}

	reg = readl(pwm->mmio_base + MXC_PWMCR);
	reg &= ~MXC_PWMCR_EN;
	writel(reg, pwm->mmio_base + MXC_PWMCR);

}
EXPORT_SYMBOL(pwm_disable);

static DEFINE_MUTEX(pwm_lock);
static LIST_HEAD(pwm_list);

struct pwm_device *pwm_request(int pwm_id, const char *label)
{
	struct pwm_device *pwm;
	int found = 0;

	mutex_lock(&pwm_lock);

	list_for_each_entry(pwm, &pwm_list, node) {
		if (pwm->pwm_id == pwm_id) {
			found = 1;
			break;
		}
	}

	if (found) {
		if (pwm->use_count == 0) {
			pwm->use_count++;
			pwm->label = label;
		} else
			pwm = ERR_PTR(-EBUSY);
	} else
		pwm = ERR_PTR(-ENOENT);

	mutex_unlock(&pwm_lock);
	return pwm;
}
EXPORT_SYMBOL(pwm_request);

void pwm_free(struct pwm_device *pwm)
{
	mutex_lock(&pwm_lock);

	if (pwm->use_count) {
		pwm->use_count--;
		pwm->label = NULL;
	} else
		pr_warning("PWM device already freed\n");

	mutex_unlock(&pwm_lock);
}
EXPORT_SYMBOL(pwm_free);

#if defined(CONFIG_PM)
static int mxc_pwm_suspend(struct platform_device *pdev, pm_message_t state)
{
#if defined(CONFIG_MXC_PWM_CLASS)
	struct platform_pwm_data *data = pdev->dev.platform_data;
	if (data && data->export)
		return mxc_pwm_class_suspend(&data->cdev);
#endif

	return 0;
}

static int mxc_pwm_resume(struct platform_device *pdev)
{
#if defined(CONFIG_MXC_PWM_CLASS)
	struct platform_pwm_data *data = pdev->dev.platform_data;
	if (data && data->export)
		return mxc_pwm_class_resume(&data->cdev);
#endif
	return 0;
}
#else
#define mxc_pwm_suspend NULL
#define mxc_pwm_resume NULL
#endif

static int __devinit mxc_pwm_probe(struct platform_device *pdev)
{
	struct pwm_device *pwm;
	struct resource *r;
	struct platform_pwm_data *data = pdev->dev.platform_data;
	int ret = 0;

	pwm = kzalloc(sizeof(struct pwm_device), GFP_KERNEL);
	if (pwm == NULL) {
		dev_err(&pdev->dev, "failed to allocate memory\n");
		return -ENOMEM;
	}

	pwm->clk = clk_get(&pdev->dev, "pwm");

	if (IS_ERR(pwm->clk)) {
		ret = PTR_ERR(pwm->clk);
		goto err_free;
	}

	pwm->clk_enabled = 0;

	if (data)
		pwm_set_invert(pwm, data->invert);
	pwm->use_count = 0;
	pwm->pwm_id = pdev->id;
	pwm->pdev = pdev;

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (r == NULL) {
		dev_err(&pdev->dev, "no memory resource defined\n");
		ret = -ENODEV;
		goto err_free_clk;
	}

	r = request_mem_region(r->start, r->end - r->start + 1, pdev->name);
	if (r == NULL) {
		dev_err(&pdev->dev, "failed to request memory resource\n");
		ret = -EBUSY;
		goto err_free_clk;
	}

	pwm->mmio_base = ioremap(r->start, r->end - r->start + 1);
	if (pwm->mmio_base == NULL) {
		dev_err(&pdev->dev, "failed to ioremap() registers\n");
		ret = -ENODEV;
		goto err_free_mem;
	}

	mutex_lock(&pwm_lock);
	list_add_tail(&pwm->node, &pwm_list);
	mutex_unlock(&pwm_lock);

#if defined(CONFIG_MXC_PWM_CLASS)
	if (data && data->export) {
		data->cdev.name = data->name;
		data->cdev.invert = data->invert;
		ret = mxc_pwm_classdev_register(&pdev->dev, pdev->id, &data->cdev);
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to register classdev\n");
			goto err_classdev_register;
		}
	}
#endif

	platform_set_drvdata(pdev, pwm);

	return 0;

#if defined(CONFIG_MXC_PWM_CLASS)
err_classdev_register:
	iounmap(pwm->mmio_base);
#endif
err_free_mem:
	release_mem_region(r->start, r->end - r->start + 1);
err_free_clk:
	clk_put(pwm->clk);
err_free:
	kfree(pwm);
	return ret;
}

static int __devexit mxc_pwm_remove(struct platform_device *pdev)
{
	struct pwm_device *pwm;
	struct resource *r;
	struct platform_pwm_data *data __maybe_unused = pdev->dev.platform_data;

	pwm = platform_get_drvdata(pdev);
	if (pwm == NULL)
		return -ENODEV;

#if defined(CONFIG_MXC_PWM_CLASS)
	if (data && data->export)
		mxc_pwm_classdev_unregister(&data->cdev);
#endif

	mutex_lock(&pwm_lock);
	list_del(&pwm->node);
	mutex_unlock(&pwm_lock);

	iounmap(pwm->mmio_base);

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(r->start, r->end - r->start + 1);

	clk_put(pwm->clk);

	kfree(pwm);
	return 0;
}

static struct platform_driver mxc_pwm_driver = {
	.driver		= {
		.name	= "mxc_pwm",
	},
	.probe		= mxc_pwm_probe,
	.remove		= __devexit_p(mxc_pwm_remove),
	.suspend	= mxc_pwm_suspend,
	.resume		= mxc_pwm_resume,
};

static int __init mxc_pwm_init(void)
{
#if defined(CONFIG_MXC_PWM_CLASS)
	int ret = mxc_pwm_class_init();

	if (ret)
		return ret;
#endif

	return platform_driver_register(&mxc_pwm_driver);
}
arch_initcall(mxc_pwm_init);

static void __exit mxc_pwm_exit(void)
{
	platform_driver_unregister(&mxc_pwm_driver);
#if defined(CONFIG_MXC_PWM_CLASS)
	mxc_pwm_class_exit();
#endif
}
module_exit(mxc_pwm_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Sascha Hauer <s.hauer@pengutronix.de>");
