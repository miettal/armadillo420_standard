/*
 * armadillo500fx-wm8978.c
 *   --  Armadillo-500 FX Driver for Wolfson WM8978 Codec
 *
 * Copyright 2008 Atmark Techno, Inc.
 *
 * Based on imx32ads-wm8350.c by Liam Girdwood,
 *   Copyright 2007 Wolfson Microelectronics PLC.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/bitops.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <asm/hardware.h>
#include <asm/io.h>

#include "../codecs/wm8978.h"
#include "imx-ssi.h"
#include "imx31-pcm.h"

//#define ARMADILLO500FX_WM8978_MCLOCK 12000000

#ifndef ARMADILLO500FX_WM8978_MCLOCK
#define ARMADILLO500FX_WM8978_MCLOCK 12288000
#endif

static int armadillo500fx_hifi_hw_params(struct snd_pcm_substream *substream,
				   struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_cpu_dai *cpu_dai = rtd->dai->cpu_dai;
	struct snd_soc_codec_dai *codec_dai = rtd->dai->codec_dai;
	unsigned int rate = params_rate(params);
	unsigned int channels = 2;
	u32 dai_format, bclkdiv, mclkdiv, mclksel, sysclk;
	int ret;

	switch (rate) {
#if ARMADILLO500FX_WM8978_MCLOCK == 12288000
	case 8000:
		bclkdiv = WM8978_BCLK_DIV_4;
		mclkdiv = WM8978_MCLK_DIV_6;
		mclksel = WM8978_MCLK_MCLK;
		sysclk = 0;
		break;
	case 12000:
		bclkdiv = WM8978_BCLK_DIV_4;
		mclkdiv = WM8978_MCLK_DIV_4;
		mclksel = WM8978_MCLK_MCLK;
		sysclk = 0;
		break;
	case 16000:
		bclkdiv = WM8978_BCLK_DIV_4;
		mclkdiv = WM8978_MCLK_DIV_3;
		mclksel = WM8978_MCLK_MCLK;
		sysclk = 0;
		break;
	case 24000:
		bclkdiv = WM8978_BCLK_DIV_4;
		mclkdiv = WM8978_MCLK_DIV_2;
		mclksel = WM8978_MCLK_MCLK;
		sysclk = 0;
		break;
	case 32000:
		bclkdiv = WM8978_BCLK_DIV_4;
		mclkdiv = WM8978_MCLK_DIV_1_5;
		mclksel = WM8978_MCLK_MCLK;
		sysclk = 0;
		break;
	case 48000:
		bclkdiv = WM8978_BCLK_DIV_4;
		mclkdiv = WM8978_MCLK_DIV_1;
		mclksel = WM8978_MCLK_MCLK;
		sysclk = 0;
		break;
	case 11025:
		bclkdiv = WM8978_BCLK_DIV_4;
		mclkdiv = WM8978_MCLK_DIV_8;
		mclksel = WM8978_MCLK_PLL;
		sysclk = 11289600;
		break;
	case 22050:
		bclkdiv = WM8978_BCLK_DIV_4;
		mclkdiv = WM8978_MCLK_DIV_4;
		mclksel = WM8978_MCLK_PLL;
		sysclk = 11289600;
		break;
	case 44100:
		bclkdiv = WM8978_BCLK_DIV_4;
		mclkdiv = WM8978_MCLK_DIV_2;
		mclksel = WM8978_MCLK_PLL;
		sysclk = 11289600;
		break;
#else
	case 8000:
		bclkdiv = WM8978_BCLK_DIV_4;
		mclkdiv = WM8978_MCLK_DIV_12;
		mclksel = WM8978_MCLK_PLL;
		sysclk = 12288000;
		break;
	case 12000:
		bclkdiv = WM8978_BCLK_DIV_4;
		mclkdiv = WM8978_MCLK_DIV_8;
		mclksel = WM8978_MCLK_PLL;
		sysclk = 12288000;
		break;
	case 16000:
		bclkdiv = WM8978_BCLK_DIV_4;
		mclkdiv = WM8978_MCLK_DIV_6;
		mclksel = WM8978_MCLK_PLL;
		sysclk = 12288000;
		break;
	case 24000:
		bclkdiv = WM8978_BCLK_DIV_4;
		mclkdiv = WM8978_MCLK_DIV_4;
		mclksel = WM8978_MCLK_PLL;
		sysclk = 12288000;
		break;
	case 32000:
		bclkdiv = WM8978_BCLK_DIV_4;
		mclkdiv = WM8978_MCLK_DIV_3;
		mclksel = WM8978_MCLK_PLL;
		sysclk = 12288000;
		break;
	case 48000:
		bclkdiv = WM8978_BCLK_DIV_4;
		mclkdiv = WM8978_MCLK_DIV_2;
		mclksel = WM8978_MCLK_PLL;
		sysclk = 12288000;
		break;
	case 11025:
		bclkdiv = WM8978_BCLK_DIV_4;
		mclkdiv = WM8978_MCLK_DIV_8;
		mclksel = WM8978_MCLK_PLL;
		sysclk = 11289600;
		break;
	case 22050:
		bclkdiv = WM8978_BCLK_DIV_4;
		mclkdiv = WM8978_MCLK_DIV_4;
		mclksel = WM8978_MCLK_PLL;
		sysclk = 11289600;
		break;
	case 44100:
		bclkdiv = WM8978_BCLK_DIV_4;
		mclkdiv = WM8978_MCLK_DIV_2;
		mclksel = WM8978_MCLK_PLL;
		sysclk = 11289600;
		break;
#endif
	default:
		return -EINVAL;
	}

	dai_format = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
	    SND_SOC_DAIFMT_CBM_CFM | SND_SOC_DAIFMT_SYNC;

	if (channels == 2)
		dai_format |= SND_SOC_DAIFMT_TDM;

	/* set codec DAI configuration */
	ret = codec_dai->dai_ops.set_fmt(codec_dai, dai_format);
	if (ret < 0)
		return ret;

	/* set cpu DAI configuration */
	ret = cpu_dai->dai_ops.set_fmt(cpu_dai, dai_format);
	if (ret < 0)
		return ret;

	/* set i.MX active slot mask */
	ret = cpu_dai->dai_ops.set_tdm_slot(cpu_dai,
					    channels == 1 ?
					    0xfffffffe : 0xfffffffc,
					    channels);
	if (ret < 0)
		return ret;

	/* set the SSI system clock as input (unused) */
	ret = cpu_dai->dai_ops.set_sysclk(cpu_dai, IMX_SSP_SYS_CLK,
					  0, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	/* set codec BCLK division for sample rate */
	ret = codec_dai->dai_ops.set_clkdiv(codec_dai, WM8978_BCLKDIV,
					    bclkdiv);
	if (ret < 0)
		return ret;

	/* set codec MCLK division for 256fs */
	ret = codec_dai->dai_ops.set_clkdiv(codec_dai, WM8978_MCLKDIV,
					    mclkdiv);
	if (ret < 0)
		return ret;

	/* select the source of the clock */
	ret = codec_dai->dai_ops.set_clkdiv(codec_dai, WM8978_MCLKSEL,
					    mclksel);
	if (ret < 0)
		return ret;

	/* codec PLL input is rate from MCLK */
	ret = codec_dai->dai_ops.set_pll(codec_dai, 0,
					 ARMADILLO500FX_WM8978_MCLOCK, sysclk);
	if (ret < 0)
		return ret;

	return 0;
}

static int armadillo500fx_hifi_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec_dai *codec_dai = rtd->dai->codec_dai;

	/* disable the PLL */
	codec_dai->dai_ops.set_clkdiv(codec_dai, WM8978_MCLKSEL,
				      WM8978_MCLK_MCLK);
	codec_dai->dai_ops.set_pll(codec_dai, 0, 0, 0);
	return 0;
}

/*
 * armadillo500fx WM8978 HiFi DAI opserations.
 */
static struct snd_soc_ops armadillo500fx_hifi_ops = {
	.hw_params = armadillo500fx_hifi_hw_params,
	.hw_free = armadillo500fx_hifi_hw_free,
};

/* armadillo500fx soc_card dapm widgets */
static const struct snd_soc_dapm_widget armadillo500fx_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_SPK("Speaker", NULL),
	SND_SOC_DAPM_MIC("Mic Jack", NULL),
};

static const char* audio_map[][3] = {
	/* headphone connected to LOUT1, ROUT1 */
	{"Headphone Jack", NULL, "LOUT1"},
	{"Headphone Jack", NULL, "ROUT1"},

	/* speaker connected to LOUT2, ROUT2 */
	{"Speaker", NULL, "ROUT2"},
	{"Speaker", NULL, "LOUT2"},

	/* mic is connected to LIP */
	{"LIP", NULL, "Mic Jack"},

	{NULL, NULL, NULL},
};

extern void gpio_audio_active(int select);
/*
 * This is an example machine initialisation for a wm8978 connected to a
 * armadillo500fx II. It is missing logic to detect hp/mic insertions and logic
 * to re-route the audio in such an event.
 */
static int armadillo500fx_wm8978_init(struct snd_soc_codec *codec)
{
	int ret, i;

	gpio_audio_active(5);

	/* Port 1 */
	writel(0xa5000800, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x00);
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
	writel(0x00000800, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x20);
	writel(0x00000000, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x24);
	/* Port 6 */
	writel(0x8c631800, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x28);
	writel(0x0000200f, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x2c);
	/* Port 7 */
	writel(0x8c631800, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x30);
	writel(0x0000200f, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x34);

	/*
	 * setup machine codec defaults
	 * 
	 * note: all volume update bits are also enabled here to ensure
	 * individual left/right control changes are made effective immediately.
	 */ 

	/* digital playback volume (90%) */
	codec->write(codec, WM8978_DACVOLL, 0x1e6);
	codec->write(codec, WM8978_DACVOLR, 0x1e6);

	/* digital capture volume (80%) */
	codec->write(codec, WM8978_ADCVOLL, 0x1cc);
	codec->write(codec, WM8978_ADCVOLR, 0x1cc);

	/* capture volume (80%) */
	codec->write(codec, WM8978_INPPGAL, 0x133);
	codec->write(codec, WM8978_INPPGAR, 0x133);

	/* headphone volume (70%) */
	codec->write(codec, WM8978_HPVOLL, 0x12c);
	codec->write(codec, WM8978_HPVOLR, 0x12c);

	/* speaker volume (90%) */
	codec->write(codec, WM8978_SPKVOLL, 0x139);
	codec->write(codec, WM8978_SPKVOLR, 0x139);

	/* output: enable thermal shutdown */
	codec->write(codec, WM8978_OUTPUT, 0x002); /* + 0x4 for speaker boost */
	/* output: invert ROUT2 for speaker */
	codec->write(codec, WM8978_BEEP, 0x010);
	/* output: OUT3 used as a VMID buffer */
	codec->write(codec, WM8978_OUT3MIX, 0x040);

	/* inputs (enable LIP and LIN) */
	codec->write(codec, WM8978_INPUT, 0x003);

	/* GPIO/jack */
	codec->write(codec, WM8978_ADD, 0x001);
	codec->write(codec, WM8978_JACK1, 0x040);
//	codec->write(codec, WM8978_JACK1, 0x0c0);
	codec->write(codec, WM8978_JACK2, 0x012);

	/* set up NC codec pins */
	snd_soc_dapm_set_endpoint(codec, "RIP", 0);
	snd_soc_dapm_set_endpoint(codec, "RIN", 0);
	snd_soc_dapm_set_endpoint(codec, "AUXL", 0);
	snd_soc_dapm_set_endpoint(codec, "AUXR", 0);
	snd_soc_dapm_set_endpoint(codec, "OUT3", 0);
	snd_soc_dapm_set_endpoint(codec, "OUT4", 0);

	/* Add armadillo500fx specific widgets */
	for (i = 0; i < ARRAY_SIZE(armadillo500fx_dapm_widgets); i++) {
		ret = snd_soc_dapm_new_control
		  (codec, &armadillo500fx_dapm_widgets[i]);
		if (ret < 0) {
			printk("widgets %d ret: %d\n", i, ret);
		}
	}

	/* set up armadillo500fx audio specific path audio_map */
	for (i = 0; audio_map[i][0] != NULL; i++) {
		ret = snd_soc_dapm_connect_input(codec, audio_map[i][0],
			audio_map[i][1], audio_map[i][2]);
		if (ret < 0)
			return ret;
	}

	snd_soc_dapm_sync_endpoints(codec);
	return 0;
}

static struct snd_soc_dai_link armadillo500fx_dai[] = {
{ /* Hifi Playback */
	.name = "WM8978",
	.stream_name = "WM8978 HiFi",
	.cpu_dai = &imx_ssi_pcm_dai[0],
	.codec_dai = &wm8978_dai,
	.init = armadillo500fx_wm8978_init,
	.ops = &armadillo500fx_hifi_ops,
},
};

static struct snd_soc_machine armadillo500fx = {
	.name = "armadillo500fx",
	.dai_link = armadillo500fx_dai,
	.num_links = ARRAY_SIZE(armadillo500fx_dai),
};

static struct wm8978_setup_data armadillo500fx_wm8978_setup = {
	.i2c_address = 0x1a,
};

static struct snd_soc_device armadillo500fx_snd_devdata = {
	.machine = &armadillo500fx,
	.platform = &imx3x_platform,
	.codec_dev = &soc_codec_dev_wm8978,
	.codec_data = &armadillo500fx_wm8978_setup,
};

static struct platform_device *armadillo500fx_snd_device;

static int __init armadillo500fx_init(void)
{
	int ret;

	armadillo500fx_snd_device = platform_device_alloc("soc-audio", 0);
	if (!armadillo500fx_snd_device)
		return -ENOMEM;

	get_ssi_clk(0, &armadillo500fx_snd_device->dev);
	platform_set_drvdata(armadillo500fx_snd_device,
			     &armadillo500fx_snd_devdata);
	armadillo500fx_snd_devdata.dev = &armadillo500fx_snd_device->dev;
	ret = platform_device_add(armadillo500fx_snd_device);

	if (ret)
		platform_device_put(armadillo500fx_snd_device);

	return ret;
}

static void __exit armadillo500fx_exit(void)
{
	platform_device_unregister(armadillo500fx_snd_device);
	put_ssi_clk(0);
}

module_init(armadillo500fx_init);
module_exit(armadillo500fx_exit);

MODULE_AUTHOR("Atmark Techno, Inc.");
MODULE_DESCRIPTION("WM8978 Driver for Armadillo-500 FX");
MODULE_LICENSE("GPL");
