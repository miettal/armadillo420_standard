/*
 * wm8978.c  --  WM8978 ALSA Soc Audio driver
 *
 * Copyright 2006 Wolfson Microelectronics PLC.
 * Copyright 2008 Atmark Techno, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>

#include "wm8978.h"

#define AUDIO_NAME "wm8978"
#define WM8978_VERSION "0.2"

/*
 * Debug
 */

#define WM8978_DEBUG 0

#ifdef WM8978_DEBUG
#define dbg(format, arg...) \
	printk(KERN_DEBUG AUDIO_NAME ": " format "\n" , ## arg)
#else
#define dbg(format, arg...) do {} while (0)
#endif
#define err(format, arg...) \
	printk(KERN_ERR AUDIO_NAME ": " format "\n" , ## arg)
#define info(format, arg...) \
	printk(KERN_INFO AUDIO_NAME ": " format "\n" , ## arg)
#define warn(format, arg...) \
	printk(KERN_WARNING AUDIO_NAME ": " format "\n" , ## arg)


/*
 * wm8978 register cache
 * We can't read the WM8978 register space when we are
 * using 2 wire for device control, so we cache them instead.
 */
static const u16 wm8978_reg[WM8978_CACHEREGNUM] = {
	0x0000, 0x0000, 0x0000, 0x0000, /*  0 */
	0x0050, 0x0000, 0x0140, 0x0000, /*  4 */
	0x0000, 0x0000, 0x0000, 0x00ff, /*  8 */
	0x00ff, 0x0000, 0x0100, 0x00ff, /* 12 */
	0x00ff, 0x0000, 0x012c, 0x002c, /* 16 */
	0x002c, 0x002c, 0x002c, 0x0000, /* 20 */
	0x0032, 0x0000, 0x0000, 0x0000, /* 24 */
	0x0000, 0x0000, 0x0000, 0x0000, /* 28 */
	0x0038, 0x000b, 0x0032, 0x0000, /* 32 */
	0x0008, 0x000c, 0x0093, 0x00e9, /* 36 */
	0x0000, 0x0000, 0x0000, 0x0000, /* 40 */
	0x0033, 0x0010, 0x0010, 0x0100, /* 44 */
	0x0100, 0x0002, 0x0001, 0x0001, /* 48 */
	0x0039, 0x0039, 0x0039, 0x0039, /* 52 */
	0x0001, 0x0001,                 /* 56 */
};

static int wm8978_check_vol_reg(unsigned int reg)
{
	u16 vol_regs[] = {
		WM8978_DACVOLL, WM8978_DACVOLR,
		WM8978_ADCVOLL, WM8978_ADCVOLR,
		WM8978_INPPGAL, WM8978_INPPGAR,
		WM8978_HPVOLL, WM8978_HPVOLR,
		WM8978_SPKVOLL, WM8978_SPKVOLR,
	};
	int i;

	for (i=0; i<ARRAY_SIZE(vol_regs); i++)
		if ((u16)reg == vol_regs[i])
			return 1;
	return 0;
}

/*
 * read wm8978 register cache
 */
static inline unsigned int wm8978_read_reg_cache(struct snd_soc_codec  *codec,
	unsigned int reg)
{
	u16 *cache = codec->reg_cache;
	if (reg == WM8978_RESET)
		return 0;
	if (reg >= WM8978_CACHEREGNUM)
		return -1;
	return cache[reg];
}

/*
 * write wm8978 register cache
 */
static inline void wm8978_write_reg_cache(struct snd_soc_codec  *codec,
	u16 reg, unsigned int value)
{
	u16 *cache = codec->reg_cache;
	if (reg >= WM8978_CACHEREGNUM)
		return;
	cache[reg] = value;
}

/*
 * write to the WM8978 register space
 */
static int wm8978_write(struct snd_soc_codec  *codec, unsigned int reg,
	unsigned int value)
{
	u8 data[2];

	if (wm8978_check_vol_reg(reg))
		value |= 0x100;

	/* data is
	 *   D15..D9 WM8978 register offset
	 *   D8...D0 register data
	 */
	data[0] = (reg << 1) | ((value >> 8) & 0x0001);
	data[1] = value & 0x00ff;

	if (codec->hw_write(codec->control_data, data, 2) != 2)
		return -EIO;

	wm8978_write_reg_cache (codec, reg, value);

	return 0;
}

#define wm8978_reset(c)	wm8978_write(c, WM8978_RESET, 0)

static const char *wm8978_companding[] = {"Off", "NC", "u-law", "A-law" };
static const char *wm8978_highpass_mode[] =
  {"Audio mode", "Application mode" };
static const char *wm8978_eqmode[] = {"Capture", "Playback" };
static const char *wm8978_bw[] = {"Narrow", "Wide" };
static const char *wm8978_eq1[] = {"80Hz", "105Hz", "135Hz", "175Hz" };
static const char *wm8978_eq2[] = {"230Hz", "300Hz", "385Hz", "500Hz" };
static const char *wm8978_eq3[] = {"650Hz", "850Hz", "1.1kHz", "1.4kHz" };
static const char *wm8978_eq4[] = {"1.8kHz", "2.4kHz", "3.2kHz", "4.1kHz" };
static const char *wm8978_eq5[] = {"5.3kHz", "6.9kHz", "9kHz", "11.7kHz" };
static const char *wm8978_alc_func[] =
  {"ALC off", "ALC right only", "ALC left only", "ALC both on" };
static const char *wm8978_alc_mode[] =
  {"ALC mode", "Limiter mode" };

static const struct soc_enum wm8978_enum[] = {
	SOC_ENUM_SINGLE(WM8978_COMP, 1, 4, wm8978_companding), /* adc */
	SOC_ENUM_SINGLE(WM8978_COMP, 3, 4, wm8978_companding), /* dac */
	SOC_ENUM_SINGLE(WM8978_ADC,  7, 2, wm8978_highpass_mode),
	SOC_ENUM_SINGLE(WM8978_EQ1,  8, 2, wm8978_eqmode),

	SOC_ENUM_SINGLE(WM8978_EQ1,  5, 4, wm8978_eq1),
	SOC_ENUM_SINGLE(WM8978_EQ2,  8, 2, wm8978_bw),
	SOC_ENUM_SINGLE(WM8978_EQ2,  5, 4, wm8978_eq2),
	SOC_ENUM_SINGLE(WM8978_EQ3,  8, 2, wm8978_bw),

	SOC_ENUM_SINGLE(WM8978_EQ3,  5, 4, wm8978_eq3),
	SOC_ENUM_SINGLE(WM8978_EQ4,  8, 2, wm8978_bw),
	SOC_ENUM_SINGLE(WM8978_EQ4,  5, 4, wm8978_eq4),
	SOC_ENUM_SINGLE(WM8978_EQ5,  8, 2, wm8978_bw),

	SOC_ENUM_SINGLE(WM8978_EQ5,  5, 4, wm8978_eq5),
	SOC_ENUM_SINGLE(WM8978_ALC1, 7, 4, wm8978_alc_func),
	SOC_ENUM_SINGLE(WM8978_ALC3, 8, 2, wm8978_alc_mode),
};

static const struct snd_kcontrol_new wm8978_snd_controls[] = {
SOC_SINGLE("Digital Loopback Switch", WM8978_COMP, 0, 1, 0),

SOC_ENUM("ADC Companding", wm8978_enum[0]),
SOC_ENUM("DAC Companding", wm8978_enum[1]),
SOC_SINGLE("Companding Control 8-bit Mode Switch", WM8978_COMP, 5, 1, 0),

SOC_SINGLE("Jack Detection Enable", WM8978_JACK1, 6, 1, 0),

SOC_DOUBLE("DAC Inversion Switch", WM8978_DAC, 0, 1, 1, 0),

SOC_SINGLE("DAC Automute", WM8978_DAC, 2, 1, 0),

SOC_DOUBLE_R("Playback Gain", WM8978_DACVOLL, WM8978_DACVOLR, 0, 255, 0),

SOC_SINGLE("High Pass Filter Switch", WM8978_ADC, 8, 1, 0),
SOC_ENUM("High Pass Filter Mode", wm8978_enum[2]),
SOC_SINGLE("High Pass Cut Off", WM8978_ADC, 4, 7, 0),

SOC_DOUBLE("ADC Inversion Switch", WM8978_ADC, 0, 1, 1, 0),

SOC_DOUBLE_R("Capture Gain", WM8978_ADCVOLL, WM8978_ADCVOLR, 0, 255, 0),

SOC_ENUM("Equaliser Function", wm8978_enum[3]),
SOC_ENUM("EQ1 Cut Off", wm8978_enum[4]),
SOC_SINGLE("EQ1 Volume", WM8978_EQ1,  0, 31, 1),

SOC_ENUM("Equaliser EQ2 Bandwith", wm8978_enum[5]),
SOC_ENUM("EQ2 Cut Off", wm8978_enum[6]),
SOC_SINGLE("EQ2 Volume", WM8978_EQ2,  0, 31, 1),

SOC_ENUM("Equaliser EQ3 Bandwith", wm8978_enum[7]),
SOC_ENUM("EQ3 Cut Off", wm8978_enum[8]),
SOC_SINGLE("EQ3 Volume", WM8978_EQ3,  0, 31, 1),

SOC_ENUM("Equaliser EQ4 Bandwith", wm8978_enum[9]),
SOC_ENUM("EQ4 Cut Off", wm8978_enum[10]),
SOC_SINGLE("EQ4 Volume", WM8978_EQ4,  0, 31, 1),

SOC_ENUM("Equaliser EQ5 Bandwith", wm8978_enum[11]),
SOC_ENUM("EQ5 Cut Off", wm8978_enum[12]),
SOC_SINGLE("EQ5 Volume", WM8978_EQ5,  0, 31, 1),

SOC_SINGLE("DAC Playback Limiter Switch", WM8978_DACLIM1,  8, 1, 0),
SOC_SINGLE("DAC Playback Limiter Decay", WM8978_DACLIM1,  4, 15, 0),
SOC_SINGLE("DAC Playback Limiter Attack", WM8978_DACLIM1,  0, 15, 0),

SOC_SINGLE("DAC Playback Limiter Threshold", WM8978_DACLIM2,  4, 7, 0),
SOC_SINGLE("DAC Playback Limiter Boost", WM8978_DACLIM2,  0, 15, 0),

SOC_SINGLE("ALC Enable Switch", WM8978_ALC1,  8, 1, 0),
SOC_ENUM("ALC Capture Function", wm8978_enum[13]),
SOC_SINGLE("ALC Capture Max Gain", WM8978_ALC1,  3, 7, 0),
SOC_SINGLE("ALC Capture Min Gain", WM8978_ALC1,  0, 7, 0),

SOC_SINGLE("ALC Capture ZC Switch", WM8978_ALC2,  8, 1, 0),
SOC_SINGLE("ALC Capture Hold", WM8978_ALC2,  4, 15, 0),
SOC_SINGLE("ALC Capture Target", WM8978_ALC2,  0, 15, 0),

SOC_ENUM("ALC Capture Mode", wm8978_enum[14]),
SOC_SINGLE("ALC Capture Decay", WM8978_ALC3,  4, 15, 0),
SOC_SINGLE("ALC Capture Attack", WM8978_ALC3,  0, 15, 0),

SOC_SINGLE("ALC Capture Noise Gate Switch", WM8978_NGATE,  3, 1, 0),
SOC_SINGLE("ALC Capture Noise Gate Threshold", WM8978_NGATE,  0, 7, 0),

SOC_SINGLE("Stereo depth", WM8978_3D,  0, 15, 0),

SOC_DOUBLE_R("Mic ZC Switch", WM8978_INPPGAL, WM8978_INPPGAR, 7, 1, 0),
SOC_DOUBLE_R("Mic Mute Switch", WM8978_INPPGAL, WM8978_INPPGAR, 6, 1, 1),
SOC_DOUBLE_R("Mic Playback Volume", WM8978_INPPGAL, WM8978_INPPGAR, 0, 63, 0),

SOC_DOUBLE_R("Headphone ZC Switch", WM8978_HPVOLL, WM8978_HPVOLR,
	     7, 1, 0),
SOC_DOUBLE_R("Headphone Mute Switch", WM8978_HPVOLL, WM8978_HPVOLR,
	     6, 1, 1),
SOC_DOUBLE_R("Headphone Playback Volume", WM8978_HPVOLL, WM8978_HPVOLR,
	     0, 63, 0),

SOC_DOUBLE_R("PC Speaker ZC Switch", WM8978_SPKVOLL, WM8978_SPKVOLR,
	     7, 1, 0),
SOC_DOUBLE_R("PC Speaker Mute Switch", WM8978_SPKVOLL, WM8978_SPKVOLR,
	     6, 1, 1),
SOC_DOUBLE_R("PC Speaker Playback Volume", WM8978_SPKVOLL, WM8978_SPKVOLR,
	     0, 63, 0),

SOC_DOUBLE_R("Mic Boost(+20dB) Switch", WM8978_ADCBOOSTL, WM8978_ADCBOOSTR,
	     8, 1, 0),
};

/* add non dapm controls */
static int wm8978_add_controls(struct snd_soc_codec *codec)
{
	int err, i;

	for (i = 0; i < ARRAY_SIZE(wm8978_snd_controls); i++) {
		err = snd_ctl_add(codec->card,
				snd_soc_cnew(&wm8978_snd_controls[i],
					codec, NULL));
		if (err < 0)
			return err;
	}

	return 0;
}

/* Left Output Mixer */
static const struct snd_kcontrol_new wm8978_left_mixer_controls[] = {
SOC_DAPM_SINGLE("Left Auxilliary Switch", WM8978_MIXL, 5, 1, 0),
SOC_DAPM_SINGLE("Left Input Bypass Switch", WM8978_MIXL, 1, 1, 0),
SOC_DAPM_SINGLE("Left DAC Output Switch", WM8978_MIXL, 0, 1, 0),
SOC_DAPM_SINGLE("Right DAC Output Switch", WM8978_OUTPUT, 5, 1, 0),
};

/* Right Output Mixer */
static const struct snd_kcontrol_new wm8978_right_mixer_controls[] = {
SOC_DAPM_SINGLE("Right Auxilliary Switch", WM8978_MIXR, 5, 1, 0),
SOC_DAPM_SINGLE("Right Input Bypass Switch", WM8978_MIXR, 1, 1, 0),
SOC_DAPM_SINGLE("Left DAC Output Switch", WM8978_OUTPUT, 6, 1, 0),
SOC_DAPM_SINGLE("Right DAC Output Switch", WM8978_MIXR, 0, 1, 0),
};

/* Left Input boost vol */
static const struct snd_kcontrol_new wm8978_left_boost_controls[] = {
SOC_DAPM_SINGLE("AUXL Volume", WM8978_ADCBOOSTL, 0, 7, 0),
SOC_DAPM_SINGLE("L2 Volume", WM8978_ADCBOOSTL, 4, 7, 0),
SOC_DAPM_SINGLE("Left Capture Mute Switch", WM8978_INPPGAL, 6, 1, 1),
};

/* Right Input boost vol */
static const struct snd_kcontrol_new wm8978_right_boost_controls[] = {
SOC_DAPM_SINGLE("AUXR Volume", WM8978_ADCBOOSTR, 0, 7, 0),
SOC_DAPM_SINGLE("R2 Volume", WM8978_ADCBOOSTR, 4, 7, 0),
SOC_DAPM_SINGLE("Right Capture Mute Switch", WM8978_INPPGAR, 6, 1, 1),
};

/* out3 mixer */
static const struct snd_kcontrol_new wm8978_out3_mixer_controls[] = {
SOC_DAPM_SINGLE("Left DAC Output Switch", WM8978_OUT3MIX, 0, 1, 0),
SOC_DAPM_SINGLE("Left Mixer Switch", WM8978_OUT3MIX, 1, 1, 0),
SOC_DAPM_SINGLE("Left ADC Bypass Switch", WM8978_OUT3MIX, 2, 1, 0),
SOC_DAPM_SINGLE("OUT4 Mixer Switch", WM8978_OUT3MIX, 3, 1, 0),
};

/* out4 mixer */
static const struct snd_kcontrol_new wm8978_out4_mixer_controls[] = {
SOC_DAPM_SINGLE("Right DAC Output Switch", WM8978_MONOMIX, 0, 1, 0),
SOC_DAPM_SINGLE("Left DAC Output Switch", WM8978_MONOMIX, 3, 1, 0),
SOC_DAPM_SINGLE("Right Mixer Switch", WM8978_MONOMIX, 1, 1, 0),
SOC_DAPM_SINGLE("Left Mixer Switch", WM8978_MONOMIX, 4, 1, 0),
SOC_DAPM_SINGLE("Right ADC Bypass Switch", WM8978_MONOMIX, 2, 1, 0),
};

/* Widgets */
static const struct snd_soc_dapm_widget wm8978_dapm_widgets[] = {
SND_SOC_DAPM_MIXER("Left Boost Mixer", WM8978_POWER2, 4, 0,
		   wm8978_left_boost_controls,
		   ARRAY_SIZE(wm8978_left_boost_controls)),
SND_SOC_DAPM_MIXER("Right Boost Mixer", WM8978_POWER2, 5, 0,
		   wm8978_right_boost_controls,
		   ARRAY_SIZE(wm8978_right_boost_controls)),
SND_SOC_DAPM_MIXER("OUT3 Mixer", WM8978_POWER1, 6, 0,
		   wm8978_out3_mixer_controls,
		   ARRAY_SIZE(wm8978_out3_mixer_controls)),
SND_SOC_DAPM_MIXER("OUT4 Mixer", WM8978_POWER1, 7, 0,
		   wm8978_out4_mixer_controls,
		   ARRAY_SIZE(wm8978_out4_mixer_controls)),

SND_SOC_DAPM_MICBIAS("Mic Bias", WM8978_POWER1, 4, 0),

SND_SOC_DAPM_INPUT("LIN"),
SND_SOC_DAPM_INPUT("LIP"),
SND_SOC_DAPM_INPUT("RIN"),
SND_SOC_DAPM_INPUT("RIP"),
SND_SOC_DAPM_INPUT("L2"),
SND_SOC_DAPM_INPUT("R2"),
SND_SOC_DAPM_INPUT("AUXL"),
SND_SOC_DAPM_INPUT("AUXR"),
SND_SOC_DAPM_OUTPUT("LOUT1"),
SND_SOC_DAPM_OUTPUT("ROUT1"),
SND_SOC_DAPM_OUTPUT("LOUT2"),
SND_SOC_DAPM_OUTPUT("ROUT2"),
SND_SOC_DAPM_OUTPUT("OUT3"),
SND_SOC_DAPM_OUTPUT("OUT4"),
};

static const char *audio_map[][3] = {
	/* left capture PGA */
	{"Left Capture PGA", NULL, "LIN"},
	{"Left Capture PGA", NULL, "LIP"},
	{"Left Capture PGA", NULL, "L2"},

	/* right capture PGA */
	{"Right Capture PGA", NULL, "RIN"},
	{"Right Capture PGA", NULL, "RIP"},
	{"Right Capture PGA", NULL, "R2"},

	/* left boost mixer */
	{"Left Boost Mixer", "AUXL Volume", "AUXL"},
	{"Left Boost Mixer", "Left Capture Mute Switch", "Left Capture PGA"},
	{"Left Boost Mixer", "L2 Volume", "L2"},

	/* right boost mixer */
	{"Right Boost Mixer", "AUXR Volume", "AUXR"},
	{"Right Boost Mixer", "Right Capture Mute Switch", "Right Capture PGA"},
	{"Right Boost Mixer", "R2 Volume", "R2"},

	/* ADC */
	{"Left ADC", NULL, "Left Boost Mixer"},
	{"Right ADC", NULL, "Right Boost Mixer"},

	/* left output mixer */
	{"Left Output Mixer", "Left Auxilliary Switch", "AUXL"},	
	{"Left Output Mixer", "Left Input Bypass Switch", "Left Boost Mixer"},
	{"Left Output Mixer", "Left DAC Output Switch", "Left DAC"},
	{"Left Output Mixer", "Right DAC Output Switch", "Right DAC"},

	/* right output mixer */
	{"Right Output Mixer", "Right Auxilliary Switch", "AUXR"},	
	{"Right Output Mixer", "Right Input Bypass Switch", "Right Boost Mixer"},
	{"Right Output Mixer", "Left DAC Output Switch", "Left DAC"},
	{"Right Output Mixer", "Right DAC Output Switch", "Right DAC"},

	/* OUT3 mixer */
	{"OUT3 Mixer", "Left DAC Output Switch", "Left DAC"},
	{"OUT3 Mixer", "Left Mixer Switch", "Left Output Mixer"},
	{"OUT3 Mixer", "Left ADC Bypass Switch", "Left ADC"},
	{"OUT3 Mixer", "OUT4 Mixer Switch", "OUT4 Mixer"},

	/* OUT4 mixer */
	{"OUT4 Mixer", "Right DAC Output Switch", "Right DAC"},
	{"OUT4 Mixer", "Right Mixer Switch", "Right Output Mixer"},
	{"OUT4 Mixer", "Right ADC Bypass Switch", "Right ADC"},
	{"OUT4 Mixer", "Left DAC Output Switch", "Left DAC"},
	{"OUT4 Mixer", "Left Mixer Switch", "Left Output Mixer"},

	/* outputs */
	{"LOUT1", NULL, "Left Output Mixer"},
	{"ROUT1", NULL, "Right Output Mixer"},
	{"LOUT2", NULL, "Left Output Mixer"},
	{"ROUT2", NULL, "Right Output Mixer"},
	{"OUT3", NULL, "OUT3 Mixer"},
	{"OUT4", NULL, "OUT4 Mixer"},

	/* terminator */
	{NULL, NULL, NULL},
};

static int wm8978_add_widgets(struct snd_soc_codec *codec)
{
	int i, ret;

	for(i = 0; i < ARRAY_SIZE(wm8978_dapm_widgets); i++) {
		ret = snd_soc_dapm_new_control(codec, &wm8978_dapm_widgets[i]);
		if (ret < 0)
			return ret;
	}

	/* set up audio path map */
	for(i = 0; audio_map[i][0] != NULL; i++) {
		ret = snd_soc_dapm_connect_input(codec, audio_map[i][0], 
						 audio_map[i][1],
						 audio_map[i][2]);
		if (ret < 0)
			return ret;
	}

	snd_soc_dapm_new_widgets(codec);
	return 0;
}

struct _pll_div {
	unsigned int pre:4; /* prescale - 1 */
	unsigned int n:4;
	unsigned int k;
};

static struct _pll_div pll_div;

/* The size in bits of the pll divide multiplied by 10
 * to allow rounding later */
#define FIXED_PLL_SIZE ((1 << 24) * 10)

static void pll_factors(unsigned int target, unsigned int source)
{
	unsigned long long Kpart;
	unsigned int K, Ndiv, Nmod;

	Ndiv = target / source;
	if (Ndiv < 6) {
		source >>= 1;
		pll_div.pre = 1;
		Ndiv = target / source;
	} else
		pll_div.pre = 0;

	if ((Ndiv < 6) || (Ndiv > 12))
		printk(KERN_WARNING
			"WM8978 N value outwith recommended range! N = %d\n",Ndiv);

	pll_div.n = Ndiv;
	Nmod = target % source;
	Kpart = FIXED_PLL_SIZE * (long long)Nmod;

	do_div(Kpart, source);

	K = Kpart & 0xFFFFFFFF;

	/* Check if we need to round */
	if ((K % 10) >= 5)
		K += 5;

	/* Move down to proper range now rounding is done */
	K /= 10;

	pll_div.k = K;
}

static int wm8978_set_pll(struct snd_soc_codec_dai *codec_dai,
			  int pll_id, unsigned int freq_in, unsigned int freq_out)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	u16 reg;

	if (freq_in == 0 || freq_out == 0) {
		reg = wm8978_read_reg_cache(codec, WM8978_POWER1);
		wm8978_write(codec, WM8978_POWER1, reg & 0x1df);
		return 0;
	}

	pll_factors(freq_out * 8, freq_in);

	wm8978_write(codec, WM8978_PLLN, (pll_div.pre << 4) | pll_div.n);
	wm8978_write(codec, WM8978_PLLK1, pll_div.k >> 18);
	wm8978_write(codec, WM8978_PLLK2, (pll_div.k >> 9) & 0x1ff);
	wm8978_write(codec, WM8978_PLLK3, pll_div.k & 0x1ff);
	reg = wm8978_read_reg_cache(codec, WM8978_POWER1);
	wm8978_write(codec, WM8978_POWER1, reg | 0x020);
	return 0;
}

static int wm8978_set_fmt(struct snd_soc_codec_dai *codec_dai,
			  unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	u16 iface = wm8978_read_reg_cache(codec, WM8978_IFACE) & 0xfe61;
	u16 clk = wm8978_read_reg_cache(codec, WM8978_CLOCK) & 0xfffe;

	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		clk |= 0x0001;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	default:
		return -EINVAL;
	}

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		iface |= 0x0010;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		iface |= 0x0008;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		iface |= 0x0018;
		break;
	default:
		return -EINVAL;
	}

	/* clock inversion */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_IB_IF:
		iface |= 0x0180;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		iface |= 0x0100;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		iface |= 0x0080;
		break;
	default:
		return -EINVAL;
	}

	wm8978_write(codec, WM8978_IFACE, iface);
	wm8978_write(codec, WM8978_CLOCK, clk);

	return 0;
}

static int wm8978_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->codec;
	u16 iface = wm8978_read_reg_cache(codec, WM8978_IFACE) & 0xff9e;
	u16 adn = wm8978_read_reg_cache(codec, WM8978_ADD) & 0xfff1;

	/* bit size */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		iface |= 0x0020;
		break;
	case SNDRV_PCM_FORMAT_S24_3LE:
	case SNDRV_PCM_FORMAT_S24_LE:
		iface |= 0x0040;
		break;
	default:
		return -EINVAL;
	}

	/* filter coefficient */
	switch (params_rate(params)) {
	case 8000:
		adn |= 0x5 << 1;
		break;
	case 11025:
	case 12000:
		adn |= 0x4 << 1;
		break;
	case 16000:
		adn |= 0x3 << 1;
		break;
	case 22050:
	case 24000:
		adn |= 0x2 << 1;
		break;
	case 32000:
		adn |= 0x1 << 1;
		break;
	case 44100:
	case 48000:
		adn |= 0x0 << 1;
		break;
	default:
		return -EINVAL;
	}

	/* set iface */
	wm8978_write(codec, WM8978_IFACE, iface);
	wm8978_write(codec, WM8978_ADD, adn);
	return 0;
}

static int wm8978_prepare(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->codec;
	u16 output = wm8978_read_reg_cache(codec, WM8978_OUTPUT) & 0xffbf;
	if (runtime->channels == 1)
		output |= 0x40;
	wm8978_write(codec, WM8978_OUTPUT, output);

	return 0;
}

static int wm8978_set_clkdiv(struct snd_soc_codec_dai *codec_dai,
			     int div_id, int div)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	u16 reg;

	switch (div_id) {
	case WM8978_MCLKDIV:
		reg = wm8978_read_reg_cache(codec, WM8978_CLOCK) & 0x11f;
		wm8978_write(codec, WM8978_CLOCK, reg | div);
		break;
	case WM8978_BCLKDIV:
		reg = wm8978_read_reg_cache(codec, WM8978_CLOCK) & 0x1e3;
		wm8978_write(codec, WM8978_CLOCK, reg | div);
		break;
	case WM8978_OPCLKDIV:
		reg = wm8978_read_reg_cache(codec, WM8978_GPIO) & 0x1cf;
		wm8978_write(codec, WM8978_GPIO, reg | div);
		break;
	case WM8978_DACOSR:
		reg = wm8978_read_reg_cache(codec, WM8978_DAC) & 0x1f7;
		wm8978_write(codec, WM8978_DAC, reg | div);
		break;
	case WM8978_ADCOSR:
		reg = wm8978_read_reg_cache(codec, WM8978_ADC) & 0x1f7;
		wm8978_write(codec, WM8978_ADC, reg | div);
		break;
	case WM8978_MCLKSEL:
		reg = wm8978_read_reg_cache(codec, WM8978_CLOCK) & 0x0ff;
		wm8978_write(codec, WM8978_CLOCK, reg | div);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int wm8978_digital_mute(struct snd_soc_codec_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;
	u16 mute_reg = wm8978_read_reg_cache(codec, WM8978_DAC) & 0xffbf;

	if(mute)
		wm8978_write(codec, WM8978_DAC, mute_reg | 0x40);
	else
		wm8978_write(codec, WM8978_DAC, mute_reg);

	return 0;
}

/* TODO: liam need to make this lower power with dapm */
static int wm8978_dapm_event(struct snd_soc_codec *codec, int event)
{
	static u16 pwr_reg1 = -1;
	static u16 pwr_reg2 = -1;
	static u16 pwr_reg3 = -1;

	if (pwr_reg1 == (u16)-1)
		pwr_reg1 = wm8978_read_reg_cache(codec, WM8978_POWER1);
	if (pwr_reg2 == (u16)-1)
		pwr_reg2 = wm8978_read_reg_cache(codec, WM8978_POWER2);
	if (pwr_reg3 == (u16)-1)
		pwr_reg3 = wm8978_read_reg_cache(codec, WM8978_POWER3);

	switch (event) {
	case SNDRV_CTL_POWER_D0: /* full On */
		pwr_reg1 = wm8978_read_reg_cache(codec, WM8978_POWER1) & 0x1fc;
		/* set vmid to 75k */
		wm8978_write(codec, WM8978_POWER1, pwr_reg1 | 0x001);
		break;
	case SNDRV_CTL_POWER_D1: /* partial On */
	case SNDRV_CTL_POWER_D2: /* partial On */
		pwr_reg1 = wm8978_read_reg_cache(codec, WM8978_POWER1) & 0x1fc;
		/* set vmid to 5k for quick power up */
		wm8978_write(codec, WM8978_POWER1, pwr_reg1 | 0x003);
		break;
	case SNDRV_CTL_POWER_D3hot: /* Off, with power */
		/* restore power regs */
		pwr_reg1 &= 0x1fc;
		/* vmid to 300k */
		wm8978_write(codec, WM8978_POWER1, pwr_reg1 | 0x002);
		wm8978_write(codec, WM8978_POWER2, pwr_reg2);
		wm8978_write(codec, WM8978_POWER3, pwr_reg3);
		break;
	case SNDRV_CTL_POWER_D3cold: /* Off, without power */
		/* save power regs */
		pwr_reg1 = wm8978_read_reg_cache(codec, WM8978_POWER1);
		pwr_reg2 = wm8978_read_reg_cache(codec, WM8978_POWER2);
		pwr_reg3 = wm8978_read_reg_cache(codec, WM8978_POWER3);
		/* everything off, dac mute, inactive */
		wm8978_write(codec, WM8978_POWER1, 0x0);
		wm8978_write(codec, WM8978_POWER2, 0x0);
		wm8978_write(codec, WM8978_POWER3, 0x0);
		break;
	}
	codec->dapm_state = event;
	return 0;
}

#define WM8978_RATES \
	(SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | \
	SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | \
	SNDRV_PCM_RATE_48000)

#define WM8978_FORMATS \
	(SNDRV_PCM_FORMAT_S16_LE | SNDRV_PCM_FORMAT_S20_3LE | \
	SNDRV_PCM_FORMAT_S24_3LE | SNDRV_PCM_FORMAT_S24_LE)

struct snd_soc_codec_dai wm8978_dai = {
	.name = "WM8978 HiFi",
	.playback = {
		.stream_name = "HiFi Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = WM8978_RATES,
		.formats = WM8978_FORMATS,},
	.capture = {
		.stream_name = "HiFi Capture",
		.channels_min = 1,
		.channels_max = 1,
		.rates = WM8978_RATES,
		.formats = WM8978_FORMATS,},
	.ops = {
		.hw_params = wm8978_hw_params,
		.prepare = wm8978_prepare,
	},
	.dai_ops = {
		.digital_mute = wm8978_digital_mute,
		.set_fmt = wm8978_set_fmt,
		.set_clkdiv = wm8978_set_clkdiv,
		.set_pll = wm8978_set_pll,
	},
};
EXPORT_SYMBOL_GPL(wm8978_dai);

static int wm8978_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;

	if (codec->name)
		wm8978_dapm_event(codec, SNDRV_CTL_POWER_D3cold);

	return 0;
}

static int wm8978_resume(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;
	int i;
	u8 data[2];
	u16 *cache;

	if (!codec->name)
		return 0;

	cache = codec->reg_cache;

	/* Sync reg_cache with the hardware */
	for (i = 0; i < ARRAY_SIZE(wm8978_reg); i++) {
		data[0] = (i << 1) | ((cache[i] >> 8) & 0x0001);
		data[1] = cache[i] & 0x00ff;
		codec->hw_write(codec->control_data, data, 2);
	}
	wm8978_dapm_event(codec, SNDRV_CTL_POWER_D3hot);
	wm8978_dapm_event(codec, codec->suspend_dapm_state);
	return 0;
}

/*
 * initialise the WM8978 driver
 * register the mixer and dsp interfaces with the kernel
 */
static int wm8978_init(struct snd_soc_device* socdev)
{
	struct snd_soc_codec *codec = socdev->codec;
	int ret = 0;

	codec->name = "WM8978";
	codec->owner = THIS_MODULE;
	codec->read = wm8978_read_reg_cache;
	codec->write = wm8978_write;
	codec->dapm_event = wm8978_dapm_event;
	codec->dai = &wm8978_dai;
	codec->num_dai = 1;
	codec->reg_cache_size = ARRAY_SIZE(wm8978_reg);
	codec->reg_cache = kmemdup(wm8978_reg, sizeof(wm8978_reg), GFP_KERNEL);
	if (codec->reg_cache == NULL)
		return -ENOMEM;

	wm8978_reset(codec);

	/* register pcms */
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if(ret < 0) {
		printk(KERN_ERR "wm8978: failed to create pcms\n");
		goto pcm_err;
	}

	/* power on device */
	wm8978_write(codec, WM8978_POWER1, 0x11c);
	wm8978_write(codec, WM8978_POWER2, 0x18f);
	wm8978_write(codec, WM8978_POWER3, 0x1ef);

	wm8978_dapm_event(codec, SNDRV_CTL_POWER_D3hot);

	wm8978_add_controls(codec);
	wm8978_add_widgets(codec);
	ret = snd_soc_register_card(socdev);
	if (ret < 0) {
		printk(KERN_ERR "wm8978: failed to register card\n");
		goto card_err;
	}
	return ret;

card_err:
	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);
pcm_err:
	kfree(codec->reg_cache);
	return ret;
}

static struct snd_soc_device *wm8978_socdev;

#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)

static unsigned short normal_i2c[] = { 0, I2C_CLIENT_END };

/* Magic definition of all other variables and things */
I2C_CLIENT_INSMOD;

static struct i2c_driver wm8978_i2c_driver;
static struct i2c_client client_template;

static int wm8978_codec_probe(struct i2c_adapter *adap, int addr, int kind)
{
	struct snd_soc_device *socdev = wm8978_socdev;
	struct wm8978_setup_data *setup = socdev->codec_data;
	struct snd_soc_codec *codec = socdev->codec;
	struct i2c_client *i2c;
	int ret;

	if (addr != setup->i2c_address)
		return -ENODEV;

	client_template.adapter = adap;
	client_template.addr = addr;

	i2c = kmemdup(&client_template, sizeof(client_template), GFP_KERNEL);
	if (i2c == NULL){
		kfree(codec);
		return -ENOMEM;
	}

	i2c_set_clientdata(i2c, codec);

	codec->control_data = i2c;

	ret = i2c_attach_client(i2c);
	if(ret < 0) {
		err("failed to attach codec at addr %x\n", addr);
		goto err;
	}

	ret = wm8978_init(socdev);
	if(ret < 0) {
		err("failed to initialise WM8978\n");
		goto err;
	}
	return ret;

err:
	kfree(codec);
	kfree(i2c);
	return ret;

}

static int wm8978_i2c_detach(struct i2c_client *client)
{
	struct snd_soc_codec *codec = i2c_get_clientdata(client);
	i2c_detach_client(client);
	kfree(codec->reg_cache);
	kfree(client);
	return 0;
}

static int wm8978_i2c_attach(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &addr_data, wm8978_codec_probe);
}

/* i2c codec control layer */
static struct i2c_driver wm8978_i2c_driver = {
	.driver = {
		.name = "WM8978 I2C Codec",
		.owner = THIS_MODULE,
	},
	.id =             I2C_DRIVERID_WM8978,
	.attach_adapter = wm8978_i2c_attach,
	.detach_client =  wm8978_i2c_detach,
	.command =        NULL,
};

static struct i2c_client client_template = {
	.name =   "WM8978",
	.driver = &wm8978_i2c_driver,
};
#endif

static int wm8978_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct wm8978_setup_data *setup;
	struct snd_soc_codec *codec;
	int ret = 0;

	info("WM8978 Audio Codec v%s", WM8978_VERSION);

	setup = socdev->codec_data;
	codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (codec == NULL)
		return -ENOMEM;

	socdev->codec = codec;
	mutex_init(&codec->mutex);
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	wm8978_socdev = socdev;
#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)
	if (setup->i2c_address) {
		normal_i2c[0] = setup->i2c_address;
		codec->hw_write = (hw_write_t)i2c_master_send;
		ret = i2c_add_driver(&wm8978_i2c_driver);
		if (ret != 0)
			printk(KERN_ERR "can't add i2c driver");
	}
#else
	/* Add other interfaces here */
#endif
	return ret;
}

/* power down chip */
static int wm8978_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;

	if (codec->control_data)
		wm8978_dapm_event(codec, SNDRV_CTL_POWER_D3cold);

	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);
#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)
	i2c_del_driver(&wm8978_i2c_driver);
#endif
	kfree(codec);

	return 0;
}

struct snd_soc_codec_device soc_codec_dev_wm8978 = {
	.probe = 	wm8978_probe,
	.remove = 	wm8978_remove,
	.suspend = 	wm8978_suspend,
	.resume =	wm8978_resume,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_wm8978);

MODULE_DESCRIPTION("ASoC WM8978 driver");
MODULE_AUTHOR("Atmark Techno, Inc.");
MODULE_LICENSE("GPL");
