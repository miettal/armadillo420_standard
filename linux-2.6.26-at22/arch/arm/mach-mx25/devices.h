#ifndef __ARCH_ARM_MACH_MX25_DEVICES_H__
#define __ARCH_ARM_MACH_MX25_DEVICES_H__

#include <linux/platform_device.h>

extern struct platform_device mx25_i2c1_device;
extern struct platform_device mx25_i2c2_device;
extern struct platform_device mx25_i2c3_device;
extern struct platform_device mx25_spi1_device;
extern struct platform_device mx25_spi2_device;
extern struct platform_device mx25_spi3_device;
extern struct platform_device mx25_fb_device;
extern struct platform_device mx25_adc_device;
extern struct platform_device mx25_adc_ts_device;
extern struct platform_device mx25_sdhc1_device;
extern struct platform_device mx25_sdhc2_device;
extern struct platform_device mx25_fec_device;
extern struct platform_device mx25_pwm1_device;
extern struct platform_device mx25_pwm2_device;
extern struct platform_device mx25_pwm3_device;
extern struct platform_device mx25_pwm4_device;
extern struct platform_device mx25_pwm_backlight_device;
extern struct platform_device mx25_mtd_nor_device;
extern struct platform_device mx25_gpio_led_device;
extern struct platform_device mx25_a2x0_compat_led_device;
extern struct platform_device mx25_gpio_key_device;
extern struct platform_device mx25_a2x0_compat_gpio_device;
extern struct platform_device mx25_gpio_w1_device;
extern struct platform_device mx25_mxc_w1_device;
extern struct platform_device armadillo440_wm8978_audio_device;
extern struct platform_device mx25_flexcan1_device;
extern struct platform_device mx25_flexcan2_device;
extern struct platform_device mx25_keypad_device;

enum mx25_i2c_id {
	MX25_I2C1_ID = 0,
	MX25_I2C2_ID,
	MX25_I2C3_ID,
	MX25_NR_I2C_IDS,
};

#endif
