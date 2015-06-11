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
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/tlv320aic.h>

#define DRIVER_NAME "tlv320aic"
#define AUTHOR      "Atmark Techno, Inc."
#define DESCRIPTION "TI TLV320AIC Audio codec driver"

#define NR_REGS (10)
#define I2C_TLV320AIC_ADDR (0x1a)
static unsigned short ignore[] = { I2C_CLIENT_END };
static unsigned short normal_addr[] = { I2C_TLV320AIC_ADDR, I2C_CLIENT_END };

static struct i2c_client_address_data addr_data = {
	.normal_i2c	= normal_addr,
	.probe		= ignore,
	.ignore		= ignore,
};

static u16 reginfo[NR_REGS];

static struct i2c_driver tlv320aic_driver;
static struct i2c_client *client_tlv320aic = NULL;
static DECLARE_MUTEX(mutex);

u16
tlv320aic_getreg(u8 reg)
{
	u16 ret;
	down(&mutex);
	if (reg <= TLV_DACT)
		ret = reginfo[reg];
	else 
		ret = -1;
	up(&mutex);
	return ret;
}
EXPORT_SYMBOL(tlv320aic_getreg);

int
tlv320aic_setreg(u8 reg, u16 val)
{
	int ret;
	u8 buf[2];
	struct i2c_msg msgs[1];

	if (!client_tlv320aic)
		return -1;
  
	msgs[0].addr  = client_tlv320aic->addr;
	msgs[0].flags = 0;
	msgs[0].len   = 2;
	msgs[0].buf   = buf;

	buf[0] = (reg << 1) | ((val & 0x100) ? 1 : 0);
	buf[1] = (val & 0x00ff);

	down(&mutex);
	ret = i2c_transfer(client_tlv320aic->adapter, msgs, 1);
	if (ret == 1) {
		ret = 0;
		if (reg <= TLV_DACT)
			reginfo[reg] = (val & 0x1ff);
	}
	else if (ret >= 0) {
		ret = -1;
	}
	up(&mutex);
  
	return ret;
}
EXPORT_SYMBOL(tlv320aic_setreg);

static void
tlv320aic_register_init(void)
{
	memset(reginfo, 0, sizeof(u16) * NR_REGS);

	tlv320aic_setreg(TLV_RESET, 0);

	/* set chip default value */
	tlv320aic_setreg(TLV_LIVOL,  0x097);
	tlv320aic_setreg(TLV_RIVOL,  0x097);
	tlv320aic_setreg(TLV_LHVOL,  0x0f9);
	tlv320aic_setreg(TLV_RHVOL,  0x0f9);
	tlv320aic_setreg(TLV_APATH,  0x00a);
	tlv320aic_setreg(TLV_DPATH,  0x008);
	tlv320aic_setreg(TLV_PWR,    0x007);
	tlv320aic_setreg(TLV_DIFORM, 0x001);
	tlv320aic_setreg(TLV_SRATE,  0x020);
	tlv320aic_setreg(TLV_DACT,   0x000);
}

static int
tlv320aic_probe(struct i2c_adapter *adap, int addr, int kind)
{
	int ret;

	client_tlv320aic = kzalloc(sizeof(struct i2c_client), GFP_KERNEL);
	if (!client_tlv320aic)
		return -ENOMEM;

	strcpy(client_tlv320aic->name, DRIVER_NAME);
	client_tlv320aic->addr = addr;
	client_tlv320aic->adapter = adap;
	client_tlv320aic->driver = &tlv320aic_driver;

	ret = i2c_attach_client(client_tlv320aic);
	if (ret) {
		kfree(client_tlv320aic);
		client_tlv320aic = NULL;
		return ret;
	}

	tlv320aic_register_init();

	dev_info(&adap->dev, "detected TLV320AIC Audio codec\n");

	return 0;
}

static int
tlv320aic_attach(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &addr_data, tlv320aic_probe);
}

static int
tlv320aic_detach(struct i2c_client *client)
{
	int ret;

	ret = i2c_detach_client(client);
	if (ret)
		return ret;

	kfree(client);
	client_tlv320aic = NULL;

	return 0;
}

static struct i2c_driver tlv320aic_driver = {
	.driver	= {
  		.name	= DRIVER_NAME,
	},
	.attach_adapter	= tlv320aic_attach,
	.detach_client	= tlv320aic_detach,
};

static int __init 
tlv320aic_init(void)
{
	return i2c_add_driver(&tlv320aic_driver);
}

static void __exit
tlv320aic_exit(void)
{
	i2c_del_driver(&tlv320aic_driver);
}

module_init(tlv320aic_init);
module_exit(tlv320aic_exit);

MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESCRIPTION);
MODULE_LICENSE("GPL");
