/*
 * imx31-pcm.c -- ALSA SoC interface for the Freescale i.MX CPU's
 *
 * Copyright 2006 Wolfson Microelectronics PLC.
 * Copyright 2008 Atmark Techno, Inc.
 *
 * Based on pxa2xx-pcm.c by Nicolas Pitre, (C) 2004 MontaVista Software, Inc.
 * and on mxc-alsa-mc13783 (C) 2006 Freescale.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <asm/arch/dma.h>
#include <asm/arch/spba.h>
#include <asm/arch/clock.h>
#include <asm/mach-types.h>
#include <asm/hardware.h>

#include "imx31-pcm.h"
#include "imx-ssi.h"

/* debug */
#define IMX_PCM_DEBUG 0
#if IMX_PCM_DEBUG
#define dbg(format, arg...) printk(KERN_DEBUG format, ## arg)
#else
#define dbg(format, arg...)
#endif

/*
 * Coherent DMA memory is used by default, although Freescale have used
 * bounce buffers in all their drivers for i.MX31 to date. If you have any
 * issues, please select bounce buffers.
 */
#define IMX31_DMA_BOUNCE 0

static const struct snd_pcm_hardware imx31_pcm_hardware = {
	.info = (SNDRV_PCM_INFO_NONINTERLEAVED |
		 SNDRV_PCM_INFO_BLOCK_TRANSFER |
		 SNDRV_PCM_INFO_MMAP |
		 SNDRV_PCM_INFO_MMAP_VALID |
		 SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME),
	.formats = (SNDRV_PCM_FMTBIT_S16_LE |
		    SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE),
	.rates = (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 |
		  SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_48000 |
#if !defined(CONFIG_SND_SOC_ARMADILLO440_WM8978)
		  SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_22050 |
		  SNDRV_PCM_RATE_44100 |
#endif
		  SNDRV_PCM_RATE_CONTINUOUS),
	.rate_min = 8000,
	.rate_max = 48000,
	.channels_min = 1,
	.channels_max = 2,
	.buffer_bytes_max = 64 * 1024,
	.period_bytes_min = 64,
	.period_bytes_max = 8 * 1024,
	.periods_min = 2,
	.periods_max = 128,
	.fifo_size = 0,
};

struct mxc_runtime_data {
	int dma_ch;
	struct imx31_pcm_dma_param *dma_params;
	spinlock_t dma_lock; /* sdma lock */
	int active, period, periods;
	int dma_wchannel[2];
	int dma_active;
	int old_offset;
	int dma_alloc;
};

static void audio_stop_dma(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mxc_runtime_data *prtd = runtime->private_data;
	unsigned long flags;
	int i;
#if IMX31_DMA_BOUNCE
	unsigned int dma_size =
	  frames_to_bytes(runtime, runtime->period_size) / runtime->channels;
	unsigned int offset = dma_size * prtd->periods;
	int dir;
#endif

	/* stops the dma channel and clears the buffer ptrs */
	spin_lock_irqsave(&prtd->dma_lock, flags);
	prtd->active = 0;
	prtd->period = 0;
	prtd->periods = 0;
	for (i = 0; i < 2; i++)
		mxc_dma_disable(prtd->dma_wchannel[i]);

#if IMX31_DMA_BOUNCE
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dir = DMA_TO_DEVICE;
	else
		dir = DMA_FROM_DEVICE;
	dma_unmap_single(NULL, runtime->dma_addr + offset, dma_size,
			 dir);
	dma_unmap_single(NULL, runtime->dma_addr +
			 imx31_pcm_hardware.buffer_bytes_max + offset,
			 dma_size, dir);
#endif
	spin_unlock_irqrestore(&prtd->dma_lock, flags);
}

static int dma_new_period(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mxc_runtime_data *prtd = runtime->private_data;
	unsigned int dma_size =
	  frames_to_bytes(runtime, runtime->period_size) / runtime->channels;
	unsigned int offset = dma_size * prtd->period;
	int ret = 0, i;
	mxc_dma_requestbuf_t sdma_request;
	dma_addr_t* pdma_addr;
#if IMX31_DMA_BOUNCE
	int dir;
#endif
	mxc_dma_mode_t mode;

	if (!prtd->active)
		return 0;

	memset(&sdma_request, 0, sizeof(mxc_dma_requestbuf_t));

	dbg("period pos  ALSA %x DMA %x\n", runtime->periods, prtd->period);
	dbg("period size ALSA %x DMA %x Offset %x dmasize %x\n",
	    (unsigned int)runtime->period_size, runtime->dma_bytes,
	    offset, dma_size);
	dbg("DMA addr %x\n", runtime->dma_addr + offset);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		pdma_addr = &sdma_request.src_addr;
#if IMX31_DMA_BOUNCE
		dir = DMA_TO_DEVICE;
#endif
		mode = MXC_DMA_MODE_WRITE;
	}
	else {
		pdma_addr = &sdma_request.dst_addr;
#if IMX31_DMA_BOUNCE
		dir = DMA_FROM_DEVICE;
#endif
		mode = MXC_DMA_MODE_READ;
	}
	for (i = 0; i < 2; i++) {
#if IMX31_DMA_BOUNCE
		*pdma_addr =
		  dma_map_single(NULL, runtime->dma_area +
				 imx31_pcm_hardware.buffer_bytes_max * i +
				 offset, dma_size, dir);
#else
		*pdma_addr = runtime->dma_addr +
		  imx31_pcm_hardware.buffer_bytes_max * i + offset;
#endif
		sdma_request.num_of_bytes = dma_size;

		ret = mxc_dma_config(prtd->dma_wchannel[i], &sdma_request, 1,
				     mode);
		if (ret < 0) {
			printk(KERN_ERR "imx31-pcm: "
			       "cannot configure audio DMA channel\n");
			goto out;
		}
	}

	for (i = 0; i < 2; i++) {
		ret = mxc_dma_enable(prtd->dma_wchannel[i]);
		if (ret < 0) {
			printk(KERN_ERR
			       "imx31-pcm: cannot queue audio DMA buffer\n");
			goto out;
		}
	}
	prtd->dma_active = 1;
	prtd->period++;
	prtd->period %= runtime->periods;

 out:
	return ret;
}

static void audio_dma_irq(void *data, int error_status, unsigned int count)
{
	struct snd_pcm_substream *substream = (struct snd_pcm_substream *)data;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mxc_runtime_data *prtd = runtime->private_data;
#if IMX31_DMA_BOUNCE
	unsigned int dma_size =
	  frames_to_bytes(runtime, runtime->period_size) / runtime->channels;
	unsigned int offset = dma_size * prtd->periods;
	int dir;
#endif
	static int lock = 0;

	spin_lock(&prtd->dma_lock);
	if (lock == 0) {
		lock++;
		spin_unlock(&prtd->dma_lock);
		return;
	}
	else {
		lock = 0;
	}
	spin_unlock(&prtd->dma_lock);

	prtd->dma_active = 0;
	prtd->periods++;
	prtd->periods %= runtime->periods;

	dbg("irq per %d offset %x\n", prtd->periods,
	    frames_to_bytes(runtime, runtime->period_size) * prtd->periods);
#if IMX31_DMA_BOUNCE
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dir = DMA_TO_DEVICE;
	else
		dir = DMA_FROM_DEVICE;
	dma_unmap_single(NULL, runtime->dma_addr + offset, dma_size,
			 dir);
	dma_unmap_single(NULL, runtime->dma_addr +
			 imx31_pcm_hardware.buffer_bytes_max + offset,
			 dma_size, dir);
#endif

	if (prtd->active)
		snd_pcm_period_elapsed(substream);
	spin_lock(&prtd->dma_lock);
	dma_new_period(substream);
	spin_unlock(&prtd->dma_lock);
}

static int imx31_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mxc_runtime_data *prtd = runtime->private_data;

	prtd->period = 0;
	prtd->periods = 0;
	return 0;
}

static int imx31_pcm_hw_params(struct snd_pcm_substream *substream,
			       struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mxc_runtime_data *prtd = runtime->private_data;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_cpu_dai *cpu_dai = rtd->dai->cpu_dai;
	int ret = 0, i;

	/* only allocate the DMA chn once */
	if (!prtd->dma_alloc) {
		mxc_dma_device_t id[2];
		char *dev_name[2], *dir_name;

		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (params_format(params) == SNDRV_PCM_FORMAT_S16_LE) {
				if (cpu_dai->id == IMX_DAI_SSI0) {
					id[0] = MXC_DMA_SSI1_16BIT_TX0;
					id[1] = MXC_DMA_SSI1_16BIT_TX1;
				}
				else {
					id[0] = MXC_DMA_SSI2_16BIT_TX0;
					id[1] = MXC_DMA_SSI2_16BIT_TX1;
				}
			}
			else {
				if (cpu_dai->id == IMX_DAI_SSI0) {
					id[0] = MXC_DMA_SSI1_24BIT_TX0;
					id[1] = MXC_DMA_SSI1_24BIT_TX1;
				}
				else {
					id[0] = MXC_DMA_SSI2_24BIT_TX0;
					id[1] = MXC_DMA_SSI2_24BIT_TX1;
				}
			}
			dev_name[0] = "ALSA TX0 SDMA";
			dev_name[1] = "ALSA TX1 SDMA";
			dir_name = "write";
		}
		else {
			if (params_format(params) == SNDRV_PCM_FORMAT_S16_LE) {
				if (cpu_dai->id == IMX_DAI_SSI0) {
					id[0] = MXC_DMA_SSI1_16BIT_RX0;
					id[1] = MXC_DMA_SSI1_16BIT_RX1;
				}
				else {
					id[0] = MXC_DMA_SSI2_16BIT_RX0;
					id[1] = MXC_DMA_SSI2_16BIT_RX1;
				}
			}
			else {
				if (cpu_dai->id == IMX_DAI_SSI0) {
					id[0] = MXC_DMA_SSI1_24BIT_RX0;
					id[1] = MXC_DMA_SSI1_24BIT_RX1;
				}
				else {
					id[0] = MXC_DMA_SSI2_24BIT_RX0;
					id[1] = MXC_DMA_SSI2_24BIT_RX1;
				}
			}
			dev_name[0] = "ALSA RX0 SDMA";
			dev_name[1] = "ALSA RX1 SDMA";
			dir_name = "read";
		}
		for (i = 0; i < 2; i++) {
			prtd->dma_wchannel[i] =
			  mxc_dma_request(id[i], dev_name[i]);
			if (prtd->dma_wchannel[i] < 0) {
				printk(KERN_ERR
				       "imx31-pcm: error requesting a %s"
				       " dma channel\n", dir_name);
				ret = prtd->dma_wchannel[i];
				goto out;
			}
		}
		prtd->dma_alloc = 1;

		for (i = 0; i < 2; i++) {
			ret = mxc_dma_callback_set(prtd->dma_wchannel[i],
						   audio_dma_irq, substream);
			if (ret < 0) {
				printk(KERN_ERR "imx31-pcm: failed to setup "
				       "audio DMA chn %d(%d)\n",
				       prtd->dma_wchannel[i], ret);
				goto out;
			}
		}
	}
#if IMX31_DMA_BOUNCE
	ret =
	  snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params) * 2);
	if (ret < 0) {
		printk(KERN_ERR "imx31-pcm: failed to malloc pcm pages\n");
		goto out;
	}
	runtime->dma_addr = virt_to_phys(runtime->dma_area);

	memset(runtime->dma_area, 0, params_buffer_bytes(params) * 2);
#else
	memset(substream->dma_buffer.area, 0, params_buffer_bytes(params) * 2);
	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
#endif
	return 0;

 out:
	for (i = 0; i < 2; i++) {
		if (prtd->dma_wchannel[i] > 0)
			mxc_dma_free(prtd->dma_wchannel[i]);
		prtd->dma_wchannel[i] = 0;
	}
	return ret;
}

static int imx31_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mxc_runtime_data *prtd = runtime->private_data;
	int i;

	for (i = 0; i < 2; i++)
		if (prtd->dma_wchannel[i]) {
			mxc_dma_free(prtd->dma_wchannel[i]);
			prtd->dma_wchannel[i] = 0;
		}
	prtd->dma_alloc = 0;
#if IMX31_DMA_BOUNCE
	snd_pcm_lib_free_pages(substream);
#endif
	return 0;
}

static int imx31_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct mxc_runtime_data *prtd = substream->runtime->private_data;
	int ret = 0;

	spin_lock(&prtd->dma_lock);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		prtd->dma_active = 0;
		/* requested stream startup */
		prtd->active = 1;
		ret = dma_new_period(substream);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		/* requested stream shutdown */
		audio_stop_dma(substream);
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
		prtd->active = 0;
		prtd->periods = 0;
		break;
	case SNDRV_PCM_TRIGGER_RESUME:
		prtd->active = 1;
		prtd->dma_active = 0;
		ret = dma_new_period(substream);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		prtd->active = 0;
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		prtd->active = 1;
		if (prtd->old_offset) {
			prtd->dma_active = 0;
			ret = dma_new_period(substream);
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}
	spin_unlock(&prtd->dma_lock);

	return ret;
}

static snd_pcm_uframes_t imx31_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mxc_runtime_data *prtd = runtime->private_data;
	unsigned int offset = 0;

	offset = (runtime->period_size * (prtd->periods));
	if (offset >= runtime->buffer_size)
		offset = 0;
	dbg("pointer offset %x\n", offset);

	return offset;
}

static int imx31_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mxc_runtime_data *prtd;
	int ret;

	snd_soc_set_runtime_hwparams(substream, &imx31_pcm_hardware);

	ret = snd_pcm_hw_constraint_integer(runtime,
					    SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0)
		return ret;

	prtd = kzalloc(sizeof(struct mxc_runtime_data), GFP_KERNEL);
	if (prtd == NULL)
		return -ENOMEM;

	spin_lock_init(&prtd->dma_lock);

	runtime->private_data = prtd;
	return 0;
}

static int imx31_pcm_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mxc_runtime_data *prtd = runtime->private_data;

	kfree(prtd);
	return 0;
}

static int
imx31_pcm_mmap(struct snd_pcm_substream *substream, struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	return dma_mmap_writecombine(substream->pcm->card->dev, vma,
				     runtime->dma_area,
				     runtime->dma_addr, runtime->dma_bytes);
}

struct snd_pcm_ops imx31_pcm_ops = {
	.open = imx31_pcm_open,
	.close = imx31_pcm_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = imx31_pcm_hw_params,
	.hw_free = imx31_pcm_hw_free,
	.prepare = imx31_pcm_prepare,
	.trigger = imx31_pcm_trigger,
	.pointer = imx31_pcm_pointer,
	.mmap = imx31_pcm_mmap,
};

#if !IMX31_DMA_BOUNCE

static int imx31_pcm_preallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size = imx31_pcm_hardware.buffer_bytes_max * 2;
	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;
	buf->area = dma_alloc_writecombine(pcm->card->dev, size,
					   &buf->addr, GFP_KERNEL);
	if (!buf->area)
		return -ENOMEM;

	buf->bytes = size;
	return 0;
}

static void imx31_pcm_free_dma_buffers(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;

	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;

		dma_free_writecombine(pcm->card->dev, buf->bytes,
				      buf->area, buf->addr);
		buf->area = NULL;
	}
}

#endif

static u64 imx31_pcm_dmamask = 0xffffffff;

static int imx31_pcm_new(struct snd_card *card, struct snd_soc_codec_dai *dai,
			 struct snd_pcm *pcm)
{
	int ret = 0;

	if (!card->dev->dma_mask)
		card->dev->dma_mask = &imx31_pcm_dmamask;

	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = 0xffffffff;

#if IMX31_DMA_BOUNCE
	ret = snd_pcm_lib_preallocate_pages_for_all(pcm,
		SNDRV_DMA_TYPE_CONTINUOUS,
		snd_dma_continuous_data(GFP_KERNEL),
		imx31_pcm_hardware.buffer_bytes_max * 2,
		imx31_pcm_hardware.buffer_bytes_max * 2);
	if (ret < 0) {
		printk(KERN_ERR "imx31-pcm: failed to preallocate pages\n");
		goto out;
	}
#else
	if (dai->playback.channels_min) {
		ret =
		  imx31_pcm_preallocate_dma_buffer(pcm,
						   SNDRV_PCM_STREAM_PLAYBACK);
		if (ret)
			goto out;
	}
	if (dai->capture.channels_min) {
		ret =
		  imx31_pcm_preallocate_dma_buffer(pcm,
						   SNDRV_PCM_STREAM_CAPTURE);
		if (ret)
			goto out;
	}
#endif

 out:
	return ret;
}

struct snd_soc_platform imx3x_platform = {
	.name		= "imx31-pcm",
	.pcm_ops	= &imx31_pcm_ops,
	.pcm_new	= imx31_pcm_new,
#if IMX31_DMA_BOUNCE
	.pcm_free	= NULL,
#else
	.pcm_free	= imx31_pcm_free_dma_buffers,
#endif
};
EXPORT_SYMBOL_GPL(imx3x_platform);
struct snd_soc_platform imx_platform = {
	.name		= "imx-pcm",
	.pcm_ops	= &imx31_pcm_ops,
	.pcm_new	= imx31_pcm_new,
#if IMX31_DMA_BOUNCE
	.pcm_free	= NULL,
#else
	.pcm_free	= imx31_pcm_free_dma_buffers,
#endif
};
EXPORT_SYMBOL_GPL(imx_platform);

MODULE_AUTHOR("Atmark Techno, Inc.");
MODULE_DESCRIPTION("Freescale i.MX31 PCM DMA module");
MODULE_LICENSE("GPL");
