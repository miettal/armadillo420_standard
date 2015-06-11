/*
 * i.MX25 generic GPIO handling
 *
 * Copyright (c) 2009 Atmark Techno, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/gpio.h>

struct mxc_gpio_chip {
	struct gpio_chip chip;
	int port_num;
};

#define to_mxc_gpio_chip(c) container_of(c, struct mxc_gpio_chip, chip)

#define GPIO_TO_IOMUX(port, num) (((port) << MUX_IO_P) | ((num) << MUX_IO_I))

#if defined(DEBUG)
#define dprintf(...) do{pr_dbg(__func__": "__VA_ARGS__);}while(0)
#else
#define dpeintf(...) do{}while(0)
#endif

static int mx25_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	struct mxc_gpio_chip *mxc_gpio = to_mxc_gpio_chip(chip);
	u32 pin = GPIO_TO_IOMUX(mxc_gpio->port_num, offset);

	mxc_set_gpio_direction(pin, 1);

	return 0;
}

static int mx25_gpio_direction_output(struct gpio_chip *chip,
				      unsigned offset, int val)
{
	struct mxc_gpio_chip *mxc_gpio = to_mxc_gpio_chip(chip);
	u32 pin = GPIO_TO_IOMUX(mxc_gpio->port_num, offset);

	mxc_set_gpio_dataout(pin, val);
	mxc_set_gpio_direction(pin, 0);

	return 0;
}

static int mx25_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct mxc_gpio_chip *mxc_gpio = to_mxc_gpio_chip(chip);
	u32 pin = GPIO_TO_IOMUX(mxc_gpio->port_num, offset);

	return mxc_get_gpio_datain(pin);
}

static void mx25_gpio_set(struct gpio_chip *chip, unsigned offset, int val)
{
	struct mxc_gpio_chip *mxc_gpio = to_mxc_gpio_chip(chip);
	u32 pin = GPIO_TO_IOMUX(mxc_gpio->port_num, offset);

	mxc_set_gpio_dataout(pin, val);
}

static void mx25_gpio_dbg_show(struct seq_file *s, struct gpio_chip *chip)
{
	struct mxc_gpio_chip *mxc_gpio = to_mxc_gpio_chip(chip);
	u32 pin;
	int val, dir;
	int i;

	for (i = 0; i < chip->ngpio; i++) {
		pin = GPIO_TO_IOMUX(mxc_gpio->port_num, i);
		val = mxc_get_gpio_datain(pin);
		dir = mxc_get_gpio_direction(pin);

                 seq_printf(s, "GPIO %s%d: %s %s\n",
			    chip->label, i,
                            (val == 1) ? "set" : (val == 0) ? "clear" : "error",
                            (dir == 1) ? "in" : (dir == 0) ? "out" : "error");
	}
}

#define MXC_GPIO_BANK(name, nport, base_gpio)                            \
         {                                                               \
                 .chip = {                                               \
                         .label            = name,                       \
                         .direction_input  = mx25_gpio_direction_input,  \
                         .direction_output = mx25_gpio_direction_output, \
                         .get              = mx25_gpio_get,              \
                         .set              = mx25_gpio_set,              \
                         .dbg_show         = mx25_gpio_dbg_show,         \
                         .base             = base_gpio,                  \
                         .ngpio            = GPIO_NUM_PIN,               \
                 },                                                      \
		 .port_num = nport,                                      \
         }

static struct mxc_gpio_chip mx25_gpio_banks[] = {
	MXC_GPIO_BANK("1", 0, MXC_GPIO_LINE_1_BASE),
	MXC_GPIO_BANK("2", 1, MXC_GPIO_LINE_2_BASE),
	MXC_GPIO_BANK("3", 2, MXC_GPIO_LINE_3_BASE),
	MXC_GPIO_BANK("4", 3, MXC_GPIO_LINE_4_BASE),
};

void mx25_generic_gpio_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mx25_gpio_banks); i++)
		gpiochip_add(&mx25_gpio_banks[i].chip);
}
EXPORT_SYMBOL(mx25_generic_gpio_init);
