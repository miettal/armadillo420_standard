/*
 * armadillo440-wm8978.c
 *   --  Armadillo-440 Driver for Wolfson WM8978 Codec
 *
 * Copyright 2009 Atmark Techno, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

/*
 * SSI1(Sync)   | Port1(Async) | Port5(Async) | PAD     | Codec
 * -------------+--------------+--------------+---------+-------
 * STCK         | TXC  ->      | TXC  ->      | TXC  -> | BCLK
 * STFS         | TXFS ->      | TXFS ->      | TXFS -> | LRCIN
 * SYS_CLK(SRCK)| RXC  ->      | RXC  ->      | RXC  -> | MCLK
 * SRFS         | RXFS ->      | RXFS <-      | RXFS <- | LRCOUT
 * STXD         | TXD  ->      | TXD  ->      | TXD  -> | DIN
 * SRXD         | RXD  <-      | RXD  <-      | RXD  <- | DOUT
 *
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
#include <sound/initval.h>

#include <asm/hardware.h>
#include <asm/io.h>

#include "../codecs/wm8978.h"
#include "imx-ssi.h"
#include "imx25-pcm.h"

#define DRV_NAME "armadillo440-wm8978"

//#define DEBUG
#if defined(DEBUG)
#define trace() do {pr_debug(DRV_NAME ": %s@%d\n", __func__, __LINE__);}while(0)
#define debug(...) do {pr_debug(DRV_NAME ": %s@%d: ", __func__, __LINE__); printk(__VA_ARGS__);}while(0)
#else
#define trace() do {}while(0)
#define debug(...) do {}while(0)
#endif

#define error(format, arg...) \
	pr_err(DRV_NAME ": " format "\n" , ## arg)
#define info(format, arg...) \
	pr_info(DRV_NAME ": " format "\n" , ## arg)
#define warning(format, arg...) \
	pr_warning(DRV_NAME ": " format "\n" , ## arg)

/* SSI BCLK and LRC master */
#define CODEC_IS_SSI_MASTER	0
#define ARMADILLO440_WM8978_MCLOCK 24610000

#if defined(CONFIG_ARMADILLO400_AUD5_CON11)
#define AUDMUX_FROM        1
#define AUDMUX_TO          5
#define AUDMUX_FROM_UNUSED 2
#define AUDMUX_TO_UNUSED   6
#elif defined(CONFIG_ARMADILLO400_AUD6_CON9)
#define AUDMUX_FROM        1
#define AUDMUX_TO          6
#define AUDMUX_FROM_UNUSED 2
#define AUDMUX_TO_UNUSED   5
#else
#error AUDIO MUX is not selected
#endif

/*-------------------------------------------------------------------------------*
 * function prototypes                                                           *
 *-------------------------------------------------------------------------------*/
extern void gpio_activate_audio_ports(void);
static int armadillo440_wm8978_dai_init(struct snd_soc_codec *codec);
static int armadillo440_hifi_hw_free(struct snd_pcm_substream *substream);
static int armadillo440_hifi_hw_params(struct snd_pcm_substream *substream,
				       struct snd_pcm_hw_params *params);
static void armadillo440_dump_codec_reg(struct snd_soc_codec *codec);

static void armadillo440_dump_codec_reg(struct snd_soc_codec *codec)
{
	int reg;
	u16 val;

	for (reg=0; reg < WM8978_CACHEREGNUM; reg++) {
		val = codec->read(codec, reg);
		pr_debug("%s: reg 0x%x(%d): 0x%x\n", __func__, reg, reg, val);
	}
}


/*-------------------------------------------------------------------------------*
 * data structures                                                               *
 *-------------------------------------------------------------------------------*/
static struct snd_soc_ops armadillo440_hifi_ops = {
	.hw_params = armadillo440_hifi_hw_params,
	.hw_free = armadillo440_hifi_hw_free,
};

static const struct snd_soc_dapm_widget armadillo440_dapm_widgets[] = {
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

static struct snd_soc_dai_link armadillo440_wm8978_dai_link[] = {
	{
		.name        = "WM8978",
		.stream_name = "WM8978 HiFi",
		.cpu_dai     = &imx_ssi_pcm_dai[0],
		.codec_dai   = &wm8978_dai,
		.init        = armadillo440_wm8978_dai_init,
		.ops         = &armadillo440_hifi_ops,
	},
};

static struct snd_soc_machine armadillo440_wm8978_snd_soc_mach = {
	.name        = "armadillo440",
	.dai_link    = armadillo440_wm8978_dai_link,
	.num_links   = ARRAY_SIZE(armadillo440_wm8978_dai_link),
};

static struct wm8978_setup_data armadillo440_wm8978_setup_data = {
	.i2c_address = 0x1a,
};

static struct snd_soc_device armadillo440_wm8978_snd_soc_dev = {
	.machine     = &armadillo440_wm8978_snd_soc_mach,
	.platform    = &imx_platform,
	.codec_dev   = &soc_codec_dev_wm8978,
	.codec_data  = &armadillo440_wm8978_setup_data,
};

/*-------------------------------------------------------------------------------*
 * hw params                                                                     *
 *-------------------------------------------------------------------------------*/
static int armadillo440_hifi_hw_params(struct snd_pcm_substream *substream,
				       struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_cpu_dai *cpu_dai = rtd->dai->cpu_dai;
	struct snd_soc_codec_dai *codec_dai = rtd->dai->codec_dai;

	unsigned int rate = params_rate(params);
	unsigned int channels = 2;
	u32 dai_format, bclkdiv, mclkdiv, mclksel, sysclk;
	int ssi_clk_div2, ssi_clk_psr, ssi_clk_pm;
	int ret;

	/* MCLOCK = 24610000 */
	switch (rate) {
	case 8000:
		bclkdiv = WM8978_BCLK_DIV_4;
		mclkdiv = WM8978_MCLK_DIV_12;
		mclksel = WM8978_MCLK_MCLK;
		sysclk = 0;

		ssi_clk_div2 = 0;
		ssi_clk_psr  = 0;
		ssi_clk_pm   = 0x17;

		break;

	case 16000:
		bclkdiv = WM8978_BCLK_DIV_4;
		mclkdiv = WM8978_MCLK_DIV_6;
		mclksel = WM8978_MCLK_MCLK;
		sysclk = 0;

		ssi_clk_div2 = 0;
		ssi_clk_psr  = 0;
		ssi_clk_pm   = 0x0b;

		break;
	case 32000:
		bclkdiv = WM8978_BCLK_DIV_4;
		mclkdiv = WM8978_MCLK_DIV_3;
		mclksel = WM8978_MCLK_MCLK;
		sysclk = 0;

		ssi_clk_div2 = 0;
		ssi_clk_psr  = 0;
		ssi_clk_pm   = 0x05;

		break;
	case 48000:
		bclkdiv = WM8978_BCLK_DIV_4;
		mclkdiv = WM8978_MCLK_DIV_2;
		mclksel = WM8978_MCLK_MCLK;
		sysclk = 0;

		ssi_clk_div2 = 0;
		ssi_clk_psr  = 0;
		ssi_clk_pm   = 0x03;

		break;
	default:
		error("unsupported format\n");
		return -EINVAL;
	}

#if CODEC_IS_SSI_MASTER
	dai_format = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
		SND_SOC_DAIFMT_CBM_CFM | SND_SOC_DAIFMT_SYNC;
#else
	dai_format = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
		SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_SYNC;
#endif
	if (channels == 2)
		dai_format |= SND_SOC_DAIFMT_TDM;

	/* set codec DAI configuration */
	ret = codec_dai->dai_ops.set_fmt(codec_dai, dai_format);
	if (ret < 0) {
		error("codec set_fmt failed\n");
		return ret;
	}

	/* set cpu DAI configuration */
	ret = cpu_dai->dai_ops.set_fmt(cpu_dai, dai_format);
	if (ret < 0) {
		error("cpu set_fmt failed\n");
		return ret;
	}

	/* set i.MX active slot mask */
	ret = cpu_dai->dai_ops.set_tdm_slot(cpu_dai,
					    channels == 1 ?
					    0xfffffffe : 0xfffffffc,
					    channels);

	if (ret < 0) {
		error("cpu set_tdm_slot failed\n");
		return ret;
	}

	/* set the SSI system clock as input (unused) */
#if CODEC_IS_SSI_MASTER
	ret = cpu_dai->dai_ops.set_sysclk(cpu_dai, IMX_SSP_SYS_CLK,
					  0, SND_SOC_CLOCK_IN);
#else
	ret = cpu_dai->dai_ops.set_sysclk(cpu_dai, IMX_SSP_SYS_CLK,
				       0, SND_SOC_CLOCK_OUT);
#endif
	if (ret < 0) {
		error("cpu set_sysclk failed\n");
		return ret;
	}

	cpu_dai->dai_ops.set_clkdiv(cpu_dai, IMX_SSI_TX_DIV_2, ssi_clk_div2);
	cpu_dai->dai_ops.set_clkdiv(cpu_dai, IMX_SSI_RX_DIV_2, ssi_clk_div2);

	cpu_dai->dai_ops.set_clkdiv(cpu_dai, IMX_SSI_TX_DIV_PSR, ssi_clk_psr);
	cpu_dai->dai_ops.set_clkdiv(cpu_dai, IMX_SSI_RX_DIV_PSR, ssi_clk_psr);

	cpu_dai->dai_ops.set_clkdiv(cpu_dai, IMX_SSI_TX_DIV_PM, ssi_clk_pm);
	cpu_dai->dai_ops.set_clkdiv(cpu_dai, IMX_SSI_RX_DIV_PM, ssi_clk_pm);

	/* set codec BCLK division for sample rate */
	ret = codec_dai->dai_ops.set_clkdiv(codec_dai, WM8978_BCLKDIV,
					    bclkdiv);
	if (ret < 0) {
		error("codec set_clkdiv bclk failed\n");
		return ret;
	}

	/* set codec MCLK division for 256fs */
	ret = codec_dai->dai_ops.set_clkdiv(codec_dai, WM8978_MCLKDIV,
					    mclkdiv);
	if (ret < 0) {
		error("codec set_clkdiv mclk failed\n");
		return ret;
	}

	/* select the source of the clock */
	ret = codec_dai->dai_ops.set_clkdiv(codec_dai, WM8978_MCLKSEL,
					    mclksel);
	if (ret < 0) {
		error("codec set_clkdiv mclksel failed\n");
		return ret;
	}

	/* codec PLL input is rate from MCLK */
	ret = codec_dai->dai_ops.set_pll(codec_dai, 0,
					 ARMADILLO440_WM8978_MCLOCK, sysclk);
	if (ret < 0) {
		error("codec set_pll failed\n");
		return ret;
	}

	armadillo440_dump_codec_reg(codec_dai->codec);

	return 0;
}

static int armadillo440_hifi_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec_dai *codec_dai = rtd->dai->codec_dai;

	/* disable the PLL */
	codec_dai->dai_ops.set_clkdiv(codec_dai, WM8978_MCLKSEL,
				      WM8978_MCLK_MCLK);
	codec_dai->dai_ops.set_pll(codec_dai, 0, 0, 0);

	return 0;
}

/*-------------------------------------------------------------------------------*
 * DAI                                                                           *
 *-------------------------------------------------------------------------------*/
static void armadillo440_wm8978_dam_init(void)
{
	int p;

#if CODEC_IS_SSI_MASTER
	DAM_PTCR(AUDMUX_FROM) =
		AUDMUX_PTCR_TFSDIR_OUTPUT | AUDMUX_PTCR_TFSSEL_TFS | AUDMUX_PTCR_TFSSEL_PORT(AUDMUX_TO) |
		AUDMUX_PTCR_TCLKDIR_OUTPUT | AUDMUX_PTCR_TCSEL_TC | AUDMUX_PTCR_TCSEL_PORT(AUDMUX_TO) |
		AUDMUX_PTCR_RFSDIR_INPUT | AUDMUX_PTCR_RFSSEL_RFS | AUDMUX_PTCR_RFSSEL_PORT(1) |
		AUDMUX_PTCR_RCLKDIR_INPUT | AUDMUX_PTCR_RCSEL_RC | AUDMUX_PTCR_RCSEL_PORT(1) |
		AUDMUX_PTCR_SYN;
	DAM_PTCR(AUDMUX_TO) =
		AUDMUX_PTCR_TFSDIR_INPUT | AUDMUX_PTCR_TFSSEL_TFS | AUDMUX_PTCR_TFSSEL_PORT(1) |
		AUDMUX_PTCR_TCLKDIR_INPUT | AUDMUX_PTCR_TCSEL_TC | AUDMUX_PTCR_TCSEL_PORT(1) |
		AUDMUX_PTCR_RFSDIR_INPUT | AUDMUX_PTCR_RFSSEL_RFS | AUDMUX_PTCR_RFSSEL_PORT(1) |
		AUDMUX_PTCR_RCLKDIR_INPUT | AUDMUX_PTCR_RCSEL_RC | AUDMUX_PTCR_RCSEL_PORT(1) |
		AUDMUX_PTCR_SYN;
#else
	DAM_PTCR(AUDMUX_FROM) =
		AUDMUX_PTCR_TFSDIR_INPUT | AUDMUX_PTCR_TFSSEL_TFS | AUDMUX_PTCR_TFSSEL_PORT(1) |
		AUDMUX_PTCR_TCLKDIR_INPUT | AUDMUX_PTCR_TCSEL_TC | AUDMUX_PTCR_TCSEL_PORT(1) |
		AUDMUX_PTCR_RFSDIR_INPUT | AUDMUX_PTCR_RFSSEL_RFS | AUDMUX_PTCR_RFSSEL_PORT(1) |
		AUDMUX_PTCR_RCLKDIR_INPUT | AUDMUX_PTCR_RCSEL_RC | AUDMUX_PTCR_RCSEL_PORT(1) |
		AUDMUX_PTCR_ASYN;
	DAM_PTCR(AUDMUX_TO) =
		AUDMUX_PTCR_TFSDIR_OUTPUT | AUDMUX_PTCR_TFSSEL_TFS | AUDMUX_PTCR_TFSSEL_PORT(1) |
		AUDMUX_PTCR_TCLKDIR_OUTPUT | AUDMUX_PTCR_TCSEL_TC | AUDMUX_PTCR_TCSEL_PORT(1) |
		AUDMUX_PTCR_RFSDIR_OUTPUT | AUDMUX_PTCR_RFSSEL_RFS | AUDMUX_PTCR_RFSSEL_PORT(1) |
		AUDMUX_PTCR_RCLKDIR_OUTPUT | AUDMUX_PTCR_RCSEL_RC | AUDMUX_PTCR_RCSEL_PORT(1) |
		AUDMUX_PTCR_ASYN;
#endif /* CODEC_IS_SSI_MASTER */

	DAM_PTCR(AUDMUX_FROM_UNUSED) = 0xAD400800;
	DAM_PTCR(AUDMUX_TO_UNUSED)   = 0x00000800;
	DAM_PTCR(3) = 0x9CC00800;
	DAM_PTCR(4) = 0x00000800;
	DAM_PTCR(7) = 0x00000800;

	DAM_PDCR(AUDMUX_FROM)        = AUDMUX_PDCR_RXDSEL(AUDMUX_TO);
	DAM_PDCR(AUDMUX_FROM_UNUSED) = AUDMUX_PDCR_RXDSEL(AUDMUX_TO_UNUSED);
	DAM_PDCR(AUDMUX_TO_UNUSED)   = AUDMUX_PDCR_RXDSEL(AUDMUX_FROM_UNUSED);
	DAM_PDCR(AUDMUX_TO)          = AUDMUX_PDCR_RXDSEL(AUDMUX_FROM);
	DAM_PDCR(3) = AUDMUX_PDCR_RXDSEL(4);
	DAM_PDCR(4) = AUDMUX_PDCR_RXDSEL(3);
	DAM_PDCR(7) = AUDMUX_PDCR_RXDSEL(7);

	for (p = 1; p <= 7; p++) {
		debug("* DAM @ Port%d. PTCR(%p): 0x%08x, PDCR(%p): 0x%08x\n",
		      p,
		      &DAM_PTCR(p), DAM_PTCR(p),
		      &DAM_PDCR(p), DAM_PDCR(p));
	}
}

static void armadillo440_wm8978_codec_init(struct snd_soc_codec *codec)
{
	/* digital playback volume (100%) */
	codec->write(codec, WM8978_DACVOLL, 0x1ff);
	codec->write(codec, WM8978_DACVOLR, 0x1ff);

	/* digital capture volume (100%) */
	codec->write(codec, WM8978_ADCVOLL, 0x1ff);
	codec->write(codec, WM8978_ADCVOLR, 0x1ff);

	/* capture volume (75%) */
	codec->write(codec, WM8978_INPPGAL, 0x12f); /* 23.25dB: 0x2f */
	codec->write(codec, WM8978_INPPGAR, 0x12f); /* 23.25dB: 0x2f */

	/* headphone volume (50%) */
	codec->write(codec, WM8978_HPVOLL, 0x120); /* 0dB: 0x39 */
	codec->write(codec, WM8978_HPVOLR, 0x120); /* 0dB: 0x39 */

	/* speaker volume (80%) */
	codec->write(codec, WM8978_SPKVOLL, 0x133); /* 0dB: 0x39 */
	codec->write(codec, WM8978_SPKVOLR, 0x133); /* 0dB: 0x39 */

	/* output: enable thermal shutdown */
	codec->write(codec, WM8978_OUTPUT, 0x002); /* + 0x4 for speaker boost */
	/* output: invert ROUT2 for speaker */
	codec->write(codec, WM8978_BEEP, 0x010);
	/* output: OUT3 mute (used as a VMID buffer) */
	codec->write(codec, WM8978_OUT3MIX, 0x040);
	/* output: OUT4 mute (used as a VMID buffer) */
	codec->write(codec, WM8978_MONOMIX, 0x040);

	/* inputs (enable LIP and LIN) */
	codec->write(codec, WM8978_INPUT, 0x003);

	/* GPIO/jack */
	codec->write(codec, WM8978_ADD, 0x001);
	codec->write(codec, WM8978_JACK1, 0x050);
	codec->write(codec, WM8978_JACK2, 0x012);
}

static int armadillo440_wm8978_dai_init(struct snd_soc_codec *codec)
{
	int i;
	int ret;

	armadillo440_wm8978_codec_init(codec);

	/* set up NC codec pins */
	snd_soc_dapm_set_endpoint(codec, "RIP", 0);
	snd_soc_dapm_set_endpoint(codec, "RIN", 0);
	snd_soc_dapm_set_endpoint(codec, "AUXL", 0);
	snd_soc_dapm_set_endpoint(codec, "AUXR", 0);
	snd_soc_dapm_set_endpoint(codec, "OUT3", 0);
	snd_soc_dapm_set_endpoint(codec, "OUT4", 0);

	/* Add armadillo500fx specific widgets */
	for (i = 0; i < ARRAY_SIZE(armadillo440_dapm_widgets); i++) {
		ret = snd_soc_dapm_new_control(codec,
					       &armadillo440_dapm_widgets[i]);
		if (ret < 0) {
			error("widgets %d ret: %d\n", i, ret);
		}
	}

	/* set up armadillo440 audio specific path audio_map */
	for (i = 0; audio_map[i][0] != NULL; i++) {
		ret = snd_soc_dapm_connect_input(codec, audio_map[i][0],
			audio_map[i][1], audio_map[i][2]);
		if (ret < 0)
			return ret;
	}

	snd_soc_dapm_sync_endpoints(codec);

	return 0;
}

/*-------------------------------------------------------------------------------*
 * Device Register                                                               *
 *-------------------------------------------------------------------------------*/
static struct platform_device *soc_audio_dev;

static int __devinit armadillo440_wm8978_probe(struct platform_device *pdev)
{
	int ret;

	soc_audio_dev = platform_device_alloc("soc-audio", 0);
	if (!soc_audio_dev)
		return -ENOMEM;

	platform_set_drvdata(soc_audio_dev,
			     &armadillo440_wm8978_snd_soc_dev);
	armadillo440_wm8978_snd_soc_dev.dev = &soc_audio_dev->dev;

	ret = get_ssi_clk(0, NULL);
	if (ret)
		return ret;

	gpio_activate_audio_ports();

	armadillo440_wm8978_dam_init();

	ret = platform_device_add(soc_audio_dev);
	if (ret)
		platform_device_put(soc_audio_dev);

	return 0;
}

static int __devexit armadillo440_wm8978_remove(struct platform_device *pdev)
{
	platform_device_unregister(soc_audio_dev);

	put_ssi_clk(0);

	return 0;
}

static struct platform_driver armadillo440_wm8978_driver = {
	.driver = {
		.name = "armadillo440-wm8978-audio",
	},
	.probe = armadillo440_wm8978_probe,
	.remove = __devexit_p(armadillo440_wm8978_remove),
};

static int __init armadillo440_wm8978_init(void)
{
	return platform_driver_register(&armadillo440_wm8978_driver);
}
module_init(armadillo440_wm8978_init);

static void __exit armadillo440_wm8978_exit(void)
{
	platform_driver_unregister(&armadillo440_wm8978_driver);
}
module_exit(armadillo440_wm8978_exit);
