/*
 * Copyright 2009 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file adc/imx_adc.c
 * @brief This is the main file of i.MX ADC driver.
 *
 * @ingroup IMX_ADC
 */

/*
 * Includes
 */

#include <linux/io.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <asm/arch/imx_adc.h>
#include "imx_adc_reg.h"

/* Driver data */
struct imx_adc_data {
	u32 irq;
	struct clk *adc_clk;
};

static int imx_adc_major;

/*!
 * Number of users waiting in suspendq
 */
static int swait;

/*!
 * To indicate whether any of the adc devices are suspending
 */
static int suspend_flag;

/*!
 * The suspendq is used by blocking application calls
 */
static wait_queue_head_t suspendq;
static wait_queue_head_t tsq;

static bool imx_adc_ready;

static struct class *imx_adc_class;
static struct imx_adc_data *adc_data;

static DECLARE_MUTEX(general_convert_mutex);
static DECLARE_MUTEX(ts_convert_mutex);

static struct completion irq_wait;

unsigned long tsc_base;

static struct device *ts_dev;

int is_imx_adc_ready(void)
{
	return imx_adc_ready;
}
EXPORT_SYMBOL(is_imx_adc_ready);

int imx_adc_register_ts(struct device *dev)
{
	if (ts_dev)
		return -EBUSY;

	ts_dev = dev;

	return 0;
}
EXPORT_SYMBOL(imx_adc_register_ts);

void imx_adc_unregister_ts(void)
{
	ts_dev = NULL;
}
EXPORT_SYMBOL(imx_adc_unregister_ts);

void tsc_clk_enable(void)
{
	unsigned long reg;

	clk_enable(adc_data->adc_clk);

	reg = __raw_readl(tsc_base + TGCR);
	reg |= TGCR_IPG_CLK_EN;
	__raw_writel(reg, tsc_base + TGCR);
}

void tsc_clk_disable(void)
{
	unsigned long reg;

	clk_disable(adc_data->adc_clk);

	reg = __raw_readl(tsc_base + TGCR);
	reg &= ~TGCR_IPG_CLK_EN;
	__raw_writel(reg, tsc_base + TGCR);
}

void tsc_self_reset(void)
{
	unsigned long reg;

	reg = __raw_readl(tsc_base + TGCR);
	reg |= TGCR_TSC_RST;
	__raw_writel(reg, tsc_base + TGCR);

	while (__raw_readl(tsc_base + TGCR) & TGCR_TSC_RST)
		continue;
}

/* Internal reference */
void tsc_intref_enable(void)
{
	unsigned long reg;

	reg = __raw_readl(tsc_base + TGCR);
	reg |= TGCR_INTREFEN;
	__raw_writel(reg, tsc_base + TGCR);
}

/* initialize touchscreen */
void imx_tsc_init(void)
{
	unsigned long reg;
	int lastitemid;
	int dbtime;

	/* Level sense */
	reg = __raw_readl(tsc_base + TCQCR);
	reg |= CQCR_PD_CFG;
	reg |= (0xf << CQCR_FIFOWATERMARK_SHIFT);  /* watermark */
	__raw_writel(reg, tsc_base + TCQCR);

	/* Configure 4-wire */
	reg = TSC_4WIRE_PRECHARGE;
	reg |= CC_IGS;
	__raw_writel(reg, tsc_base + TCC0);

	reg = TSC_4WIRE_TOUCH_DETECT;
	reg |= 3 << CC_NOS_SHIFT;	/* 4 samples */
	reg |= 128 << CC_SETTLING_TIME_SHIFT;	/* it's important! */
	__raw_writel(reg, tsc_base + TCC1);

	reg = TSC_4WIRE_X_MEASUMENT;
	reg |= 3 << CC_NOS_SHIFT;	/* 4 samples */
	reg |= 128 << CC_SETTLING_TIME_SHIFT;	/* settling time */
	__raw_writel(reg, tsc_base + TCC2);

	reg = TSC_4WIRE_Y_MEASUMENT;
	reg |= 3 << CC_NOS_SHIFT;	/* 4 samples */
	reg |= 128 << CC_SETTLING_TIME_SHIFT;	/* settling time */
	__raw_writel(reg, tsc_base + TCC3);

	reg = (TCQ_ITEM_TCC0 << TCQ_ITEM7_SHIFT) |
	      (TCQ_ITEM_TCC0 << TCQ_ITEM6_SHIFT) |
	      (TCQ_ITEM_TCC1 << TCQ_ITEM5_SHIFT) |
	      (TCQ_ITEM_TCC0 << TCQ_ITEM4_SHIFT) |
	      (TCQ_ITEM_TCC3 << TCQ_ITEM3_SHIFT) |
	      (TCQ_ITEM_TCC2 << TCQ_ITEM2_SHIFT) |
	      (TCQ_ITEM_TCC1 << TCQ_ITEM1_SHIFT) |
	      (TCQ_ITEM_TCC0 << TCQ_ITEM0_SHIFT);
	__raw_writel(reg, tsc_base + TCQ_ITEM_7_0);

	lastitemid = 5;
	reg = __raw_readl(tsc_base + TCQCR);
	reg = (reg & ~CQCR_LAST_ITEM_ID_MASK) |
	      (lastitemid << CQCR_LAST_ITEM_ID_SHIFT);
	__raw_writel(reg, tsc_base + TCQCR);

	/* pen down & eoq enable */
	__raw_writel(0xffffffff, tsc_base + TCQSR);
	reg = __raw_readl(tsc_base + TCQCR);
	reg &= ~CQCR_PD_MSK;
	__raw_writel(reg, tsc_base + TCQCR);
	reg = __raw_readl(tsc_base + TCQMR);
	reg &= ~(TCQMR_PD_IRQ_MSK | TCQMR_EOQ_IRQ_MSK);
	__raw_writel(reg, tsc_base + TCQMR);

	/* Debounce time = dbtime*8 adc clock cycles */
	reg = __raw_readl(tsc_base + TGCR);
	dbtime = 3;
	reg &= ~TGCR_PDBTIME_MASK;
	reg |= dbtime << TGCR_PDBTIME_SHIFT;
	__raw_writel(reg, tsc_base + TGCR);

}

static irqreturn_t imx_adc_interrupt(int irq, void *dev_id)
{
	int reg;
	u32 status = __raw_readl(tsc_base + TCQSR);

	if (status & CQSR_PD) {
		/* disable pen down detect */
		reg = __raw_readl(tsc_base + TGCR);
		reg &= ~TGCR_PD_EN;
		__raw_writel(reg, tsc_base + TGCR);

		__raw_writel(CQSR_PD, tsc_base + TCQSR);
	}

	if (status & CQSR_EOQ) {
		/* stop the conversion */
		reg = __raw_readl(tsc_base + TCQCR);
		reg &= ~(CQCR_QSM_MASK | CQCR_FQS);
		__raw_writel(reg, tsc_base + TCQCR);

		__raw_writel(CQSR_EOQ, tsc_base + TCQSR);
		complete(&irq_wait);
	}

	return IRQ_HANDLED;
}

enum IMX_ADC_STATUS imx_adc_read_general(unsigned short *result)
{
	unsigned long reg;
	unsigned int data_num = 0;

	reg = __raw_readl(tsc_base + GCQCR);
	reg |= CQCR_FQS;
	__raw_writel(reg, tsc_base + GCQCR);

	while (!(__raw_readl(tsc_base + GCQSR) & CQSR_EOQ))
		continue;
	reg = __raw_readl(tsc_base + GCQCR);
	reg &= ~CQCR_FQS;
	__raw_writel(reg, tsc_base + GCQCR);
	reg = __raw_readl(tsc_base + GCQSR);
	reg |= CQSR_EOQ;
	__raw_writel(reg, tsc_base + GCQSR);

	while (!(__raw_readl(tsc_base + GCQSR) & CQSR_EMPT)) {
		result[data_num] = __raw_readl(tsc_base + GCQFIFO) >>
				 GCQFIFO_ADCOUT_SHIFT;
		data_num++;
	}
	return IMX_ADC_SUCCESS;
}

/*!
 * This function will get raw (X,Y) value by converting the voltage
 * @param        touch_sample Pointer to touch sample
 *
 * return        This funciton returns 0 if successful.
 *
 *
 */
enum IMX_ADC_STATUS imx_adc_read_ts(struct t_touch_screen *sample,
				    int wait_tsi)
{
	int reg;
	int i;

	if (sample == NULL)
		return IMX_ADC_PARAMETER_ERROR;

	memset(sample, 0, sizeof(struct t_touch_screen));

	if (wait_tsi) {
		/* Config idle for 4-wire */
		reg = TSC_4WIRE_TOUCH_DETECT;
		__raw_writel(reg, tsc_base + TICR);

		/* Pen interrupt starts new conversion queue */
		reg = __raw_readl(tsc_base + TCQCR);
		reg &= ~CQCR_QSM_MASK;
		reg |= CQCR_QSM_PEN;
		__raw_writel(reg, tsc_base + TCQCR);

		/* PDEN and PDBEN */
		reg = __raw_readl(tsc_base + TGCR);
		reg |= (TGCR_PDB_EN | TGCR_PD_EN);
		__raw_writel(reg, tsc_base + TGCR);

		wait_for_completion(&irq_wait);

		/* change configuration for FQS mode */
		reg = (0x1 << CC_YPLLSW_SHIFT) | (0x1 << CC_XNURSW_SHIFT) |
		      CC_XPULSW;
		__raw_writel(reg, tsc_base + TICR);
	} else {
		/* FQS semaphore */
		down(&ts_convert_mutex);

		reg = (0x1 << CC_YPLLSW_SHIFT) | (0x1 << CC_XNURSW_SHIFT) |
		      CC_XPULSW;
		__raw_writel(reg, tsc_base + TICR);

		/* FQS */
		reg = __raw_readl(tsc_base + TCQCR);
		reg &= ~CQCR_QSM_MASK;
		reg |= CQCR_QSM_FQS | CQCR_FQS;
		__raw_writel(reg, tsc_base + TCQCR);

		wait_for_completion(&irq_wait);
	}

	for (i=0; i<16; i++) {
		/* In most cases no wait, this is charm */
		while (__raw_readl(tsc_base + TCQSR) & CQSR_EMPT);

		sample->raw_data[i] = __raw_readl(tsc_base + TCQFIFO);
	}

	if (!wait_tsi)
		up(&ts_convert_mutex);

	pr_debug("DETECT1: %04x, %04x, %04x, %04x\n",
		 sample->raw_data[ 0], sample->raw_data[ 1],
		 sample->raw_data[ 2], sample->raw_data[ 3]);
	pr_debug("      X: %04x, %04x, %04x, %04x\n",
		 sample->raw_data[ 4], sample->raw_data[ 5],
		 sample->raw_data[ 6], sample->raw_data[ 7]);
	pr_debug("      Y: %04x, %04x, %04x, %04x\n",
		 sample->raw_data[ 8], sample->raw_data[ 9],
		 sample->raw_data[10], sample->raw_data[11]);
	pr_debug("DETECT2: %04x, %04x, %04x, %04x\n",
		 sample->raw_data[12], sample->raw_data[13],
		 sample->raw_data[14], sample->raw_data[15]);

	if (ADC_ID(sample->raw_data[ 0]) != DETECT_ITEM_ID_1 ||
	    ADC_ID(sample->raw_data[ 4]) != TS_X_ITEM_ID ||
	    ADC_ID(sample->raw_data[ 8]) != TS_Y_ITEM_ID ||
	    ADC_ID(sample->raw_data[12]) != DETECT_ITEM_ID_2) {
		pr_err("critical bug: unknown item_ids[%d,%d,%d,%d]\n",
		       ADC_ID(sample->raw_data[ 0]),
		       ADC_ID(sample->raw_data[ 4]),
		       ADC_ID(sample->raw_data[ 8]),
		       ADC_ID(sample->raw_data[12]));
		return IMX_ADC_ERROR;
	}

	return IMX_ADC_SUCCESS;
}

static unsigned int calc_optimum4(unsigned int *val)
{
	int i;
	unsigned int min, max, avg, lc, hc, tmp;
	min = min(min(ADC_VAL(val[0]), ADC_VAL(val[1])),
		  min(ADC_VAL(val[2]), ADC_VAL(val[3])));
	max = max(max(ADC_VAL(val[0]), ADC_VAL(val[1])),
		  max(ADC_VAL(val[2]), ADC_VAL(val[3])));

	avg = (min + max) / 2;
	for (i=0, lc=0, hc=0; i<4; i++)
		ADC_VAL(val[i]) >= avg ? hc++: lc++;
	pr_debug("min:%04x, max:%04x, avg:%04x, hc:%d, lc:%d\n",
		 min, max, avg, hc, lc);
	if (hc == lc)
		return (ADC_VAL(val[0]) + ADC_VAL(val[1]) +
			ADC_VAL(val[2]) + ADC_VAL(val[3])) / 4;

	tmp = 0;
	if (hc > lc) {
		for (i=0; i<4; i++)
			tmp += ADC_VAL(val[i]) >= avg ? ADC_VAL(val[i]) : 0;
		return tmp / hc;
	} else {
		for (i=0; i<4; i++)
			tmp += ADC_VAL(val[i]) < avg ? ADC_VAL(val[i]) : 0;
		return tmp / lc;
	}
}

/*!
 * This function performs filtering and rejection of excessive noise prone
 * sampl.
 *
 * @param        sample     Touch screen value
 *
 * @return       This function returns 0 on success, -1 otherwise.
 */
static int imx_adc_filter(struct t_touch_screen *sample)
{
	sample->flag = TSF_VALID;
	if (ADC_VAL(sample->raw_data[ 0]) >= TS_DETECT_WM ||
	    ADC_VAL(sample->raw_data[ 1]) >= TS_DETECT_WM ||
	    ADC_VAL(sample->raw_data[ 2]) >= TS_DETECT_WM ||
	    ADC_VAL(sample->raw_data[ 3]) >= TS_DETECT_WM ||
	    ADC_VAL(sample->raw_data[12]) >= TS_DETECT_WM ||
	    ADC_VAL(sample->raw_data[13]) >= TS_DETECT_WM ||
	    ADC_VAL(sample->raw_data[14]) >= TS_DETECT_WM ||
	    ADC_VAL(sample->raw_data[15]) >= TS_DETECT_WM) {
		sample->flag &= ~TSF_VALID;
		return 0;
	}

	sample->pos_x = calc_optimum4(&sample->raw_data[ 4]);
	sample->pos_y = calc_optimum4(&sample->raw_data[ 8]);

	return 0;
}

/*!
 * This function retrieves the current touch screen (X,Y) coordinates.
 *
 * @param        touch_sample Pointer to touch sample.
 *
 * @return       This function returns IMX_ADC_SUCCESS if successful.
 */
enum IMX_ADC_STATUS imx_adc_get_touch_sample(struct t_touch_screen
					     *touch_sample, int wait_tsi)
{
	if (imx_adc_read_ts(touch_sample, wait_tsi))
		return IMX_ADC_ERROR;
	if (!imx_adc_filter(touch_sample))
		return IMX_ADC_SUCCESS;
	else
		return IMX_ADC_ERROR;
}
EXPORT_SYMBOL(imx_adc_get_touch_sample);

/*!
 * This is the suspend of power management for the i.MX ADC API.
 * It supports SAVE and POWER_DOWN state.
 *
 * @param        pdev           the device
 * @param        state          the state
 *
 * @return       This function returns 0 if successful.
 */
static int imx_adc_suspend(struct platform_device *pdev, pm_message_t state)
{
	suspend_flag = 1;

	if (device_may_wakeup(&pdev->dev) ||
	    (ts_dev && device_may_wakeup(ts_dev)))
		enable_irq_wake(adc_data->irq);
	else
		tsc_clk_disable();

	return 0;
};

/*!
 * This is the resume of power management for the i.MX adc API.
 * It supports RESTORE state.
 *
 * @param        pdev           the device
 *
 * @return       This function returns 0 if successful.
 */
static int imx_adc_resume(struct platform_device *pdev)
{
	suspend_flag = 0;

	if (device_may_wakeup(&pdev->dev) ||
	    (ts_dev && device_may_wakeup(ts_dev)))
		disable_irq_wake(adc_data->irq);
	else
		tsc_clk_enable();

	while (swait > 0) {
		swait--;
		wake_up_interruptible(&suspendq);
	}

	return 0;
}

/*!
 * This function implements the open method on an i.MX ADC device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int imx_adc_open(struct inode *inode, struct file *file)
{
	while (suspend_flag) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, !suspend_flag))
			return -ERESTARTSYS;
	}
	pr_debug("imx_adc : imx_adc_open()\n");
	return 0;
}

/*!
 * This function implements the release method on an i.MX ADC device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int imx_adc_free(struct inode *inode, struct file *file)
{
	pr_debug("imx_adc : imx_adc_free()\n");
	return 0;
}

/*!
 * This function initializes all ADC registers with default values. This
 * function also registers the interrupt events.
 *
 * @return       This function returns IMX_ADC_SUCCESS if successful.
 */
static enum IMX_ADC_STATUS imx_adc_init(void)
{
	int reg;

	pr_debug("imx_adc_init()\n");

	if (suspend_flag)
		return -EBUSY;

	tsc_clk_enable();

	/* Reset */
	tsc_self_reset();

#if defined(CONFIG_USE_INTERNAL_REF_FOR_GENERAL_ADC)
	/* Internal reference */
	tsc_intref_enable();
#endif

	/* Set power mode */
	reg = __raw_readl(tsc_base + TGCR) & ~TGCR_POWER_MASK;
	reg |= TGCR_POWER_SAVE;
	__raw_writel(reg, tsc_base + TGCR);

	imx_tsc_init();

	return IMX_ADC_SUCCESS;
}

/*!
 * This function disables the ADC, de-registers the interrupt events.
 *
 * @return       This function returns IMX_ADC_SUCCESS if successful.
 */
static enum IMX_ADC_STATUS imx_adc_deinit(void)
{
	pr_debug("imx_adc_deinit()\n");

	return IMX_ADC_SUCCESS;
}

/*!
 * This function triggers a conversion and returns one sampling result of one
 * channel.
 *
 * @param        channel   The channel to be sampled
 * @param        nr_samples Number of samples
 * @param        result    The pointer to the conversion result. The memory
 *                         should be allocated by the caller of this function.
 *
 * @return       This function returns IMX_ADC_SUCCESS if successful.
 */
static enum IMX_ADC_STATUS imx_adc_convert(enum t_channel channel,
					   int nr_samples,
					   unsigned short *result)
{
	int reg;
	int lastitemid;
	struct t_touch_screen touch_sample;

	switch (channel) {

	case TS_X_POS:
		imx_adc_get_touch_sample(&touch_sample, 1);
		result[0] = touch_sample.pos_x;
		break;

	case TS_Y_POS:
		imx_adc_get_touch_sample(&touch_sample, 1);
		result[1] = touch_sample.pos_y;
		break;

	case GER_PURPOSE_ADC0:
		if (nr_samples <= 0 || IMX_ADC_MAX_SAMPLE < nr_samples)
			return IMX_ADC_PARAMETER_ERROR;
		down(&general_convert_mutex);

		lastitemid = 0;
		reg = (0xf << CQCR_FIFOWATERMARK_SHIFT) |
		      (lastitemid << CQCR_LAST_ITEM_ID_SHIFT) | CQCR_QSM_FQS;
		__raw_writel(reg, tsc_base + GCQCR);

		reg = TSC_GENERAL_ADC_GCC0;
#if !defined(CONFIG_USE_INTERNAL_REF_FOR_GENERAL_ADC)
		reg &= ~CC_SELREFP_MASK;
		reg |= CC_SELREFP_EXT;
#endif
		reg |= ((nr_samples-1) << CC_NOS_SHIFT) | (16 << CC_SETTLING_TIME_SHIFT);
		__raw_writel(reg, tsc_base + GCC0);

		imx_adc_read_general(result);
		up(&general_convert_mutex);
		break;

	case GER_PURPOSE_ADC1:
		if (nr_samples <= 0 || IMX_ADC_MAX_SAMPLE < nr_samples)
			return IMX_ADC_PARAMETER_ERROR;
		down(&general_convert_mutex);

		lastitemid = 0;
		reg = (0xf << CQCR_FIFOWATERMARK_SHIFT) |
		      (lastitemid << CQCR_LAST_ITEM_ID_SHIFT) | CQCR_QSM_FQS;
		__raw_writel(reg, tsc_base + GCQCR);

		reg = TSC_GENERAL_ADC_GCC1;
#if !defined(CONFIG_USE_INTERNAL_REF_FOR_GENERAL_ADC)
		reg &= ~CC_SELREFP_MASK;
		reg |= CC_SELREFP_EXT;
#endif
		reg |= ((nr_samples-1) << CC_NOS_SHIFT) | (16 << CC_SETTLING_TIME_SHIFT);
		__raw_writel(reg, tsc_base + GCC0);

		imx_adc_read_general(result);
		up(&general_convert_mutex);
		break;

	case GER_PURPOSE_ADC2:
		if (nr_samples <= 0 || IMX_ADC_MAX_SAMPLE < nr_samples)
			return IMX_ADC_PARAMETER_ERROR;
		down(&general_convert_mutex);

		lastitemid = 0;
		reg = (0xf << CQCR_FIFOWATERMARK_SHIFT) |
		      (lastitemid << CQCR_LAST_ITEM_ID_SHIFT) | CQCR_QSM_FQS;
		__raw_writel(reg, tsc_base + GCQCR);

		reg = TSC_GENERAL_ADC_GCC2;
#if !defined(CONFIG_USE_INTERNAL_REF_FOR_GENERAL_ADC)
		reg &= ~CC_SELREFP_MASK;
		reg |= CC_SELREFP_EXT;
#endif
		reg |= ((nr_samples-1) << CC_NOS_SHIFT) | (16 << CC_SETTLING_TIME_SHIFT);
		__raw_writel(reg, tsc_base + GCC0);

		imx_adc_read_general(result);
		up(&general_convert_mutex);
		break;

	case GER_PURPOSE_MULTICHNNEL:
		if (nr_samples <= 0 || IMX_ADC_MAX_SAMPLE < nr_samples*3)
			return IMX_ADC_PARAMETER_ERROR;
		down(&general_convert_mutex);

		reg = TSC_GENERAL_ADC_GCC0;
		reg |= ((nr_samples-1) << CC_NOS_SHIFT) | (16 << CC_SETTLING_TIME_SHIFT);
		__raw_writel(reg, tsc_base + GCC0);

		reg = TSC_GENERAL_ADC_GCC1;
		reg |= ((nr_samples-1) << CC_NOS_SHIFT) | (16 << CC_SETTLING_TIME_SHIFT);
		__raw_writel(reg, tsc_base + GCC1);

		reg = TSC_GENERAL_ADC_GCC2;
		reg |= ((nr_samples-1) << CC_NOS_SHIFT) | (16 << CC_SETTLING_TIME_SHIFT);
		__raw_writel(reg, tsc_base + GCC2);

		reg = (GCQ_ITEM_GCC2 << GCQ_ITEM2_SHIFT) |
		      (GCQ_ITEM_GCC1 << GCQ_ITEM1_SHIFT) |
		      (GCQ_ITEM_GCC0 << GCQ_ITEM0_SHIFT);
		__raw_writel(reg, tsc_base + GCQ_ITEM_7_0);

		lastitemid = 2;
		reg = (0xf << CQCR_FIFOWATERMARK_SHIFT) |
		      (lastitemid << CQCR_LAST_ITEM_ID_SHIFT) | CQCR_QSM_FQS;
		__raw_writel(reg, tsc_base + GCQCR);

		imx_adc_read_general(result);
		up(&general_convert_mutex);
		break;
	default:
		pr_debug("%s: bad channel number\n", __func__);
		return IMX_ADC_ERROR;
	}

	return IMX_ADC_SUCCESS;
}

/*!
 * This function triggers a conversion and returns sampling results of each
 * specified channel.
 *
 * @param        channels  This input parameter is bitmap to specify channels
 *                         to be sampled.
 * @param        nr_samples Number of samples
 * @param        result    The pointer to array to store sampling results.
 *                         The memory should be allocated by the caller of this
 *                         function.
 *
 * @return       This function returns IMX_ADC_SUCCESS if successful.
 */
static enum IMX_ADC_STATUS imx_adc_convert_multichnnel(enum t_channel channels,
						       int nr_samples,
						       unsigned short *result)
{
	imx_adc_convert(GER_PURPOSE_MULTICHNNEL, nr_samples, result);
	return IMX_ADC_SUCCESS;
}

/*!
 * This function implements IOCTL controls on an i.MX ADC device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @param        cmd         the command
 * @param        arg         the parameter
 * @return       This function returns 0 if successful.
 */
static int imx_adc_ioctl(struct inode *inode, struct file *file,
			 unsigned int cmd, unsigned long arg)
{
	struct t_adc_convert_param *convert_param;

	if ((_IOC_TYPE(cmd) != 'p') && (_IOC_TYPE(cmd) != 'D'))
		return -ENOTTY;

	while (suspend_flag) {
		swait++;
		/* Block if the device is suspended */
		if (wait_event_interruptible(suspendq, !suspend_flag))
			return -ERESTARTSYS;
	}

	switch (cmd) {
	case IMX_ADC_INIT:
		pr_debug("init adc\n");
		CHECK_ERROR(imx_adc_init());
		break;

	case IMX_ADC_DEINIT:
		pr_debug("deinit adc\n");
		CHECK_ERROR(imx_adc_deinit());
		break;

	case IMX_ADC_CONVERT:
		convert_param = kmalloc(sizeof(*convert_param), GFP_KERNEL);
		if (convert_param == NULL)
			return -ENOMEM;
		if (copy_from_user(convert_param,
				   (struct t_adc_convert_param *)arg,
				   sizeof(*convert_param))) {
			kfree(convert_param);
			return -EFAULT;
		}
		CHECK_ERROR_KFREE(imx_adc_convert(convert_param->channel,
						  convert_param->nr_samples,
						  convert_param->result),
				  (kfree(convert_param)));

		if (copy_to_user((struct t_adc_convert_param *)arg,
				 convert_param, sizeof(*convert_param))) {
			kfree(convert_param);
			return -EFAULT;
		}
		kfree(convert_param);
		break;

	case IMX_ADC_CONVERT_MULTICHANNEL:
		convert_param = kmalloc(sizeof(*convert_param), GFP_KERNEL);
		if (convert_param == NULL)
			return -ENOMEM;
		if (copy_from_user(convert_param,
				   (struct t_adc_convert_param *)arg,
				   sizeof(*convert_param))) {
			kfree(convert_param);
			return -EFAULT;
		}
		CHECK_ERROR_KFREE(imx_adc_convert_multichnnel
				  (convert_param->channel,
				   convert_param->nr_samples,
				   convert_param->result),
				  (kfree(convert_param)));

		if (copy_to_user((struct t_adc_convert_param *)arg,
				 convert_param, sizeof(*convert_param))) {
			kfree(convert_param);
			return -EFAULT;
		}
		kfree(convert_param);
		break;

	default:
		pr_debug("imx_adc_ioctl: unsupported ioctl command 0x%x\n",
			 cmd);
		return -EINVAL;
	}
	return 0;
}

static struct file_operations imx_adc_fops = {
	.owner = THIS_MODULE,
	.ioctl = imx_adc_ioctl,
	.open = imx_adc_open,
	.release = imx_adc_free,
};

static int imx_adc_module_probe(struct platform_device *pdev)
{
	int ret = 0;
	int retval;
	struct device *temp_class;
	struct resource *res;
	void __iomem *base;
	struct platform_imx_adc_data *plat_data = pdev->dev.platform_data;

	/* ioremap the base address */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "No TSC base address provided\n");
		goto err_out0;
	}
	base = ioremap(res->start, res->end - res->start);
	if (base == NULL) {
		dev_err(&pdev->dev, "failed to rebase TSC base address\n");
		goto err_out0;
	}
	tsc_base = (unsigned long)base;

	/* create the chrdev */
	imx_adc_major = register_chrdev(0, "imx_adc", &imx_adc_fops);

	if (imx_adc_major < 0) {
		dev_err(&pdev->dev, "Unable to get a major for imx_adc\n");
		return imx_adc_major;
	}
	init_waitqueue_head(&suspendq);
	init_waitqueue_head(&tsq);
	init_completion(&irq_wait);

	imx_adc_class = class_create(THIS_MODULE, "imx_adc");
	if (IS_ERR(imx_adc_class)) {
		dev_err(&pdev->dev, "Error creating imx_adc class.\n");
		ret = PTR_ERR(imx_adc_class);
		goto err_out1;
	}

	temp_class = device_create(imx_adc_class, NULL,
				   MKDEV(imx_adc_major, 0), "imx_adc");
	if (IS_ERR(temp_class)) {
		dev_err(&pdev->dev, "Error creating imx_adc class device.\n");
		ret = PTR_ERR(temp_class);
		goto err_out2;
	}

	adc_data = kmalloc(sizeof(struct imx_adc_data), GFP_KERNEL);
	if (adc_data == NULL)
		return -ENOMEM;
	adc_data->irq = platform_get_irq(pdev, 0);
	retval = request_irq(adc_data->irq, imx_adc_interrupt,
			     0, MOD_NAME, MOD_NAME);
	if (retval) {
		dev_dbg(&pdev->dev, "ADC: request_irq(%d) returned error %d\n",
			MXC_INT_TSC, retval);
		return retval;
	}

	device_init_wakeup(&pdev->dev, 1);
	if (plat_data && !plat_data->is_wake_src)
		device_set_wakeup_enable(&pdev->dev, 0);

	adc_data->adc_clk = clk_get(&pdev->dev, "tchscrn_clk");

	ret = imx_adc_init();

	if (ret != IMX_ADC_SUCCESS) {
		dev_err(&pdev->dev, "Error in imx_adc_init.\n");
		goto err_out4;
	}
	imx_adc_ready = 1;
	pr_info("i.MX ADC at 0x%x irq %d\n", (unsigned int)res->start,
		adc_data->irq);
	return ret;

err_out4:
	device_destroy(imx_adc_class, MKDEV(imx_adc_major, 0));
err_out2:
	class_destroy(imx_adc_class);
err_out1:
	unregister_chrdev(imx_adc_major, "imx_adc");
err_out0:
	return ret;
}

static int imx_adc_module_remove(struct platform_device *pdev)
{
	imx_adc_ready = 0;
	imx_adc_deinit();
	device_destroy(imx_adc_class, MKDEV(imx_adc_major, 0));
	class_destroy(imx_adc_class);
	unregister_chrdev(imx_adc_major, "imx_adc");
	free_irq(adc_data->irq, MOD_NAME);
	kfree(adc_data);
	pr_debug("i.MX ADC successfully removed\n");
	return 0;
}

static struct platform_driver imx_adc_driver = {
	.driver = {
		   .name = "imx_adc",
		   },
	.suspend = imx_adc_suspend,
	.resume = imx_adc_resume,
	.probe = imx_adc_module_probe,
	.remove = imx_adc_module_remove,
};

/*
 * Initialization and Exit
 */
static int __init imx_adc_module_init(void)
{
	pr_debug("i.MX ADC driver loading...\n");
	return platform_driver_register(&imx_adc_driver);
}

static void __exit imx_adc_module_exit(void)
{
	platform_driver_unregister(&imx_adc_driver);
	pr_debug("i.MX ADC driver successfully unloaded\n");
}

/*
 * Module entry points
 */

module_init(imx_adc_module_init);
module_exit(imx_adc_module_exit);

MODULE_DESCRIPTION("i.MX ADC device driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
