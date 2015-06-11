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

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/clk.h>

#include <linux/tlv320aic.h>

#include <asm/arch/dma.h>
#include <asm/arch/clock.h>
#include <asm/arch/gpio.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/control.h>

#include <ssi/ssi.h>
#include <ssi/registers.h>
#include <dam/dam.h>

#define DRIVER_NAME "mxc_alsa_i2s"
#define REVISION    "Rev.2"
#define AUTHOR      "Atmark Techno, Inc."
#define DESCRIPTION "MXC ALSA iis audio driver"

#if defined(DEBUG)
#define DEBUG_FUNC()         pr_info(DRIVER_NAME ": %s()\n", __FUNCTION__)
#define DEBUG_INFO(args...)  pr_info(DRIVER_NAME ": " args)
#define DEBUG_DUMP(args...)  pr_info(args)
#else
#define DEBUG_FUNC()
#define DEBUG_INFO(args...)
#define DEBUG_DUMP(args...)
#endif

#define DMA_BUFFER_SIZE			(8*1024)
#define MAX_BUFFER_SIZE			(32*1024)
#define MIN_PERIOD_SIZE			(8*8)
#define MIN_PERIOD			(2)
#define MAX_PERIOD			(255)

#define DEFAULT_SAMPLING_RATE		(48000)
#define DEFAULT_LINE_INPUT_VOLUME	(0x0a0a)
#define DEFAULT_HEADPHONE_VOLUME	(0x1e1e)

#define TX_WATERMARK			0x4
#define RX_WATERMARK			0x6

extern void gpio_audio_active(int select);
extern void gpio_audio_inactive(int select);

struct audio_stream {
	int ssi;
	int dma_channel_0;
	int dma_channel_1;
	int current_period;
	int complete_period;
	spinlock_t dma_lock;
};

struct audio_private {
	struct audio_stream *playback_stream;
	struct audio_stream *capture_stream;
	struct clk *clk;
};

enum volume_type {
	MIXERID_LINE_INPUT_VOL,
	MIXERID_HEADPHONE_VOL,
};

/* macro */
#define INCREMENT_CURRENT_PERIOD()                  \
({                                                  \
	stream->current_period += 1;                \
	stream->current_period %= runtime->periods; \
})
#define INCREMENT_COMPLETE_PERIOD()                  \
({                                                   \
	stream->complete_period += 1;                \
	stream->complete_period %= runtime->periods; \
})

static void
codec_dump_register(void)
{
	u16 livol, rivol, lhvol, rhvol;
	u16 apath, dpath, pwr, diform, srate, dact;

	livol = tlv320aic_getreg(TLV_LIVOL);
	rivol = tlv320aic_getreg(TLV_RIVOL);
	lhvol = tlv320aic_getreg(TLV_LHVOL);
	rhvol = tlv320aic_getreg(TLV_RHVOL);
	apath = tlv320aic_getreg(TLV_APATH);
	dpath = tlv320aic_getreg(TLV_DPATH);
	pwr = tlv320aic_getreg(TLV_PWR);
	diform = tlv320aic_getreg(TLV_DIFORM);
	srate = tlv320aic_getreg(TLV_SRATE);
	dact = tlv320aic_getreg(TLV_DACT);

	DEBUG_DUMP("LIVOL : 0x%04x,  RIVOL  : 0x%04x\n", livol, rivol);
	DEBUG_DUMP("LHVOL : 0x%04x,  RHVOL  : 0x%04x\n", lhvol, rhvol);
	DEBUG_DUMP("APATH : 0x%04x,  DPATH  : 0x%04x\n", apath, dpath);
	DEBUG_DUMP("PWR   : 0x%04x,  DIFORM : 0x%04x\n", pwr, diform);
	DEBUG_DUMP("SRATE : 0x%04x,  DACT   : 0x%04x\n", srate, dact);
}

static void
i2s_dump_register(int ssi)
{
	u32 base = (ssi == SSI2) ? SSI2_BASE_ADDR : SSI1_BASE_ADDR;
	u32 scr, sisr, sier, stcr, stccr, sfcsr, srcr, srccr;

	DEBUG_FUNC();

	scr   = readl(IO_ADDRESS(base) + MXC_SSISCR);
	sisr  = readl(IO_ADDRESS(base) + MXC_SSISISR);
	sier  = readl(IO_ADDRESS(base) + MXC_SSISIER);
	stcr  = readl(IO_ADDRESS(base) + MXC_SSISTCR);
	srcr  = readl(IO_ADDRESS(base) + MXC_SSISRCR);
	stccr = readl(IO_ADDRESS(base) + MXC_SSISTCCR);
	srccr = readl(IO_ADDRESS(base) + MXC_SSISRCCR);
	sfcsr = readl(IO_ADDRESS(base) + MXC_SSISFCSR);

	DEBUG_DUMP("SCR  : 0x%08x,  SFCSR: 0x%08x\n", scr, sfcsr);
	DEBUG_DUMP("SISR : 0x%08x,  SIER : 0x%08x\n", sisr, sier);
	DEBUG_DUMP("STCR : 0x%08x,  SRCR : 0x%08x\n", stcr, srcr);
	DEBUG_DUMP("STCCR: 0x%08x,  SRCCR: 0x%08x\n", stccr, srccr);
}

static void
mxc_alsa_i2s_unregister_private(struct snd_card *card)
{
	struct audio_private *priv;

	DEBUG_FUNC();

	priv = card->private_data;
	if (priv) {
		kfree(priv->capture_stream);
		kfree(priv->playback_stream);
		kfree(priv);
		card->private_data = 0;
		card->private_free = NULL;
	}
}

static int
mxc_alsa_i2s_register_private(struct snd_card *card, struct clk *clk)
{
	struct audio_private *priv;

	DEBUG_FUNC();

	priv = kcalloc(1, sizeof(struct audio_private), GFP_KERNEL);
	if (!priv)
		goto error_out;

	priv->playback_stream = kcalloc(1, sizeof(struct audio_stream), GFP_KERNEL);
	if (!priv->playback_stream)
		goto free_priv;

	priv->capture_stream = kcalloc(1, sizeof(struct audio_stream), GFP_KERNEL);
	if (!priv->capture_stream)
		goto free_playback;

	priv->clk = clk;

	card->private_data = priv;
	card->private_free = mxc_alsa_i2s_unregister_private;

	spin_lock_init(&(priv->playback_stream->dma_lock));
	spin_lock_init(&(priv->capture_stream->dma_lock));

	return 0;

 free_playback:
	kfree(priv->playback_stream);
 free_priv:
	kfree(priv);
 error_out:
	return -ENOMEM;
}

static int
mxc_alsa_i2s_dma_playback(struct snd_pcm_substream *substream)
{
	struct audio_private *priv;
	struct audio_stream *stream;
	struct snd_pcm_runtime *runtime;
	mxc_dma_requestbuf_t dma_request;
	u32 dma_size;
	u32 offset;
	int ret;
	int i, j;

	DEBUG_FUNC();

	memset(&dma_request, 0, sizeof(mxc_dma_requestbuf_t));

	priv = snd_pcm_substream_chip(substream);
	stream = priv->playback_stream;
	runtime = substream->runtime;

	if (ssi_get_status(SSI1) & ssi_transmitter_underrun_0)
		ssi_transmit_enable(SSI1, false);

	dma_size =
	  frames_to_bytes(runtime, runtime->period_size) / runtime->channels;

	for (j = 0; j < 2; j++) {
		for (i = 0; i < 2; i++) {
			int dma_channel = ((i == 0) ? 
					   stream->dma_channel_0 :
					   stream->dma_channel_1);
			u32 register_offset = ((i == 0) ?
					       MXC_SSI1STX0 :
					       MXC_SSI1STX1);

			offset = dma_size * stream->current_period;

			dma_request.src_addr =
			  dma_map_single(NULL,
					 runtime->dma_area +
					 (runtime->dma_bytes /
					  runtime->channels) * i +
					 offset,
					 dma_size,
					 DMA_TO_DEVICE);
			dma_request.dst_addr =
			  SSI1_BASE_ADDR + register_offset;
			dma_request.num_of_bytes = dma_size;

			ret = mxc_dma_config(dma_channel, &dma_request, 1,
					     MXC_DMA_MODE_WRITE);
			if (ret)
				pr_info("DMA Config failed\n");
		}

		ssi_interrupt_enable(SSI1, ssi_tx_dma_interrupt_enable);
		ret = mxc_dma_enable(stream->dma_channel_0);
		ret = mxc_dma_enable(stream->dma_channel_1);
		ssi_transmit_enable(SSI1, true);

		INCREMENT_CURRENT_PERIOD();

		/*
		 * Set up the 2nd dma buffer check
		 */
		if ((stream->current_period > stream->complete_period) && 
		    ((stream->current_period - stream->complete_period) > 1)) {
			pr_debug("audio playback chain dma: "
				 "already double buffered\n");
			break;
		}
    
		if ((stream->current_period < stream->complete_period)
		    && ((stream->current_period + runtime->periods -
			 stream->complete_period) > 1)) {
			pr_debug("audio playback chain dma: "
				 "already double buffered\n");
			break;
		}

		if (stream->current_period == stream->complete_period) {
			pr_debug("audio playback chain dma: "
				 "stream->current_period == "
				 "stream->complete_period\n");
			break;
		}

		if (snd_pcm_playback_hw_avail(runtime) <
		    2 * runtime->period_size) {
			pr_debug("audio playback chain dma: "
				 "available data is not enough\n");
			break;
		}
	}
	return 0;
}

static int
mxc_alsa_i2s_dma_playback_stop(struct snd_pcm_substream *substream)
{
	struct audio_private *priv;
	struct audio_stream *stream;
	struct snd_pcm_runtime *runtime;
	u32 dma_size;
	u32 offset;
	u32 flag;

	DEBUG_FUNC();

	priv = snd_pcm_substream_chip(substream);
	stream = priv->playback_stream;
	runtime = substream->runtime;
	dma_size =
	  frames_to_bytes(runtime, runtime->period_size) / runtime->channels;
	offset = dma_size * stream->complete_period;

	spin_lock_irqsave(&stream->dma_lock, flag);

	stream->current_period = 0;
	stream->complete_period = 0;

	mxc_dma_disable(stream->dma_channel_0);
	mxc_dma_disable(stream->dma_channel_1);
	dma_unmap_single(NULL,
			 runtime->dma_addr + offset,
			 dma_size,
			 DMA_TO_DEVICE);
	dma_unmap_single(NULL,
			 runtime->dma_addr +
			 runtime->dma_bytes / runtime->channels + offset,
			 dma_size,
			 DMA_TO_DEVICE);

	spin_unlock_irqrestore(&stream->dma_lock, flag);

	return 0;
}

static void
mxc_alsa_i2s_dma_playback_callback(void *arg, int error_status,
				   unsigned int count)
{
	struct snd_pcm_substream *substream = arg;
	struct audio_private *priv;
	struct audio_stream *stream;
	struct snd_pcm_runtime *runtime;
	static int lock = 0;
	u32 dma_size;
	u32 offset;

	DEBUG_FUNC();

	priv = snd_pcm_substream_chip(substream);
	stream = priv->playback_stream;

	spin_lock(&stream->dma_lock);
	if (lock == 0) {
		lock++;
		spin_unlock(&stream->dma_lock);
		return;
	} else {
		lock=0;
	}
	spin_unlock(&stream->dma_lock);
    
	priv = snd_pcm_substream_chip(substream);
	stream = priv->playback_stream;
	runtime = substream->runtime;
	dma_size =
	  frames_to_bytes(runtime, runtime->period_size) / runtime->channels;
	offset = dma_size * stream->complete_period;

	INCREMENT_COMPLETE_PERIOD();

	dma_unmap_single(NULL,
			 runtime->dma_addr + offset,
			 dma_size,
			 DMA_TO_DEVICE);
	dma_unmap_single(NULL,
			 runtime->dma_addr +
			 runtime->dma_bytes / runtime->channels + offset,
			 dma_size,
			 DMA_TO_DEVICE);

	snd_pcm_period_elapsed(substream);

	spin_lock(&stream->dma_lock);
	mxc_alsa_i2s_dma_playback(substream);
	spin_unlock(&stream->dma_lock);
}

static int
mxc_alsa_i2s_dma_capture(struct snd_pcm_substream *substream)
{
	struct audio_private *priv;
	struct audio_stream *stream;
	struct snd_pcm_runtime *runtime;
	mxc_dma_requestbuf_t dma_request;
	u32 dma_size;
	u32 offset;
	int ret;
	int i, j;

	DEBUG_FUNC();

	memset(&dma_request, 0, sizeof(mxc_dma_requestbuf_t));

	priv = snd_pcm_substream_chip(substream);
	stream = priv->capture_stream;
	runtime = substream->runtime;

	dma_size =
	  frames_to_bytes(runtime, runtime->period_size) / runtime->channels;

	for (j = 0; j < 2; j++) {
		for (i = 0; i < 2; i++) {
			int dma_channel = ((i == 0) ?
					   stream->dma_channel_0 :
					   stream->dma_channel_1);
			u32 register_offset = ((i == 0) ?
					       MXC_SSI1SRX0 :
					       MXC_SSI1SRX1);

			offset = dma_size * stream->current_period;

			dma_request.dst_addr =
			  dma_map_single(NULL,
					 runtime->dma_area +
					 (runtime->dma_bytes /
					  runtime->channels) * i +
					 offset,
					 dma_size,
					 DMA_FROM_DEVICE);
			dma_request.src_addr =
			  SSI1_BASE_ADDR + register_offset;
			dma_request.num_of_bytes = dma_size;

			ret = mxc_dma_config(dma_channel, &dma_request, 1,
					     MXC_DMA_MODE_READ);
			if (ret)
				printk("DMA Config failed\n");
		}

		ssi_interrupt_enable(SSI1, ssi_rx_dma_interrupt_enable);
		ret = mxc_dma_enable(stream->dma_channel_0);
		ret = mxc_dma_enable(stream->dma_channel_1);
		ssi_receive_enable(SSI1, true);

		INCREMENT_CURRENT_PERIOD();

		/*
		 * Set up the 2nd dma buffer check
		 */
		if ((stream->current_period > stream->complete_period) && 
		    ((stream->current_period - stream->complete_period) > 1)) {
			pr_debug("audio playback chain dma: "
				 "already double buffered\n");
			break;
		}
    
		if ((stream->current_period < stream->complete_period)
		    && ((stream->current_period + runtime->periods -
			 stream->complete_period) > 1)) {
			pr_debug("audio playback chain dma: "
				 "already double buffered\n");
			break;
		}
    
		if (stream->current_period == stream->complete_period) {
			pr_debug("audio playback chain dma: "
				 "stream->current_period == "
				 "stream->complete_period\n");
			break;
		}
    
		if (snd_pcm_playback_hw_avail(runtime) <
		    2 * runtime->period_size) {
			pr_debug("audio playback chain dma: "
				 "available data is not enough\n");
			break;
		}
	}
	return 0;
}

static int
mxc_alsa_i2s_dma_capture_stop(struct snd_pcm_substream *substream)
{
	struct audio_private *priv;
	struct audio_stream *stream;
	struct snd_pcm_runtime *runtime;
	u32 dma_size;
	u32 offset;
	u32 flag;

	DEBUG_FUNC();

	priv = snd_pcm_substream_chip(substream);
	stream = priv->capture_stream;
	runtime = substream->runtime;

	dma_size =
	  frames_to_bytes(runtime, runtime->period_size) / runtime->channels;
	offset = dma_size * stream->complete_period;

	spin_lock_irqsave(&stream->dma_lock, flag);

	stream->current_period = 0;
	stream->complete_period = 0;

	mxc_dma_disable(stream->dma_channel_0);
	mxc_dma_disable(stream->dma_channel_1);
	dma_unmap_single(NULL,
			 runtime->dma_addr + offset,
			 dma_size,
			 DMA_FROM_DEVICE);
	dma_unmap_single(NULL,
			 runtime->dma_addr +
			 runtime->dma_bytes / runtime->channels + offset,
			 dma_size,
			 DMA_FROM_DEVICE);

	spin_unlock_irqrestore(&stream->dma_lock, flag);

	return 0;
}

static void
mxc_alsa_i2s_dma_capture_callback(void *arg, int error_status,
				  unsigned int count)
{
	struct snd_pcm_substream *substream = arg;
	struct audio_private *priv;
	struct audio_stream *stream;
	struct snd_pcm_runtime *runtime;
	static int lock = 0;
	u32 dma_size;
	u32 offset;

	DEBUG_FUNC();

	priv = snd_pcm_substream_chip(substream);
	stream = priv->capture_stream;

	spin_lock(&stream->dma_lock);
	if (lock == 0) {
		lock++;
		spin_unlock(&stream->dma_lock);
		return;
	} else {
		lock=0;
	}
	spin_unlock(&stream->dma_lock);

	priv = snd_pcm_substream_chip(substream);
	runtime = substream->runtime;
	dma_size =
	  frames_to_bytes(runtime, runtime->period_size) / runtime->channels;
	offset = dma_size * stream->complete_period;

	INCREMENT_COMPLETE_PERIOD();

	dma_unmap_single(NULL,
			 runtime->dma_addr + offset,
			 dma_size,
			 DMA_FROM_DEVICE);
	dma_unmap_single(NULL,
			 runtime->dma_addr +
			 runtime->dma_bytes / runtime->channels + offset,
			 dma_size,
			 DMA_FROM_DEVICE);

	snd_pcm_period_elapsed(substream);

	spin_lock(&stream->dma_lock);
	mxc_alsa_i2s_dma_capture(substream);
	spin_unlock(&stream->dma_lock);
}

static int
mxc_alsa_i2s_dma_request(mxc_dma_device_t id, 
			 mxc_dma_callback_t callback,
			 void *arg)
{
	struct snd_pcm_substream *substream = arg;
	struct audio_private *priv;
	int channel;
	int ret;

	DEBUG_FUNC();

	priv = snd_pcm_substream_chip(substream);

	switch (id) {
	case MXC_DMA_SSI1_16BIT_TX0:
		channel = mxc_dma_request(id, "ALSA TX0 DMA");
		priv->playback_stream->dma_channel_0 = channel;
		break;
	case MXC_DMA_SSI1_16BIT_TX1:
		channel = mxc_dma_request(id, "ALSA TX1 DMA");
		priv->playback_stream->dma_channel_1 = channel;
		break;
	case MXC_DMA_SSI1_16BIT_RX0:
		channel = mxc_dma_request(id, "ALSA RX0 DMA");
		priv->capture_stream->dma_channel_0 = channel;
		break;
	case MXC_DMA_SSI1_16BIT_RX1:
		channel = mxc_dma_request(id, "ALSA RX1 DMA");
		priv->capture_stream->dma_channel_1 = channel;
		break;
	default:
		DEBUG_INFO("%s(): DMA request failed(%08x)\n",
			   __FUNCTION__, id);
		return -ENODEV;
	}

	if (callback) {
		ret = mxc_dma_callback_set(channel, callback, substream);
		if (ret < 0) {
			mxc_dma_free(channel);
			return -EBUSY;
		}
	}

	return 0;
}

static int
mxc_alsa_i2s_dma_free(mxc_dma_device_t id, void *arg)
{
	struct audio_private *priv = arg;

	DEBUG_FUNC();

	switch (id) {
	case MXC_DMA_SSI1_16BIT_TX0:
		mxc_dma_free(priv->playback_stream->dma_channel_0);
		break;
	case MXC_DMA_SSI1_16BIT_TX1:
		mxc_dma_free(priv->playback_stream->dma_channel_1);
		break;
	case MXC_DMA_SSI1_16BIT_RX0:
		mxc_dma_free(priv->capture_stream->dma_channel_0);
		break;
	case MXC_DMA_SSI1_16BIT_RX1:
		mxc_dma_free(priv->capture_stream->dma_channel_1);
		break;
	default:
		break;
	}
	return 0;
}

static int
mxc_alsa_i2s_audio_get_volume_info(int type, struct snd_ctl_elem_info *uinfo)
{
	switch (type) {
	case MIXERID_LINE_INPUT_VOL:
		uinfo->value.integer.min = 0;
		uinfo->value.integer.max = 0x1f;
		uinfo->value.integer.step = 1;
		break;
	case MIXERID_HEADPHONE_VOL:
		uinfo->value.integer.min = 0;
		uinfo->value.integer.max = 0x4f;
		uinfo->value.integer.step = 1;
		break;
	default:
		return -EINVAL;
	}

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;

	return 0;
}

static int
mxc_alsa_i2s_audio_get_volume(int type)
{
	u8 left, right;
	u8 lval, rval;

	DEBUG_FUNC();
	switch (type) {
	case MIXERID_LINE_INPUT_VOL:
		{
		  u16 livol, rivol;
		  livol = tlv320aic_getreg(TLV_LIVOL);
		  left = (livol & TLV_LINE_INPUT_VOL_MASK);

		  rivol = tlv320aic_getreg(TLV_RIVOL);
		  right = (livol & TLV_LINE_INPUT_VOL_MASK);
		  break;
		}
	case MIXERID_HEADPHONE_VOL:
		{
		  u16 lhvol, rhvol;

		  lhvol = tlv320aic_getreg(TLV_LHVOL);
		  lval = (lhvol & TLV_HEADPHONE_VOL_MASK);
		  if (lval < TLV_HEADPHONE_VOL_MIN) {
		  	lval = TLV_HEADPHONE_VOL_MIN;
			lhvol = (lhvol & ~TLV_HEADPHONE_VOL_MASK) | lval;
			tlv320aic_setreg(TLV_DACT, 0x000); /* inactive */
			tlv320aic_setreg(TLV_LHVOL, lhvol);
			tlv320aic_setreg(TLV_DACT, 0x001); /* active */
		  }
		  left = lval - TLV_HEADPHONE_VOL_MIN;

		  rhvol = tlv320aic_getreg(TLV_RHVOL);
		  rval = (rhvol & TLV_HEADPHONE_VOL_MASK);
		  if (rval < TLV_HEADPHONE_VOL_MIN) {
		  	rval = TLV_HEADPHONE_VOL_MIN;
			rhvol = (rhvol & ~TLV_HEADPHONE_VOL_MASK) | rval;
			tlv320aic_setreg(TLV_DACT, 0x000); /* inactive */
			tlv320aic_setreg(TLV_RHVOL, rhvol);
			tlv320aic_setreg(TLV_DACT, 0x001); /* active */
		  }
		  right = rval - TLV_HEADPHONE_VOL_MIN;
		}
		break;
	default:
		return -EINVAL;
	}
	return (left & 0xff) | ((right & 0xff) << 8);
}

static int
mxc_alsa_i2s_audio_set_volume(int type, int volume)
{
	u8 left, right;
	u16 lval, rval;

	DEBUG_FUNC();

	left  = (volume & 0x00ff);
	right = ((volume & 0xff00) >> 8);

	switch (type) {
	case MIXERID_LINE_INPUT_VOL:
		{
		  u16 livol, rivol;

		  lval = left;
		  if (lval > TLV_LINE_INPUT_VOL_MAX)
		  	lval = TLV_LINE_INPUT_VOL_MAX;
		  livol = tlv320aic_getreg(TLV_LIVOL);
		  livol = (livol & ~TLV_LINE_INPUT_VOL_MASK) | lval;

		  rval = right;
		  if (rval > TLV_LINE_INPUT_VOL_MAX)
		  	rval = TLV_LINE_INPUT_VOL_MAX;
		  rivol = tlv320aic_getreg(TLV_RIVOL);
		  rivol = (rivol & ~TLV_LINE_INPUT_VOL_MASK) | rval;

		  tlv320aic_setreg(TLV_DACT, 0x000); /* inactive */
		  tlv320aic_setreg(TLV_LIVOL, livol);
		  tlv320aic_setreg(TLV_RIVOL, rivol);
		  tlv320aic_setreg(TLV_DACT, 0x001); /* active */
		  break;
		}
	case MIXERID_HEADPHONE_VOL:
		{
		  u16 lhvol, rhvol;
		  lval = left + TLV_HEADPHONE_VOL_MIN;
		  if (lval > TLV_HEADPHONE_VOL_MAX)
		  	lval = TLV_HEADPHONE_VOL_MAX;
		  lhvol = tlv320aic_getreg(TLV_LHVOL);
		  lhvol = (lhvol & ~TLV_HEADPHONE_VOL_MASK) | lval;

		  rval = right + TLV_HEADPHONE_VOL_MIN;
		  if (rval > TLV_HEADPHONE_VOL_MAX)
		  	rval = TLV_HEADPHONE_VOL_MAX;
		  rhvol = tlv320aic_getreg(TLV_RHVOL);
		  rhvol = (rhvol & ~TLV_HEADPHONE_VOL_MASK) | rval;

		  tlv320aic_setreg(TLV_DACT, 0x000); /* inactive */
		  tlv320aic_setreg(TLV_LHVOL, lhvol);
		  tlv320aic_setreg(TLV_RHVOL, rhvol);
		  tlv320aic_setreg(TLV_DACT, 0x001); /* active */
		  break;
		}
	default:
		return -EINVAL;
	}
	
	return 0;
}

static int
mxc_alsa_i2s_audio_get_input_source(void)
{
	u16 apath_insel;
	apath_insel = (tlv320aic_getreg(TLV_APATH) & 0x004);
	return (apath_insel ? 1 : 0);
}

static int
mxc_alsa_i2s_audio_set_input_source(int source)
{
	u16 apath;
	apath = (tlv320aic_getreg(TLV_APATH) & ~0x004);
	apath |= (source ? 0x004 : 0x000);

	tlv320aic_setreg(TLV_DACT, 0x000); /* inactive */
	tlv320aic_setreg(TLV_APATH, apath);
	tlv320aic_setreg(TLV_DACT, 0x001); /* active */
	return 0;
}

static int
mxc_alsa_i2s_audio_set_sampling_rate(u32 sampling_rate)
{
	static u32 previous = 0;
	u8 pm;
	u16 rate;
	u16 reg;

	if (sampling_rate == previous)
		return 0;

	DEBUG_FUNC();

	switch (sampling_rate) {
	case 96000:
		pm = 1;
		rate = 0x1c;
		break;
	case 48000:
		pm = 2;
		rate = 0x00;
		break;
	case 32000:
		pm = 3;
		rate = 0x18;
		break;
	case 8000:
		pm = 12;
		rate = 0x0c;
		break;
	default:
		return -EINVAL;
	}

	ssi_tx_prescaler_modulus(SSI1, pm);
	ssi_rx_prescaler_modulus(SSI1, pm);

	reg = tlv320aic_getreg(TLV_SRATE);
	reg = (reg & ~0x3f) | rate;

	tlv320aic_setreg(TLV_DACT, 0x000); /* inactive */
	tlv320aic_setreg(TLV_SRATE, reg);
	tlv320aic_setreg(TLV_DACT, 0x001); /* active */

	previous = sampling_rate;

	/* The converter delays 20 clocks (max). */
	udelay(25*1000*1000/sampling_rate);

	codec_dump_register();

	return 0;
}

static void
mxc_alsa_i2s_audio_init_audio_multiplexer(void)
{
	/**
	 *  Port1 <===> Port5
	 *  Port2 ====> Internal
	 *  Port3 <==== Internal
	 *  Port4 <==== Internal
	 *  Port6 <==== Internal
	 *  Port7 <==== Internal
	 */

	/* Port 1 */
	writel(0xa5294000, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x00);
	writel(0x00008000, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x04);
	/* Port 2 */
	writel(0x00000800, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x08);
	writel(0x0000a00f, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x0c);
	/* Port 3 */
	writel(0x8c631800, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x10);
	writel(0x0000200f, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x14);
	/* Port 4 */
	writel(0x8c631800, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x18);
	writel(0x0000200f, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x1c);
	/* Port 5 */
	writel(0x00000000, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x20);
	writel(0x00000000, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x24);
	/* Port 6 */
	writel(0x8c631800, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x28);
	writel(0x0000200f, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x2c);
	/* Port 7 */
	writel(0x8c631800, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x30);
	writel(0x0000200f, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x34);

	DEBUG_INFO("* DAM @ Port1. PTCR: 0x%08x, PDCR: 0x%08x\n",
		   readl(IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x00),
		   readl(IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x04));

	DEBUG_INFO("* DAM @ Port5. PTCR: 0x%08x, PDCR: 0x%08x\n",
		   readl(IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x20),
		   readl(IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x24));
}

static int 
mxc_alsa_i2s_audio_init_i2s_controller(void)
{
	/* SCR, SIER */
	ssi_i2s_mode(SSI1, i2s_slave);
	ssi_network_mode(SSI1, false);
	ssi_synchronous_mode(SSI1, true);
	ssi_system_clock(SSI1, false);
	ssi_two_channel_mode(SSI1, true);  

	/* Tx: STCR */
	ssi_tx_fifo_enable(SSI1, ssi_fifo_0, true);
	ssi_tx_fifo_enable(SSI1, ssi_fifo_1, true);
	ssi_tx_bit0(SSI1, true);
	ssi_tx_clock_direction(SSI1, ssi_tx_rx_externally);
	ssi_tx_frame_direction(SSI1, ssi_tx_rx_externally);
	ssi_tx_clock_polarity(SSI1, ssi_tx_clock_on_falling_edge);
	ssi_tx_frame_sync_active(SSI1, ssi_frame_sync_active_low);
	ssi_tx_frame_sync_length(SSI1, true);
	ssi_tx_early_frame_sync(SSI1, ssi_frame_sync_one_bit_before);
	ssi_tx_shift_direction(SSI1, ssi_msb_first);

	/* Tx: STCCR */
	ssi_tx_clock_divide_by_two(SSI1, false);
	ssi_tx_clock_prescaler(SSI1, false);
	ssi_tx_prescaler_modulus(SSI1, 2);/* 1:96k,2:48k,3:32k,12:,8k */
	ssi_tx_word_length(SSI1, ssi_16_bits);
	ssi_tx_frame_rate(SSI1, 2);

	/* Rx: SRCR */
	ssi_rx_fifo_enable(SSI1, ssi_fifo_0, true);
	ssi_rx_fifo_enable(SSI1, ssi_fifo_1, true);
	ssi_rx_bit0(SSI1, true);
	ssi_rx_clock_direction(SSI1, ssi_tx_rx_externally);	
	ssi_rx_frame_direction(SSI1, ssi_tx_rx_externally);
	ssi_rx_clock_polarity(SSI1, ssi_rx_clock_on_rising_edge);
	ssi_rx_frame_sync_active(SSI1, ssi_frame_sync_active_low);
	ssi_rx_frame_sync_length(SSI1, true);
	ssi_rx_early_frame_sync(SSI1, ssi_frame_sync_one_bit_before);
	ssi_rx_shift_direction(SSI1, ssi_msb_first);

	/* Rx: SRCCR */
	ssi_rx_clock_divide_by_two(SSI1, false);
	ssi_rx_clock_prescaler(SSI1, false);
	ssi_rx_prescaler_modulus(SSI1, 2);/* 1:96k,2:48k,3:32k,12:,8k */
	ssi_rx_word_length(SSI1, ssi_16_bits);
	ssi_rx_frame_rate(SSI1, 2);

	/* SFCSR */
	ssi_tx_fifo_empty_watermark(SSI1, ssi_fifo_0, TX_WATERMARK);
	ssi_tx_fifo_empty_watermark(SSI1, ssi_fifo_1, TX_WATERMARK);
	ssi_rx_fifo_full_watermark(SSI1, ssi_fifo_0, RX_WATERMARK);
	ssi_rx_fifo_full_watermark(SSI1, ssi_fifo_1, RX_WATERMARK);

	/* Misc */
	ssi_ac97_mode_enable(SSI1, false);

	i2s_dump_register(SSI1);

	return 0;
}

static int
mxc_alsa_i2s_audio_init_codec(void)
{
	DEBUG_FUNC();

	tlv320aic_setreg(TLV_RESET, 0);
	tlv320aic_setreg(TLV_DACT, 0x000); /* inactive */

	/* default sampling rate: 48k */
	tlv320aic_setreg(TLV_LIVOL, 0x010);
	tlv320aic_setreg(TLV_RIVOL, 0x010);
	tlv320aic_setreg(TLV_LHVOL, 0xe0);
	tlv320aic_setreg(TLV_RHVOL, 0xe0);
	tlv320aic_setreg(TLV_APATH, 0x14);/* 14: INPUT:Mic, 10: INPUT:Line */
	tlv320aic_setreg(TLV_DPATH, 0x06);
	tlv320aic_setreg(TLV_PWR, 0x000);
	tlv320aic_setreg(TLV_DIFORM, 0x042);
	tlv320aic_setreg(TLV_SRATE, 0x000);
	tlv320aic_setreg(TLV_DACT, 0x001); /* active */

	return 0;
}

static int
mxc_alsa_i2s_audio_enable(void)
{
	DEBUG_FUNC();
	return 0;
}

static int 
mxc_alsa_i2s_audio_disable(void)
{
	DEBUG_FUNC();

	ssi_enable(SSI1, false);
	ssi_transmit_enable(SSI1, false);
	ssi_receive_enable(SSI1, false);

	ssi_interrupt_disable(SSI1, ssi_tx_interrupt_enable);
	ssi_interrupt_disable(SSI1, ssi_rx_interrupt_enable);
	ssi_interrupt_disable(SSI1, ssi_tx_dma_interrupt_enable);
	ssi_interrupt_disable(SSI1, ssi_rx_dma_interrupt_enable);
	ssi_interrupt_disable(SSI1, ssi_tx_fifo_0_empty);
	ssi_interrupt_disable(SSI1, ssi_tx_fifo_1_empty);
	ssi_interrupt_disable(SSI1, ssi_rx_fifo_0_full);
	ssi_interrupt_disable(SSI1, ssi_rx_fifo_1_full);

	return 0;
}

static int
mxc_alsa_i2s_audio_sysclk_check(struct platform_device *pdev)
{
	struct clk *clk = clk_get(&pdev->dev, "serial_pll");
	u32 ccmr = readl(IO_ADDRESS(CCM_BASE_ADDR) + 0x00);
	u32 pdr1 = readl(IO_ADDRESS(CCM_BASE_ADDR) + 0x08);
	u32 spll = clk_get_rate(clk);
	u32 ssi1_pre_podf, ssi1_podf;
	u32 ssi1_select;
	u32 sysclk;

	ssi1_select = ((ccmr & 0xc0000) >> 18);
	ssi1_pre_podf = ((pdr1 & 0x1c0) >> 6);
	ssi1_podf = (pdr1 & 0x3f);
	sysclk = spll / ((ssi1_pre_podf + 1) * (ssi1_podf + 1));

	DEBUG_INFO("ssi1_select: %d\n", ssi1_select);
	DEBUG_INFO("ssi1_pre_podf: %d, ssi1_podf: %d\n",
		   ssi1_pre_podf, ssi1_podf);
	DEBUG_INFO("sys_clk: %d\n", sysclk);

	if (ssi1_select != 2) /* serial_pll_clk */
		return -ENODEV;

	/* check sysclk 24.576MHz */
	if (sysclk < 24575000 || 24577000 < sysclk)
		return -ENODEV;

	return 0;
}

static int
mxc_alsa_i2s_audio_init(struct platform_device *pdev)
{
	int ret;

	DEBUG_FUNC();

	ret = mxc_alsa_i2s_audio_sysclk_check(pdev);
	if (ret < 0)
		return ret;

	gpio_audio_active(5);

	mxc_alsa_i2s_audio_disable();

	mxc_alsa_i2s_audio_init_audio_multiplexer();
	ret = mxc_alsa_i2s_audio_init_i2s_controller();
	if (ret < 0)
		return ret;
	ret = mxc_alsa_i2s_audio_init_codec();
	if (ret < 0)
		return ret;

	ret = mxc_alsa_i2s_audio_set_sampling_rate(DEFAULT_SAMPLING_RATE);
	if (ret < 0)
		return ret;
	ret = mxc_alsa_i2s_audio_set_volume(MIXERID_LINE_INPUT_VOL,
					    DEFAULT_LINE_INPUT_VOLUME);
	if (ret < 0)
		return ret;
	ret = mxc_alsa_i2s_audio_set_volume(MIXERID_HEADPHONE_VOL,
					    DEFAULT_HEADPHONE_VOLUME);
	if (ret < 0)
		return ret;

	ret = mxc_alsa_i2s_audio_enable();
	if (ret < 0)
		return ret;

	return 0;
}

static int
mxc_alsa_i2s_audio_shutdown(struct platform_device *pdev)
{
	DEBUG_FUNC();

	mxc_alsa_i2s_audio_disable();

	return 0;
}

/* module power management */
#if defined(CONFIG_PM)
#include <linux/pm.h>
static int 
mxc_alsa_i2s_suspend(struct platform_device *pdev, pm_message_t state)
{
	DEBUG_FUNC();
	return 0;
}

static int 
mxc_alsa_i2s_resume(struct platform_device *pdev)
{
	DEBUG_FUNC();
	return 0;
}
#endif /* CONFIG_PM */

static int
mxc_mixer_volume_info(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	int mixerid = kcontrol->private_value;

	return mxc_alsa_i2s_audio_get_volume_info(mixerid, uinfo);
}

static int
mxc_mixer_volume_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *uvalue)
{
	int mixerid = kcontrol->private_value;
	int val;

	val = mxc_alsa_i2s_audio_get_volume(mixerid);
	if (val < 0)
		return val;

	uvalue->value.integer.value[0] = val & 0xff;
	uvalue->value.integer.value[1] = (val >> 8) & 0xff;

	return 0;
}

static int
mxc_mixer_volume_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *uvalue)
{
	int mixerid = kcontrol->private_value;
	int val;

	val = (uvalue->value.integer.value[0] | 
	       (uvalue->value.integer.value[1] << 8));

	return mxc_alsa_i2s_audio_set_volume(mixerid, val);
}

static struct snd_kcontrol_new mxc_alsa_i2s_line_input_vol_ctrl = {
	.iface		= SNDRV_CTL_ELEM_IFACE_MIXER,
	.name		= "Line Playback Volume",
	.index		= 0x00,
	.info		= mxc_mixer_volume_info,
	.get		= mxc_mixer_volume_get,
	.put		= mxc_mixer_volume_put,
	.private_value	= MIXERID_LINE_INPUT_VOL,
};

static struct snd_kcontrol_new mxc_alsa_i2s_headphone_vol_ctrl = {
	.iface		= SNDRV_CTL_ELEM_IFACE_MIXER,
	.name		= "Headphone Playback Volume",
	.index		= 0x00,
	.info		= mxc_mixer_volume_info,
	.get		= mxc_mixer_volume_get,
	.put		= mxc_mixer_volume_put,
	.private_value	= MIXERID_HEADPHONE_VOL,
};

static int
mxc_mixer_input_source_info(struct snd_kcontrol *kcontrol,
			    struct snd_ctl_elem_info *uinfo)
{
	static char *label[2] = {
		"Line", "Mic"
	};

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 1;
	uinfo->value.enumerated.items = 2;
	if (uinfo->value.enumerated.item >= uinfo->value.enumerated.items) {
		uinfo->value.enumerated.item =
			uinfo->value.enumerated.items - 1;
	}
	strcpy(uinfo->value.enumerated.name,
	       label[uinfo->value.enumerated.item]);
	return 0;
}

static int
mxc_mixer_input_source_get(struct snd_kcontrol *kcontrol,
			    struct snd_ctl_elem_value *uvalue)
{
	uvalue->value.enumerated.item[0] =
		mxc_alsa_i2s_audio_get_input_source();
	return 0;
}

static int
mxc_mixer_input_source_put(struct snd_kcontrol *kcontrol,
			    struct snd_ctl_elem_value *uvalue)
{
	mxc_alsa_i2s_audio_set_input_source(uvalue->value.enumerated.item[0]);
	return 0;
}

static struct snd_kcontrol_new mxc_alsa_i2s_input_source_ctrl = {
	.iface		= SNDRV_CTL_ELEM_IFACE_MIXER,
	.name		= "Input Source",
	.count		= 1,
	.info		= mxc_mixer_input_source_info,
	.get		= mxc_mixer_input_source_get,
	.put		= mxc_mixer_input_source_put,
};

static int 
mxc_alsa_i2s_mixer_new(struct snd_card *card)
{
	struct snd_kcontrol *kctl;
	int ret;

	DEBUG_FUNC();

	kctl = snd_ctl_new1(&mxc_alsa_i2s_line_input_vol_ctrl, NULL);
	ret = snd_ctl_add(card, kctl);
	if (ret < 0)
		return ret;

	kctl = snd_ctl_new1(&mxc_alsa_i2s_headphone_vol_ctrl, NULL);
	ret = snd_ctl_add(card, kctl);
	if (ret < 0)
		return ret;

	kctl = snd_ctl_new1(&mxc_alsa_i2s_input_source_ctrl, NULL);
	ret = snd_ctl_add(card, kctl);
	if (ret < 0)
		return ret;

	return 0;
}

static struct snd_pcm_hardware mxc_alsa_i2s_pcm_hardware = {
	.info             = (SNDRV_PCM_INFO_MMAP |
			     SNDRV_PCM_INFO_MMAP_VALID |
			     SNDRV_PCM_INFO_NONINTERLEAVED |
			     SNDRV_PCM_INFO_PAUSE),
	.formats          = (SNDRV_PCM_FMTBIT_S16_LE),
	.rates            = (SNDRV_PCM_RATE_8000 |
			     SNDRV_PCM_RATE_32000 |
			     SNDRV_PCM_RATE_48000 |
			     SNDRV_PCM_RATE_96000),
	.rate_min         = 8000,
	.rate_max         = 96000,
	.channels_min     = 2,
	.channels_max     = 2,
	.period_bytes_min = MIN_PERIOD_SIZE,
	.period_bytes_max = DMA_BUFFER_SIZE,
	.periods_min      = MIN_PERIOD,
	.periods_max      = MAX_PERIOD,
	.buffer_bytes_max = MAX_BUFFER_SIZE,
	.fifo_size        = 0,
};

static int 
mxc_alsa_i2s_pcm_open(struct snd_pcm_substream *substream)
{
	struct audio_private *priv;
	struct snd_pcm_runtime *runtime;
	mxc_dma_device_t dma_id_0, dma_id_1;
	mxc_dma_callback_t dma_callback;
	int ret;

	DEBUG_FUNC();

	priv = snd_pcm_substream_chip(substream);
	runtime = substream->runtime;

	runtime->hw = mxc_alsa_i2s_pcm_hardware;

	switch (substream->stream) {
	case SNDRV_PCM_STREAM_PLAYBACK:
		dma_id_0 = MXC_DMA_SSI1_16BIT_TX0;
		dma_id_1 = MXC_DMA_SSI1_16BIT_TX1;
		dma_callback = mxc_alsa_i2s_dma_playback_callback;

		ssi_two_channel_mode(SSI1, true);
		break;
	case SNDRV_PCM_STREAM_CAPTURE:
		dma_id_0 = MXC_DMA_SSI1_16BIT_RX0;
		dma_id_1 = MXC_DMA_SSI1_16BIT_RX1;
		dma_callback = mxc_alsa_i2s_dma_capture_callback;

		ssi_two_channel_mode(SSI1, true);
		break;
	default:
		return -EINVAL;
	}

	ret = mxc_alsa_i2s_dma_request(dma_id_0, dma_callback, substream);
	if (ret < 0) {
		pr_info("DMA Request failed\n");
		return ret;
	}
	ret = mxc_alsa_i2s_dma_request(dma_id_1, dma_callback, substream);
	if (ret < 0) {
		pr_info("DMA Request failed\n");
		return ret;
	}

	return 0;
}

static int
mxc_alsa_i2s_pcm_close(struct snd_pcm_substream *substream)
{
	struct audio_private *priv;
	mxc_dma_device_t dma_id_0, dma_id_1;

	DEBUG_FUNC();

	priv = snd_pcm_substream_chip(substream);

	switch (substream->stream) {
	case SNDRV_PCM_STREAM_PLAYBACK:
		dma_id_0 = MXC_DMA_SSI1_16BIT_TX0;
		dma_id_1 = MXC_DMA_SSI1_16BIT_TX1;
		break;
	case SNDRV_PCM_STREAM_CAPTURE:
		dma_id_0 = MXC_DMA_SSI1_16BIT_RX0;
		dma_id_1 = MXC_DMA_SSI1_16BIT_RX1;
		break;
	default:
		return -EINVAL;
	}

	mxc_alsa_i2s_dma_free(dma_id_0, priv);
	mxc_alsa_i2s_dma_free(dma_id_1, priv);

	ssi_enable(SSI1, false);

	return 0;
}

static int
mxc_alsa_i2s_pcm_hw_params(struct snd_pcm_substream *substream,
			   struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime;
	int ret;

	DEBUG_FUNC();

	runtime = substream->runtime;

	ret = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
	if (ret < 0)
		return ret;
	runtime->dma_addr = virt_to_phys(runtime->dma_area);

	memset(runtime->dma_area, 0, params_buffer_bytes(params));

	return 0;
}

static int
mxc_alsa_i2s_pcm_hw_free(struct snd_pcm_substream *substream)
{
	DEBUG_FUNC();
	return snd_pcm_lib_free_pages(substream);
}

static int
mxc_alsa_i2s_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct audio_private *priv;
	struct snd_pcm_runtime *runtime;
	struct audio_stream *stream;
	int ret;

	DEBUG_FUNC();

	priv = snd_pcm_substream_chip(substream);
	runtime = substream->runtime;

	DEBUG_INFO("runtime:rate        = %d\n", runtime->rate);
	DEBUG_INFO("runtime:frame_bits  = %d\n", runtime->frame_bits);
	DEBUG_INFO("runtime:channel     = %d\n", runtime->channels);

	ret = mxc_alsa_i2s_audio_set_sampling_rate(runtime->rate);
	if (ret < 0)
		return ret;

	switch (substream->stream) {
	case SNDRV_PCM_STREAM_PLAYBACK:
		ssi_interrupt_enable(SSI1, ssi_tx_dma_interrupt_enable);
		ssi_interrupt_enable(SSI1, ssi_tx_fifo_0_empty);
		ssi_interrupt_enable(SSI1, ssi_tx_fifo_1_empty);
		stream = priv->playback_stream;
		break;
	case SNDRV_PCM_STREAM_CAPTURE:
		ssi_interrupt_enable(SSI1, ssi_rx_dma_interrupt_enable);
		ssi_interrupt_enable(SSI1, ssi_rx_fifo_0_full);
		ssi_interrupt_enable(SSI1, ssi_rx_fifo_1_full);
		stream = priv->capture_stream;
		break;
	default:
		return -EINVAL;
	}
	stream->current_period = 0;
	stream->complete_period = 0;

	return 0;
}

static int
mxc_alsa_i2s_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct audio_private *priv;
	struct audio_stream *stream;
	struct pcm_trigger_ops {
		int (*start)(struct snd_pcm_substream *substream);
		int (*stop)(struct snd_pcm_substream *substream);
	} ops;
	int ret = 0;

	DEBUG_FUNC();

	priv = snd_pcm_substream_chip(substream);
	memset(&ops, 0, sizeof(struct pcm_trigger_ops));

	switch (substream->stream) {
	case SNDRV_PCM_STREAM_PLAYBACK:
		ops.start = mxc_alsa_i2s_dma_playback;
		ops.stop  = mxc_alsa_i2s_dma_playback_stop;
		stream = priv->playback_stream;
		break;
	case SNDRV_PCM_STREAM_CAPTURE:
		ops.start = mxc_alsa_i2s_dma_capture;
		ops.stop  = mxc_alsa_i2s_dma_capture_stop;
		stream = priv->capture_stream;
		break;
	default:
		return -EINVAL;
	}

	spin_lock(&stream->dma_lock);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		DEBUG_INFO("%s(): TRIGGER_START\n", __FUNCTION__);
		ssi_enable(SSI1, true);
		ops.start(substream);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		DEBUG_INFO("%s(): TRIGGER_STOP\n", __FUNCTION__);
		ops.stop(substream);
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
		DEBUG_INFO("%s(): TRIGGER_SUSPEND\n", __FUNCTION__);
		break;
	case SNDRV_PCM_TRIGGER_RESUME:
		DEBUG_INFO("%s(): TRIGGER_RESUME\n", __FUNCTION__);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		DEBUG_INFO("%s(): TRIGGER_PAUSE_PUSH\n", __FUNCTION__);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		DEBUG_INFO("%s(): TRIGGER_PAUSE_RELEASE\n", __FUNCTION__);
		break;
	default:
		DEBUG_INFO("%s(): invalied command(%08x)\n",
			   __FUNCTION__, cmd); 
		ret = -EINVAL;
		break;
	}
	spin_unlock(&stream->dma_lock);

	return ret;
}

static snd_pcm_uframes_t
mxc_alsa_i2s_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct audio_private *priv;
	struct snd_pcm_runtime *runtime;
	struct audio_stream *stream;
	u32 offset;

	DEBUG_FUNC();

	priv = snd_pcm_substream_chip(substream);  
	runtime = substream->runtime;

	switch (substream->stream) {
	case SNDRV_PCM_STREAM_PLAYBACK:
		stream = priv->playback_stream;
		break;
	case SNDRV_PCM_STREAM_CAPTURE:
		stream = priv->capture_stream;
		break;
	default:
		return -EINVAL;
	}

	offset = runtime->period_size * (stream->complete_period);
	if (offset >= runtime->buffer_size)
		offset = 0;

	return offset;
}

static struct snd_pcm_ops mxc_alsa_i2s_pcm_playback_ops = {
	.open		= mxc_alsa_i2s_pcm_open,
	.close		= mxc_alsa_i2s_pcm_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= mxc_alsa_i2s_pcm_hw_params,
	.hw_free	= mxc_alsa_i2s_pcm_hw_free,
	.prepare	= mxc_alsa_i2s_pcm_prepare,
	.trigger	= mxc_alsa_i2s_pcm_trigger,
	.pointer	= mxc_alsa_i2s_pcm_pointer,
};

static struct snd_pcm_ops mxc_alsa_i2s_pcm_capture_ops = {
	.open		= mxc_alsa_i2s_pcm_open,
	.close		= mxc_alsa_i2s_pcm_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= mxc_alsa_i2s_pcm_hw_params,
	.hw_free	= mxc_alsa_i2s_pcm_hw_free,
	.prepare	= mxc_alsa_i2s_pcm_prepare,
	.trigger	= mxc_alsa_i2s_pcm_trigger,
	.pointer	= mxc_alsa_i2s_pcm_pointer,
};

static int 
mxc_alsa_i2s_pcm_new(struct snd_card *card)
{
	struct snd_pcm *pcm;
	int play = 1;
	int capt = 1;
	int ret;

	DEBUG_FUNC();
	ret = snd_pcm_new(card, "MXC-i2s-PCM", 0, play, capt, &pcm);
	if (ret)
		return ret;

	ret = snd_pcm_lib_preallocate_pages_for_all
	  (pcm, SNDRV_DMA_TYPE_CONTINUOUS, snd_dma_continuous_data(GFP_KERNEL),
	   MAX_BUFFER_SIZE, MAX_BUFFER_SIZE);

	pcm->private_data = card->private_data;
	pcm->private_free = NULL;

	/* alsa pcm ops setting for SNDRV_PCM_STREAM_PLAYBACK */
	if (play)
		snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK,
				&mxc_alsa_i2s_pcm_playback_ops);

	/* alsa pcm ops setting for SNDRV_PCM_STREAM_CAPTURE */
	if (capt)
		snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE,
				&mxc_alsa_i2s_pcm_capture_ops);

	return 0;
}

static int __devinit 
mxc_alsa_i2s_probe(struct platform_device *pdev)
{
	struct snd_card *card;
	struct clk *clk;
	int ret;

	DEBUG_FUNC();

	clk = clk_get(&pdev->dev, "ssi_clk");
	if (IS_ERR(clk))
		return PTR_ERR(clk);
	clk_enable(clk);

	ret = mxc_alsa_i2s_audio_init(pdev);
	if (ret < 0) {
		ret = -EIO;
		goto err_init;
	}

	card = snd_card_new(-1, NULL, THIS_MODULE, sizeof(struct audio_private));
	if (card == NULL) {
		ret = -ENOMEM;
		goto err_init;
	}

	card->dev = &pdev->dev;

	ret = mxc_alsa_i2s_register_private(card, clk);
	if (ret < 0) {
		DEBUG_INFO("%s(): Register Private failed\n", __FUNCTION__);
		goto nodev;
	}

	strcpy(card->driver,    "mxc i2s");
	strcpy(card->shortname, "mxc i2s audio");
	strcpy(card->longname,  "mxc i2s audio");

	ret = mxc_alsa_i2s_pcm_new(card);
	if (ret < 0)
		goto nodev;

	ret = mxc_alsa_i2s_mixer_new(card);
	if (ret < 0)
		goto nodev;

	ret = snd_card_register(card);
	if (ret < 0)
		goto nodev;

	platform_set_drvdata(pdev, card);

	dev_info(&pdev->dev, "%s\n", DESCRIPTION);

	return 0;

 nodev:
	snd_card_free(card);
 err_init:
	clk_disable(clk);
	clk_put(clk);
	return ret;
}

static int 
mxc_alsa_i2s_remove(struct platform_device *pdev)
{
	struct snd_card *card;
	struct audio_private *priv;

	DEBUG_FUNC();

	mxc_alsa_i2s_audio_shutdown(pdev);

	card = platform_get_drvdata(pdev);
	if (card) {
		priv = card->private_data;
		clk_disable(priv->clk);
		clk_put(priv->clk);
		snd_card_free(card);
		platform_set_drvdata(pdev, NULL);
	}

	return 0;
}

//static struct platform_device *mxc_alsa_i2s_device;
static struct platform_driver mxc_alsa_i2s_driver = {
	.probe		= mxc_alsa_i2s_probe,
	.remove		= __devexit_p(mxc_alsa_i2s_remove),
#if defined(CONFIG_PM)
	.suspend	= mxc_alsa_i2s_suspend,
	.resume		= mxc_alsa_i2s_resume,
#endif
	.driver		= {
		.name	= DRIVER_NAME,
	},
};

static int __init 
mxc_alsa_i2s_init(void)
{
 	pr_info(DESCRIPTION " [" REVISION "], (C) 2007-2008 " AUTHOR "\n");

	return platform_driver_register(&mxc_alsa_i2s_driver);
}

static void __exit 
mxc_alsa_i2s_exit(void)
{
	platform_driver_unregister(&mxc_alsa_i2s_driver);
}

module_init(mxc_alsa_i2s_init);
module_exit(mxc_alsa_i2s_exit);

MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESCRIPTION);
MODULE_LICENSE("GPL");
