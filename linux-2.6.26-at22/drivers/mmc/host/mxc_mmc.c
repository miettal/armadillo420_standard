/*
 * linux/drivers/mmc/host/mxc_mmc.c - Freescale MXC/i.MX MMC driver
 *
 * Copyright 2004-2008 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This driver code is based on imxmmc.c,
 *		by Sascha Hauer, Pengutronix <sascha@saschahauer.de>.
 *
 * This driver supports both Secure Digital
 * Host Controller modules (SDHC1 and SDHC2) of MXC. SDHC is also referred as
 * MMC/SD controller. This code is not tested for SD cards.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/* Include Files */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/blkdev.h>
#include <linux/dma-mapping.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/clk.h>

#include <asm/dma.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/sizes.h>
#include <asm/mach-types.h>
#include <asm/mach/irq.h>
#include <asm/arch/mmc.h>

#include "mxc_mmc.h"

#define RSP_TYPE(x)	((x) & ~(MMC_RSP_BUSY|MMC_RSP_OPCODE))

/* Maxumum length of s/g list, only length of 1 is currently supported */
#define NR_SG   1

/* Wait count to start/stop the clock */
#define CMD_WAIT_CNT 100

/*
 * This structure is a way for the low level driver to define their own
 * mmc_host structure. This structure includes the core  mmc_host
 * structure that is provided by Linux MMC/SD Bus protocol driver as an
 * element and has other elements that are specifically required by this
 * low-level driver.
 */
struct mxcmci_priv {
	/* The mmc structure holds all the information about the device
	 * structure, current SDHC io bus settings, the current OCR setting,
	 * devices attached to this host, and so on. */
	struct mmc_host *host;

	/* This variable is used for locking the host data structure from
	 * multiple access. */
	spinlock_t lock;

	/* Resource structure, which will maintain base addresses and IRQs. */
	struct resource *res;

	/* Base address of SDHC, used in readl and writel. */
	void *base;

	/* SDHC IRQ number. */
	int irq;

	/* Card Detect IRQ number. */
	int detect_irq;

	/* Clock id to hold ipg_perclk. */
	struct clk *clk;

	/* MMC mode. */
	int mode;

	/* DMA channel number. */
	int dma, dma_1bit, dma_4bit;

	/* Pointer to hold MMC/SD request. */
	struct mmc_request *req;

	/* Pointer to hold MMC/SD command. */
	struct mmc_command *cmd;

	/* Pointer to hold MMC/SD data. */
	struct mmc_data *data;

	/* Holds the number of bytes to transfer using DMA. */
	unsigned int dma_size;

	/* Length of the scatter-gather list */
	unsigned int dma_len;

	/* Holds the direction of data transfer. */
	unsigned int dma_dir;

	/* Id for MMC block. */
	unsigned int id;

	/* Note whether this driver has been suspended. */
	unsigned int mxc_mmc_suspend_flag;

	/* Completion to wait for interrupts */
	struct completion comp_cmd_done;
	struct completion comp_dma_done;
	struct completion comp_read_op_done;
	struct completion comp_write_op_done;

	/* Platform specific data */
	struct mxc_mmc_platform_data *plat_data;
};

extern void gpio_sdhc_active(int module);
extern void gpio_sdhc_inactive(int module);

static void mxcmci_dma_irq(void *devid, int error, unsigned int cnt);
static int mxcmci_data_done(struct mxcmci_priv *priv);
static int mxcmci_cmd_done(struct mxcmci_priv *priv, unsigned int stat);

#if defined(CONFIG_MXC_MC13783_POWER)
#include <asm/arch/pmic_power.h>

static const int vdd_mapping[] = {
	0, 0,
	0,		/* MMC_VDD_160 */
	0, 0,
	1,		/* MMC_VDD_180 */
	0,
	2,		/* MMC_VDD_200 */
	0, 0, 0, 0, 0,
	3,		/* MMC_VDD_260 */
	4,		/* MMC_VDD_270 */
	5,		/* MMC_VDD_280 */
	6,		/* MMC_VDD_290 */
	7,		/* MMC_VDD_300 */
	7,		/* MMC_VDD_310 - HACK for LP1070, actually 3.0V */
	7,		/* MMC_VDD_320 - HACK for LP1070, actually 3.0V */
	0, 0, 0, 0
};

static void mxcmci_mc13783_power_ctrl(struct mmc_host *host,
				      struct mmc_ios *ios)
{
	struct mxcmci_priv *priv = mmc_priv(host);
	t_regulator_voltage voltage;

	switch (ios->power_mode) {
	case MMC_POWER_UP:
		if (priv->id == 0) {
			voltage.vmmc1 = vdd_mapping[ios->vdd];
			pmic_power_regulator_set_voltage(REGU_VMMC1, voltage);
			pmic_power_regulator_set_lp_mode(REGU_VMMC1,
							 LOW_POWER_DISABLED);
			pmic_power_regulator_on(REGU_VMMC1);
		}
		if (priv->id == 1) {
			voltage.vmmc2 = vdd_mapping[ios->vdd];
			pmic_power_regulator_set_voltage(REGU_VMMC2, voltage);
			pmic_power_regulator_set_lp_mode(REGU_VMMC2,
							 LOW_POWER_DISABLED);
			pmic_power_regulator_on(REGU_VMMC2);
		}
		dev_dbg(priv->host->parent, "mmc power on\n");
		msleep(300);
		break;
	case MMC_POWER_OFF:
		if (priv->id == 0) {
			pmic_power_regulator_set_lp_mode(REGU_VMMC1,
							 LOW_POWER_EN);
			pmic_power_regulator_off(REGU_VMMC1);
		}

		if (priv->id == 1) {
			pmic_power_regulator_set_lp_mode(REGU_VMMC2,
							 LOW_POWER_EN);
			pmic_power_regulator_off(REGU_VMMC2);
		}
		dev_dbg(priv->host->parent, "mmc power off\n");
		break;
	default:
		break;
	}
}

static void (*ext_power_ctrl)(struct mmc_host *, struct mmc_ios *) =
	mxcmci_mc13783_power_ctrl;
#else
static void mxcmci_dummy_power_ctrl(struct mmc_host *host,
				    struct mmc_ios *ios)
{
}

static void (*ext_power_ctrl)(struct mmc_host *, struct mmc_ios *) =
	mxcmci_dummy_power_ctrl;
#endif

static void dump_cmd(struct mmc_command *cmd)
{
#ifdef CONFIG_MMC_DEBUG
	pr_info("%s: CMD: opcode: %d, arg: 0x%08x, flags: 0x%08x\n",
		DRIVER_NAME, cmd->opcode, cmd->arg, cmd->flags);
#endif
}

static void dump_status(const char *func, int status)
{
#ifdef CONFIG_MMC_DEBUG
	unsigned int bitset;
	char buf[2048];
	char *ptr = buf;

	ptr += sprintf(ptr, "dump status: ");

	while (status) {
		/* Find the next bit set */
		bitset = status & ~(status - 1);
		switch (bitset) {
		case STATUS_CARD_INSERTION:
			ptr += sprintf(ptr, "CARD_INSERTION,");
			break;
		case STATUS_CARD_REMOVAL:
			ptr += sprintf(ptr, "CARD_REMOVAL,");
			break;
		case STATUS_YBUF_EMPTY:
			ptr += sprintf(ptr, "YBUF_EMPTY,");
			break;
		case STATUS_XBUF_EMPTY:
			ptr += sprintf(ptr, "XBUF_EMPTY,");
			break;
		case STATUS_YBUF_FULL:
			ptr += sprintf(ptr, "YBUF_FULL,");
			break;
		case STATUS_XBUF_FULL:
			ptr += sprintf(ptr, "XBUF_FULL,");
			break;
		case STATUS_BUF_UND_RUN:
			ptr += sprintf(ptr, "BUF_UND_RUN,");
			break;
		case STATUS_BUF_OVFL:
			ptr += sprintf(ptr, "BUF_OVFL,");
			break;
		case STATUS_READ_OP_DONE:
			ptr += sprintf(ptr, "READ_OP_DONE,");
			break;
		case STATUS_WR_CRC_ERROR_CODE_MASK:
			ptr += sprintf(ptr, "WR_CRC_ERROR_CODE,");
			break;
		case STATUS_READ_CRC_ERR:
			ptr += sprintf(ptr, "READ_CRC_ERR,");
			break;
		case STATUS_WRITE_CRC_ERR:
			ptr += sprintf(ptr, "WRITE_CRC_ERR,");
			break;
		case STATUS_SDIO_INT_ACTIVE:
			ptr += sprintf(ptr, "SDIO_INT_ACTIVE,");
			break;
		case STATUS_END_CMD_RESP:
			ptr += sprintf(ptr, "END_CMD_RESP,");
			break;
		case STATUS_WRITE_OP_DONE:
			ptr += sprintf(ptr, "WRITE_OP_DONE,");
			break;
		case STATUS_CARD_BUS_CLK_RUN:
			ptr += sprintf(ptr, "CARD_BUS_CLK_RUN,");
			break;
		case STATUS_BUF_READ_RDY:
			ptr += sprintf(ptr, "BUF_READ_RDY,");
			break;
		case STATUS_BUF_WRITE_RDY:
			ptr += sprintf(ptr, "BUF_WRITE_RDY,");
			break;
		case STATUS_RESP_CRC_ERR:
			ptr += sprintf(ptr, "RESP_CRC_ERR,");
			break;
		case STATUS_TIME_OUT_RESP:
			ptr += sprintf(ptr, "TIME_OUT_RESP,");
			break;
		case STATUS_TIME_OUT_READ:
			ptr += sprintf(ptr, "TIME_OUT_READ,");
			break;
		default:
			ptr += sprintf(ptr, "invalid value=0x%x", bitset);
			break;
		}
		status &= ~bitset;
	}

	pr_info("%s\n", buf);
#endif
}

static void mxcmci_interrupt_enable(struct mxcmci_priv *priv, u32 val)
{
	u32 intctrl = readl(priv->base + MMC_INT_CNTR);
	intctrl |= val;
	writel(intctrl, priv->base + MMC_INT_CNTR);
}

static void mxcmci_interrupt_disable(struct mxcmci_priv *priv, u32 val)
{
	u32 intctrl = readl(priv->base + MMC_INT_CNTR);
	intctrl &= ~val;
	writel(intctrl, priv->base + MMC_INT_CNTR);
}

/*
 * This function sets the SDHC register to stop the clock and waits for the
 * clock stop indication.
 */
static void mxcmci_stop_clock(struct mxcmci_priv *priv)
{
	int wait_cnt = 0;

	while (1) {
		__raw_writel(STR_STP_CLK_IPG_CLK_GATE_DIS |
			     STR_STP_CLK_IPG_PERCLK_GATE_DIS |
			     STR_STP_CLK_STOP_CLK,
			     priv->base + MMC_STR_STP_CLK);

		wait_cnt = CMD_WAIT_CNT;
		while (wait_cnt--) {
			if (!(__raw_readl(priv->base + MMC_STATUS) &
			      STATUS_CARD_BUS_CLK_RUN))
				break;
		}
		/* When we were going to stop SDCLK, it has the case
		 * that does not stop. */
		if (wait_cnt == 0)
			break;

		if (!(__raw_readl(priv->base + MMC_STATUS) &
		      STATUS_CARD_BUS_CLK_RUN))
			break;
	}
}

/*
 * This function sets the SDHC register to start the clock and waits for the
 * clock start indication. When the clock starts SDHC module starts processing
 * the command in CMD Register with arguments in ARG Register.
 *
 * @param priv Pointer to MMC/SD priv structure
 */
static void mxcmci_start_clock(struct mxcmci_priv *priv)
{
	int wait_cnt;

	while (1) {
		__raw_writel(STR_STP_CLK_IPG_CLK_GATE_DIS |
			     STR_STP_CLK_IPG_PERCLK_GATE_DIS |
			     STR_STP_CLK_START_CLK,
			     priv->base + MMC_STR_STP_CLK);

		wait_cnt = CMD_WAIT_CNT;
		while (wait_cnt--) {
			if (__raw_readl(priv->base + MMC_STATUS) &
			    STATUS_CARD_BUS_CLK_RUN) {
				break;
			}
		}
		if (wait_cnt == 0)
			break;

		if (__raw_readl(priv->base + MMC_STATUS) &
		    STATUS_CARD_BUS_CLK_RUN) {
			break;
		}
	}
}

static void mxcmci_setup_dma(struct mxcmci_priv *priv)
{
	switch (priv->id) {
	case 0:
		priv->dma_1bit = mxc_dma_request(MXC_DMA_MMC1_WIDTH_1,
						 "MMC1-1bit");
		priv->dma_4bit = mxc_dma_request(MXC_DMA_MMC1_WIDTH_4,
						 "MMC1-4bit");
		break;
	case 1:
		priv->dma_1bit = mxc_dma_request(MXC_DMA_MMC2_WIDTH_1,
						 "MMC2-1bit");
		priv->dma_4bit = mxc_dma_request(MXC_DMA_MMC2_WIDTH_4,
						 "MMC2-4bit");
		break;
	default:
		dev_err(priv->host->parent, "unknown dma-id(%d)\n", priv->id);
		return;
	}

	mxc_dma_callback_set(priv->dma_1bit, mxcmci_dma_irq, (void *)priv);
	mxc_dma_callback_set(priv->dma_4bit, mxcmci_dma_irq, (void *)priv);
}

/*
 * This function resets the SDHC host.
 *
 * @param priv  Pointer to MMC/SD  priv structure
 */
static void mxcmci_softreset(struct mxcmci_priv *priv)
{
	__raw_writel(0x8, priv->base + MMC_STR_STP_CLK);
	__raw_writel(0x9, priv->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, priv->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, priv->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, priv->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, priv->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, priv->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, priv->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, priv->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, priv->base + MMC_STR_STP_CLK);
	__raw_writel(0x3f, priv->base + MMC_CLK_RATE);

	__raw_writel(0xff, priv->base + MMC_RES_TO);
	__raw_writel(512, priv->base + MMC_BLK_LEN);
	__raw_writel(1, priv->base + MMC_NOB);
}

/*
 * This function sets up the number of blocks and block length registers.
 *
 * @param priv  Pointer to MMC/SD priv structure
 * @param data  Pointer to MMC/SD data structure
 */
static void mxcmci_setup_data(struct mxcmci_priv *priv, struct mmc_data *data)
{
	unsigned int nob = data->blocks;

	if (data->flags & MMC_DATA_STREAM)
		nob = 0xffff;

	priv->data = data;

	__raw_writel(nob, priv->base + MMC_NOB);
	__raw_writel(data->blksz, priv->base + MMC_BLK_LEN);

	priv->dma_size = data->blocks * data->blksz;
	dev_dbg(priv->host->parent, "Request bytes to transfer: %d\n",
		priv->dma_size);
}

/*
 * This function sets up the SDHC registers in order to issue a command.
 *
 * @param priv  Pointer to MMC/SD priv structure
 * @param cmd   Pointer to MMC/SD command structure
 * @param cmdat Value to store in the Command and Data Control registers
 */
static void mxcmci_start_cmd(struct mxcmci_priv *priv, struct mmc_command *cmd,
			     unsigned int cmdat)
{
	unsigned long flags;
	int timeout;
	int ret;

	WARN_ON(priv->cmd != NULL);
	priv->cmd = cmd;

	switch (RSP_TYPE(mmc_resp_type(cmd))) {
	case RSP_TYPE(MMC_RSP_R1): /* r1, r1b, r6 */
		cmdat |= CMD_DAT_CONT_RESPONSE_FORMAT_R1;
		break;
	case RSP_TYPE(MMC_RSP_R3):
		cmdat |= CMD_DAT_CONT_RESPONSE_FORMAT_R3;
		break;
	case RSP_TYPE(MMC_RSP_R2):
		cmdat |= CMD_DAT_CONT_RESPONSE_FORMAT_R2;
		break;
	default:
		/* No Response required */
		break;
	}

	if (cmd->opcode == MMC_GO_IDLE_STATE)
		cmdat |= CMD_DAT_CONT_INIT; /* This command needs init */

	if (priv->host->ios.bus_width == MMC_BUS_WIDTH_4)
		cmdat |= CMD_DAT_CONT_BUS_WIDTH_4;

	local_irq_save(flags);
	mxcmci_start_clock(priv);

	__raw_writel(cmd->opcode, priv->base + MMC_CMD);
	__raw_writel(cmd->arg, priv->base + MMC_ARG);

	__raw_writel(cmdat, priv->base + MMC_CMD_DAT_CONT);
	local_irq_restore(flags);

	timeout = wait_for_completion_timeout(&priv->comp_cmd_done,
					      msecs_to_jiffies(1000));
	if (timeout == 0) {
		dev_err(priv->host->parent, "wait cmd_done timeout\n");
		cmd->error = -ETIMEDOUT;
	}

	ret = mxcmci_cmd_done(priv, 0);
	if (ret)
		return;
}

/*
 * This function is called to complete the command request.
 *
 * @param priv Pointer to MMC/SD priv structure
 * @param req  Pointer to MMC/SD command request structure
 */
static void mxcmci_finish_request(struct mxcmci_priv *priv,
				  struct mmc_request *req)
{
	priv->req = NULL;
	priv->cmd = NULL;
	priv->data = NULL;

	if (priv->host->card
	    && (priv->host->card->state & MMC_STATE_PRESENT)) {
		if (!(priv->host->card->ext_caps &
		      MMC_CARD_CAPS_FORCE_CLK_KEEP))
			mxcmci_stop_clock(priv);
	} else {
		mxcmci_stop_clock(priv);
	}

	mmc_request_done(priv->host, req);
}

static int mxcmci_pio_data_transfer(struct mxcmci_priv *priv)
{
	struct mmc_data *data = priv->data;
	unsigned long *buf;
	u8 *buf8;
	int no_of_bytes;
	int no_of_words;
	unsigned long timeout_jiffies;
	int i;
	u32 temp_data;
	long timeout;

	buf = (unsigned long *)(sg_virt(data->sg));
	buf8 = (u8 *)buf;

	/* calculate the number of bytes and words requested for transfer */
	no_of_bytes = data->blocks * data->blksz;
	no_of_words = (no_of_bytes + 3) / 4;
	dev_dbg(priv->host->parent, "no_of_words = %d\n", no_of_words);

	if (data->flags & MMC_DATA_READ) {
		timeout_jiffies = jiffies + msecs_to_jiffies(1000);
		for (i = 0; i < no_of_words; i++) {
			while (1) {
				if (__raw_readl(priv->base + MMC_STATUS) &
				    (STATUS_BUF_READ_RDY | STATUS_READ_OP_DONE))
					break;

				if (time_is_before_jiffies(timeout_jiffies)) {
					dev_err(priv->host->parent,
						"wait read ready timeout\n");
					data->error = -ETIMEDOUT;
					break;
				}
			}

			temp_data = __raw_readl(priv->base + MMC_BUFFER_ACCESS);
			if (no_of_bytes >= 4) {
				*buf++ = temp_data;
				no_of_bytes -= 4;
			} else {
				do {
					*buf8++ = temp_data;
					temp_data = temp_data >> 8;
				} while (--no_of_bytes);
			}
		}

		if (!data->error) {
			mxcmci_interrupt_enable(priv, INT_CNTR_READ_OP_DONE);

			timeout = wait_for_completion_timeout(&priv->comp_read_op_done,
							      msecs_to_jiffies(1000));
			if (timeout == 0) {
				dev_err(priv->host->parent,
					"wait read_op_done timeout\n");
				data->error = -ETIMEDOUT;
			}
		}
	} else {
		for (i = 0; i < no_of_words; i++) {
			timeout_jiffies = jiffies + msecs_to_jiffies(1000);
			while (1) {
				if (__raw_readl(priv->base + MMC_STATUS) &
				    STATUS_BUF_WRITE_RDY)
					break;

				if (time_is_before_jiffies(timeout_jiffies)) {
					dev_err(priv->host->parent,
						"wait write ready timeout\n");
					data->error = -ETIMEDOUT;
					break;
				}
			}

			__raw_writel(*buf++, priv->base + MMC_BUFFER_ACCESS);
		}

		if (!data->error) {
			mxcmci_interrupt_enable(priv, INT_CNTR_WRITE_OP_DONE);

			timeout = wait_for_completion_timeout(&priv->comp_write_op_done,
							      msecs_to_jiffies(1000));
			if (timeout == 0) {
				dev_err(priv->host->parent,
					"wait write_op_done timeout\n");
				data->error = -ETIMEDOUT;
			}
		}
	}

	mxcmci_data_done(priv);

	return 0;
}

static int mxcmci_dma_data_transfer(struct mxcmci_priv *priv)
{
	struct mmc_host *host = priv->host;
	struct mmc_data *data = priv->data;
	long timeout;

	struct _dma_param {
		char *label;
		unsigned int dir;
		mxc_dma_mode_t mode;
		u32 int_bit;
		struct completion *comp;
	} dma_param;

	if (host->ios.bus_width != priv->mode) {
		if (host->ios.bus_width == MMC_BUS_WIDTH_4)
			priv->dma = priv->dma_4bit;
		else
			priv->dma = priv->dma_1bit;
		priv->mode = host->ios.bus_width;
	}

	if (data->blksz & 0x3)
		dev_err(priv->host->parent, "blksz must be word aligned\n");

	if (data->flags & MMC_DATA_READ) {
		dma_param.label = "read";
		dma_param.dir = DMA_FROM_DEVICE;
		dma_param.mode = MXC_DMA_MODE_READ;
		dma_param.int_bit = INT_CNTR_READ_OP_DONE;
		dma_param.comp = &priv->comp_read_op_done;
	} else {
		dma_param.label = "write";
		dma_param.dir = DMA_TO_DEVICE;
		dma_param.mode = MXC_DMA_MODE_WRITE;
		dma_param.int_bit = INT_CNTR_WRITE_OP_DONE;
		dma_param.comp = &priv->comp_write_op_done;
	}

	priv->dma_dir = dma_param.dir;
	priv->dma_len = dma_map_sg(mmc_dev(priv->host), data->sg, data->sg_len,
				   priv->dma_dir);

	mxc_dma_sg_config(priv->dma, data->sg, data->sg_len,
			  priv->dma_size, dma_param.mode);

	mxc_dma_enable(priv->dma);

	timeout = wait_for_completion_timeout(&priv->comp_dma_done,
					      msecs_to_jiffies(1000));
	if (timeout == 0) {
		dev_err(priv->host->parent, "wait dma_done timeout\n");
		data->error = -ETIMEDOUT;
	}

	mxcmci_interrupt_enable(priv, dma_param.int_bit);
	timeout = wait_for_completion_timeout(dma_param.comp,
					      msecs_to_jiffies(1000));
	if (timeout == 0) {
		dev_err(priv->host->parent,
			"dma: wait %s_op_done timeout\n", dma_param.label);
		data->error = -ETIMEDOUT;
	}

	mxcmci_data_done(priv);

	return 0;
}

/*
 * This function is called when the requested command is completed.
 * This function reads the response from the card and data if the command is
 * for data transfer. This function checks for CRC error in response FIFO or
 * data FIFO.
 *
 * @param priv  Pointer to MMC/SD priv structure
 * @param stat  Content of SDHC Status Register
 *
 * @return This function returns 0 if there is no pending command, otherwise 1
 * always.
 */
static int mxcmci_cmd_done(struct mxcmci_priv *priv, unsigned int stat)
{
	struct mmc_command *cmd = priv->cmd;
	struct mmc_host *host = priv->host;
	struct mmc_card *card = host->card;
	int i;
	u32 res[3];

	if (!cmd)
		return 0;

	/* As this function finishes the command, initialize cmd to NULL */
	priv->cmd = NULL;

	if (!stat)
		stat = __raw_readl(priv->base + MMC_STATUS);

	/* check for time out errors */
	if (stat & STATUS_TIME_OUT_RESP) {
		__raw_writel(STATUS_TIME_OUT_RESP, priv->base + MMC_STATUS);
		dev_dbg(priv->host->parent, "cmd response timeout\n");
		cmd->error = -ETIMEDOUT;
	} else if (stat & STATUS_RESP_CRC_ERR && cmd->flags & MMC_RSP_CRC) {
		__raw_writel(STATUS_RESP_CRC_ERR, priv->base + MMC_STATUS);
		dev_err(priv->host->parent, "cmd crc error\n");
		cmd->error = -EILSEQ;
	}

	/* read response from the card */
	switch (RSP_TYPE(mmc_resp_type(cmd))) {
	case RSP_TYPE(MMC_RSP_R1): /* 48bit: r1, r1b, r5, r5b, r6, r7 */
	case RSP_TYPE(MMC_RSP_R3): /* 48bit: r3, r4 */
		res[0] = __raw_readl(priv->base + MMC_RES_FIFO) & 0xffff;
		res[1] = __raw_readl(priv->base + MMC_RES_FIFO) & 0xffff;
		res[2] = __raw_readl(priv->base + MMC_RES_FIFO) & 0xffff;
		cmd->resp[0] = (res[0] << 24) | (res[1] << 8) | (res[2] >> 8);
		break;
	case RSP_TYPE(MMC_RSP_R2): /* 136bit: r2 */
		for (i = 0; i < 4; i++) {
			res[0] = (__raw_readl(priv->base + MMC_RES_FIFO) &
				  0xffff);
			res[1] = (__raw_readl(priv->base + MMC_RES_FIFO) &
				  0xffff);
			cmd->resp[i] = (res[0] << 16) | res[1];
		}
		break;
	default:
		break;
	}

	dev_dbg(priv->host->parent, "CMD RES: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
		 cmd->resp[0], cmd->resp[1], cmd->resp[2], cmd->resp[3]);

	if (!priv->data || cmd->error) {
		mxcmci_finish_request(priv, priv->req);
		return 1;
	}

	/* currently using PIO for SDIO as it seems to provide better
	 * performance */
	if (card && card->type == MMC_TYPE_SDIO)
		mxcmci_pio_data_transfer(priv);
	else
		mxcmci_dma_data_transfer(priv);

	return 1;
}

/*
 * This function is called when the data transfer is completed either by DMA
 * or by PIO. This function is called to clean up the DMA buffer and to send
 * the STOP transmission command for commands that transfered data. This function
 * completes request issued by the MMC/SD core driver.
 *
 * @param priv   pointer to MMC/SD priv structure.
 * @param stat   content of SDHC Status Register
 *
 * @return This function returns 0 if no data transfer otherwise return 1
 * always.
 */
static int mxcmci_data_done(struct mxcmci_priv *priv)
{
	struct mmc_data *data = priv->data;
	struct mmc_host *host = priv->host;
	struct mmc_card *card = host->card;

	if (!data)
		return 0;

	/* DMA is only used for non-SDIO transfers */
	if (card && card->type != MMC_TYPE_SDIO)
		dma_unmap_sg(mmc_dev(priv->host), data->sg, priv->dma_len,
			     priv->dma_dir);

	if (__raw_readl(priv->base + MMC_STATUS) & STATUS_ERR_MASK)
		dev_dbg(priv->host->parent, "data_done: error status: 0x%08x\n",
			__raw_readl(priv->base + MMC_STATUS));

	priv->data = NULL;
	data->bytes_xfered = priv->dma_size;

	if (priv->req->stop && !data->error)
		mxcmci_start_cmd(priv, priv->req->stop, 0);
	else
		mxcmci_finish_request(priv, priv->req);

	return 1;
}

static int mxcmci_get_card_detect_status(struct mxcmci_priv *priv, int verbose)
{
	int status = priv->plat_data->status(priv->host->parent);
	int ret = 0;

	if (status == priv->plat_data->card_inserted_state)
		ret = 1;

	if (verbose)
		dev_dbg(priv->host->parent, "detect status: %d, [%s]\n",
			status, ret ? "INSERTED" : "REMOVED");

	return ret;
}

static void mxcmci_update_detect_irq_type(struct mxcmci_priv *priv)
{
	int status;

	do {
		status = priv->plat_data->status(priv->host->parent);
		if (status)
			set_irq_type(priv->detect_irq, IRQT_FALLING);
		else
			set_irq_type(priv->detect_irq, IRQT_RISING);
	} while (status != priv->plat_data->status(priv->host->parent));
}

/*
 * GPIO interrupt service routine registered to handle the SDHC interrupts.
 * This interrupt routine handles card insertion and card removal interrupts.
 *
 * @param   irq    the interrupt number
 * @param   devid  driver private data
 */
static irqreturn_t mxcmci_gpio_irq(int irq, void *devid)
{
	struct mxcmci_priv *priv = devid;

	if (mxcmci_get_card_detect_status(priv, 1)) {
		mmc_detect_change(priv->host, msecs_to_jiffies(100));
	} else {
		if (priv->cmd)
			mxcmci_cmd_done(priv, STATUS_TIME_OUT_RESP);
		mmc_detect_change(priv->host, msecs_to_jiffies(50));
	}

	mxcmci_update_detect_irq_type(priv);

	return IRQ_HANDLED;
}

/*
 * Interrupt service routine registered to handle the SDHC interrupts.
 * This interrupt routine handles end of command.
 *
 * @param   irq    the interrupt number
 * @param   devid  driver private data
 */
static irqreturn_t mxcmci_irq(int irq, void *devid)
{
	struct mxcmci_priv *priv = devid;
	struct mmc_data *data = priv->data;
	unsigned int status = 0;
	u32 intctrl;

	if (priv->mxc_mmc_suspend_flag == 1)
		clk_enable(priv->clk);

	status = __raw_readl(priv->base + MMC_STATUS);
	intctrl = __raw_readl(priv->base + MMC_INT_CNTR);

	dump_status(__FUNCTION__, status);

	if (status & STATUS_END_CMD_RESP) {
		__raw_writel(STATUS_END_CMD_RESP, priv->base + MMC_STATUS);
		complete(&priv->comp_cmd_done);
	}

	else if (status & STATUS_READ_OP_DONE) {
		/* check for time out and CRC errors */
		if (status & STATUS_TIME_OUT_READ) {
			dev_dbg(priv->host->parent, "read timeout\n");
			data->error = -ETIMEDOUT;
			__raw_writel(STATUS_TIME_OUT_READ,
				     priv->base + MMC_STATUS);
		} else if (status & STATUS_READ_CRC_ERR) {
			dev_dbg(priv->host->parent, "read CRC error\n");
			data->error = -EILSEQ;
			__raw_writel(STATUS_READ_CRC_ERR,
				     priv->base + MMC_STATUS);
		}

		mxcmci_interrupt_disable(priv, INT_CNTR_READ_OP_DONE);

		__raw_writel(STATUS_READ_OP_DONE, priv->base + MMC_STATUS);

		complete(&priv->comp_read_op_done);
	}

	else if (status & STATUS_WRITE_OP_DONE) {
		/* check for CRC errors */
		if (status & STATUS_WRITE_CRC_ERR) {
			dev_dbg(priv->host->parent, "write CRC error\n");
			data->error = -EILSEQ;
			__raw_writel(STATUS_WRITE_CRC_ERR,
				     priv->base + MMC_STATUS);
		}

		mxcmci_interrupt_disable(priv, INT_CNTR_WRITE_OP_DONE);

		__raw_writel(STATUS_WRITE_OP_DONE, priv->base + MMC_STATUS);

		complete(&priv->comp_write_op_done);
	}

	else if (status & STATUS_SDIO_INT_ACTIVE)
		mmc_signal_sdio_irq(priv->host);

	return IRQ_HANDLED;
}

/*
 * This function is called by the MMC/SD Bus Protocol driver to issue MMC and SD
 * commands to the SDHC.
 *
 * @param  host  Pointer to MMC/SD host structure
 * @param  req  Pointer to MMC/SD command request structure
 */
static void mxcmci_request(struct mmc_host *host, struct mmc_request *req)
{
	struct mxcmci_priv *priv = mmc_priv(host);
	unsigned long cmdat;

	WARN_ON(priv->req != NULL);

	priv->req = req;

	dump_cmd(req->cmd);
	dump_status(__FUNCTION__, __raw_readl(priv->base + MMC_STATUS));

	cmdat = 0;
	if (req->data) {
		mxcmci_setup_data(priv, req->data);

		cmdat |= CMD_DAT_CONT_DATA_ENABLE;

		if (req->data->flags & MMC_DATA_WRITE)
			cmdat |= CMD_DAT_CONT_WRITE;

		if (req->data->flags & MMC_DATA_STREAM)
			dev_err(priv->host->parent,
				"not support stream mode\n");
	}
	mxcmci_start_cmd(priv, req->cmd, cmdat);
}

/*
 * This function is called by the MMC/SD Bus Protocol driver to change the clock
 * speed.
 *
 * @param mmc Pointer to MMC/SD host structure
 * @param ios Pointer to MMC/SD I/O type structure
 */
static void mxcmci_set_ios(struct mmc_host *host, struct mmc_ios *ios)
{
	struct mxcmci_priv *priv = mmc_priv(host);
	struct mmc_card *card = host->card;
	int prescaler;
	int clk_rate = clk_get_rate(priv->clk);

	dev_dbg(priv->host->parent,
		"ios: clock %u, bus %lu, power %u, vdd %u\n",
		ios->clock, 1UL << ios->bus_width, ios->power_mode, ios->vdd);

	ext_power_ctrl(host, ios);

	/* Vary divider first, then prescaler. */
	if (ios->clock) {
		unsigned int clk_dev = 0;
		unsigned int read_to = SD_READ_TO_VALUE;
		int chip_rev_is_2_0 = (mxc_is_cpu(0x31)) ?
			cpu_is_mx31_rev(CHIP_REV_2_0) : cpu_is_mx32_rev(CHIP_REV_2_0);

		if (chip_rev_is_2_0 < 0)
			if (ios->clock > 15000000)
				ios->clock = 15000000;

		/* when prescaler = 16, CLK_20M = CLK_DIV / 2 */
		if (ios->clock == host->f_min)
			prescaler = 16;
		else
			prescaler = 0;

		while (prescaler <= 0x800) {
			for (clk_dev = 1; clk_dev <= 0xF; clk_dev++) {
				int x;
				if (prescaler != 0)
					x = (clk_rate / (clk_dev + 1)) /
						(prescaler * 2);
				else
					x = clk_rate / (clk_dev + 1);

				dev_dbg(priv->host->parent,
					"x=%d, clock=%d %d\n",
					x, ios->clock, clk_dev);
				if (x <= ios->clock)
					break;
			}
			if (clk_dev < 0x10)
				break;

			if (prescaler == 0)
				prescaler = 1;
			else
				prescaler <<= 1;
		}

		dev_dbg(priv->host->parent,
			"prescaler = 0x%x, divider = 0x%x\n",
			prescaler, clk_dev);

		__raw_writel((prescaler << 4) | clk_dev,
			     priv->base + MMC_CLK_RATE);

		/* for SDIO */
		host->max_blk_count = host->max_req_size / host->max_blk_size;
		if (card && card->state & MMC_STATE_PRESENT) {
			if (card->type == MMC_TYPE_SDIO) {
				read_to = SDIO_READ_TO_VALUE;
				/* SingleBlockTransfer */
				host->max_blk_count = 1;
			}
		}
		__raw_writel(read_to, priv->base + MMC_READ_TO);
	}
}

static int mxcmci_get_ro(struct mmc_host *host)
{
	struct mxcmci_priv *priv = mmc_priv(host);
	int ro = 0;

	if (priv->plat_data->get_ro) {
		ro = priv->plat_data->get_ro(priv->host->parent);
		return ro;
	}

	/* Board doesn't support read only detection; let the mmc core
	 * decide what to do. */
	return -ENOSYS;
}

static int mxcmci_get_cd(struct mmc_host *host)
{
	struct mxcmci_priv *priv = mmc_priv(host);
	return mxcmci_get_card_detect_status(priv, 0);
}

static void mxcmci_enable_sdio_irq(struct mmc_host *host, int enable)
{
	struct mxcmci_priv *priv = mmc_priv(host);
	unsigned long flags;

	spin_lock_irqsave(&priv->lock, flags);

	if (enable) {
		__raw_writel(STATUS_SDIO_INT_ACTIVE, priv->base + MMC_STATUS);
		mxcmci_interrupt_enable(priv, INT_CNTR_SDIO_IRQ_EN);
	} else {
		mxcmci_interrupt_disable(priv, INT_CNTR_SDIO_IRQ_EN);
	}

	spin_unlock_irqrestore(&priv->lock, flags);
}

static int mxcmci_clock_start(struct mmc_host *host)
{
	struct mxcmci_priv *priv = mmc_priv(host);

	mxcmci_start_clock(priv);
	return 0;
}

static int mxcmci_clock_stop(struct mmc_host *host)
{
	struct mxcmci_priv *priv = mmc_priv(host);

	mxcmci_stop_clock(priv);
	return 0;
}

/*
 * MMC/SD host operations structure.
 * These functions are registered with the MMC/SD Bus protocol driver.
 */
static struct mmc_host_ops mxcmci_ops = {
	.request = mxcmci_request,
	.set_ios = mxcmci_set_ios,
	.get_ro	 = mxcmci_get_ro,
	.get_cd  = mxcmci_get_cd,
	.enable_sdio_irq = mxcmci_enable_sdio_irq,

	.clk_start = mxcmci_clock_start,
	.clk_stop = mxcmci_clock_stop,
};

/*
 * This function is called by the DMA interrupt service routine to indicate
 * the requested DMA transfer is completed.
 *
 * @param   devid  pointer to device specific structure
 * @param   error any DMA error
 * @param   cnt   amount of data that was transferred
 */
static void mxcmci_dma_irq(void *devid, int error, unsigned int cnt)
{
	struct mxcmci_priv *priv = devid;

	mxc_dma_disable(priv->dma);

	complete(&priv->comp_dma_done);
}

/*
 * This function is called during the driver binding process. Based on the SDHC
 * module that is being probed this function adds the appropriate SDHC module
 * structure in the core driver.
 *
 * @param   pdev  The device structure used to store device specific
 *                information. An appropriate platform_data structure is
 *                required.
 *
 * @return  The function returns 0 on successful registration and initialization
 *          of the SDHC module. Otherwise it returns a specific error code.
 */
static int mxcmci_probe(struct platform_device *pdev)
{
	struct mxc_mmc_platform_data *mmc_plat = pdev->dev.platform_data;
	struct mmc_host *host;
	struct mxcmci_priv *priv = NULL;
	int ret = -ENODEV;

	if (!mmc_plat)
		return -ENOENT;

	host = mmc_alloc_host(sizeof(struct mxcmci_priv), &pdev->dev);
	if (!host)
		return -ENOMEM;
	platform_set_drvdata(pdev, host);

	host->ops = &mxcmci_ops;
	host->ocr_avail = mmc_plat->ocr_mask;
	host->ocr_avail |= (MMC_VDD_31_32 | MMC_VDD_32_33 | MMC_VDD_33_34);

	host->max_phys_segs = NR_SG;
	host->caps = (MMC_CAP_4_BIT_DATA | MMC_CAP_SDIO_IRQ);

	priv = mmc_priv(host);
	priv->host = host;
	priv->dma = -1;
	priv->dma_dir = DMA_NONE;
	priv->id = pdev->id;
	priv->mxc_mmc_suspend_flag = 0;
	priv->mode = -1;
	priv->plat_data = mmc_plat;

	mxcmci_setup_dma(priv);

	priv->clk = clk_get(&pdev->dev, "sdhc_clk");
	clk_enable(priv->clk);

	host->f_min = mmc_plat->min_clk;
	host->f_max = mmc_plat->max_clk;

	init_completion(&priv->comp_cmd_done);
	init_completion(&priv->comp_dma_done);
	init_completion(&priv->comp_read_op_done);
	init_completion(&priv->comp_write_op_done);

	spin_lock_init(&priv->lock);
	priv->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!priv->res) {
		ret = -ENOENT;
		goto err_out1;
	}

	if (!request_mem_region(priv->res->start,
				priv->res->end -
				priv->res->start + 1, pdev->name)) {
		pr_err("request_mem_region failed\n");
		ret = -ENOMEM;
		goto err_out1;
	}
	priv->base = (void *)IO_ADDRESS(priv->res->start);
	if (!priv->base) {
		ret = -EFAULT;
		goto err_out2;
	}

	priv->irq = platform_get_irq(pdev, 0);
	if (!priv->irq) {
		ret = -ENOENT;
		goto err_out2;
	}

	priv->detect_irq = platform_get_irq(pdev, 1);
	if (!priv->detect_irq) {
		ret = -ENOENT;
		goto err_out2;
	}

	mxcmci_update_detect_irq_type(priv);
	ret = request_irq(priv->detect_irq, mxcmci_gpio_irq, 0,
			  pdev->name, priv);
	if (ret)
		goto err_out2;

	mxcmci_softreset(priv);

	__raw_writel(SD_READ_TO_VALUE, priv->base + MMC_READ_TO);

	__raw_writel(INT_CNTR_END_CMD_RES, priv->base + MMC_INT_CNTR);

	ret = request_irq(priv->irq, mxcmci_irq, 0, pdev->name, priv);
	if (ret)
		goto err_out3;

	gpio_sdhc_active(pdev->id);

	ret = mmc_add_host(host);
	if (ret)
		goto err_out4;

	pr_info("%s-%d found\n", pdev->name, pdev->id);

	return 0;

err_out4:
	gpio_sdhc_inactive(pdev->id);
	free_irq(priv->irq, priv);
err_out3:
	free_irq(priv->detect_irq, priv);
err_out2:
	release_mem_region(priv->res->start,
			   priv->res->end - priv->res->start + 1);
err_out1:
	clk_disable(priv->clk);
	mmc_free_host(host);
	platform_set_drvdata(pdev, NULL);
	return ret;
}

/*
 * Dissociates the driver from the SDHC device. Removes the appropriate SDHC
 * module structure from the core driver.
 *
 * @param   pdev  the device structure used to give information on which SDHC
 *                to remove
 *
 * @return  The function always returns 0.
 */
static int mxcmci_remove(struct platform_device *pdev)
{
	struct mmc_host *host = platform_get_drvdata(pdev);
	platform_set_drvdata(pdev, NULL);

	if (host) {
		struct mxcmci_priv *priv = mmc_priv(host);

		mmc_remove_host(host);
		free_irq(priv->irq, priv);
		free_irq(priv->detect_irq, priv);
		mxc_dma_free(priv->dma_1bit);
		mxc_dma_free(priv->dma_4bit);
		release_mem_region(priv->res->start,
				   priv->res->end - priv->res->start + 1);
		mmc_free_host(host);
		gpio_sdhc_inactive(pdev->id);
	}
	return 0;
}

#ifdef CONFIG_PM

/*
 * This function is called to put the SDHC in a low power state. Refer to the
 * document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   pdev  the device structure used to give information on which SDHC
 *                to suspend
 * @param   state the power state the device is entering
 *
 * @return  The function always returns 0.
 */
static int mxcmci_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct mmc_host *host = platform_get_drvdata(pdev);
	struct mxcmci_priv *priv = mmc_priv(host);
	int ret = 0;

	if (host) {
		priv->mxc_mmc_suspend_flag = 1;
		ret = mmc_suspend_host(host, state);
	}
	clk_disable(priv->clk);

	return ret;
}

/*
 * This function is called to bring the SDHC back from a low power state. Refer
 * to the document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   pdev  the device structure used to give information on which SDHC
 *                to resume
 *
 * @return  The function always returns 0.
 */
static int mxcmci_resume(struct platform_device *pdev)
{
	struct mmc_host *host = platform_get_drvdata(pdev);
	struct mxcmci_priv *priv = mmc_priv(host);
	int ret = 0;

	/* Note that a card insertion interrupt will cause this
	 * driver to resume automatically.  In that case we won't
	 * actually have to do any work here.  Return success. */
	if (!priv->mxc_mmc_suspend_flag)
		return 0;

	clk_enable(priv->clk);

	if (host) {
		ret = mmc_resume_host(host);
		priv->mxc_mmc_suspend_flag = 0;
	}
	return ret;
}
#else
#define mxcmci_suspend  NULL
#define mxcmci_resume   NULL
#endif /* CONFIG_PM */

/*
 * This structure contains pointers to the power management callback functions.
 */
static struct platform_driver mxcmci_driver = {
	.driver = {
		   .name = "mxcmci",
		   },
	.probe = mxcmci_probe,
	.remove = mxcmci_remove,
	.suspend = mxcmci_suspend,
	.resume = mxcmci_resume,
};

/*
 * This function is used to initialize the MMC/SD driver module. The function
 * registers the power management callback functions with the kernel and also
 * registers the MMC/SD callback functions with the core MMC/SD driver.
 *
 * @return  The function returns 0 on success and a non-zero value on failure.
 */
static int __init mxcmci_init(void)
{
	pr_info("MXC MMC/SD driver\n");
	return platform_driver_register(&mxcmci_driver);
}

/*
 * This function is used to cleanup all resources before the driver exits.
 */
static void __exit mxcmci_exit(void)
{
	platform_driver_unregister(&mxcmci_driver);
}

module_init(mxcmci_init);
module_exit(mxcmci_exit);

MODULE_DESCRIPTION("MXC Multimedia Card Interface Driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
