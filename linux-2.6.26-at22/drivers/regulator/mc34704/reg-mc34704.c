/*
 * Copyright 2008-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/ioctl.h>
#include <linux/regulator/regulator-platform.h>
#include <linux/regulator/regulator-drv.h>
#include <linux/platform_device.h>
#include <linux/pmic_status.h>
#include <linux/pmic_external.h>

#define MC34704_ONOFFA	0x8
#define MC34704_ONOFFC	0x4
#define MC34704_ONOFFD	0x2
#define MC34704_ONOFFE	0x1

enum {
	MC34704_REG1,		/* REG1 5V */
	MC34704_REG2,		/* REG2 3.3V */
	MC34704_REG3,		/* REG3 1.45V */
	MC34704_REG4,		/* REG4 1.8V */
	MC34704_REG5,		/* REG5 3.3V */
	MC34704_REG6,		/* REG6 not used */
	MC34704_REG7,		/* REG7 not used */
	MC34704_REG8,		/* REG8 not used */
} MC34704_regulator;

/* Regulator voltages in mV and dynamic voltage scaling limits in percent */

#define REG1_V_MV		5000
#define REG1_DVS_MIN_PCT	(-10)
#define REG1_DVS_MAX_PCT	10

#define REG2_V_MV		3300
#define REG2_DVS_MIN_PCT	(-20)
#define REG2_DVS_MAX_PCT	17.5

#define REG3_V_MV		1450
#define REG3_DVS_MIN_PCT	(-20)
#define REG3_DVS_MAX_PCT	17.5

#define REG4_V_MV		1800
#define REG4_DVS_MIN_PCT	(-20)
#define REG4_DVS_MAX_PCT	17.5

#define REG5_V_MV		3300
#define REG5_DVS_MIN_PCT	(-20)
#define REG5_DVS_MAX_PCT	17.5

/* Private data for MC34704 regulators */

struct reg_mc34704_priv {
	short enable;		/* enable bit, if available */
	short v_default;	/* default regulator voltage in mV */
	int dvs_min;		/* minimum voltage change in units of 2.5% */
	int dvs_max;		/* maximum voltage change in units of 2.5% */
	char i2c_dvs;		/* i2c DVS register number */
	char i2c_stat;		/* i2c status register number */
};

struct reg_mc34704_priv mc34704_reg1_priv = {
	.v_default = REG1_V_MV,
	.dvs_min = REG1_DVS_MIN_PCT / 2.5,
	.dvs_max = REG1_DVS_MAX_PCT / 2.5,
	.i2c_dvs = 0x4,
	.i2c_stat = 0x5,
	.enable = MC34704_ONOFFA,
};
struct reg_mc34704_priv mc34704_reg2_priv = {
	.v_default = REG2_V_MV,
	.dvs_min = REG2_DVS_MIN_PCT / 2.5,
	.dvs_max = REG2_DVS_MAX_PCT / 2.5,
	.i2c_dvs = 0x6,
	.i2c_stat = 0x7,
};
struct reg_mc34704_priv mc34704_reg3_priv = {
	.v_default = REG3_V_MV,
	.dvs_min = REG3_DVS_MIN_PCT / 2.5,
	.dvs_max = REG3_DVS_MAX_PCT / 2.5,
	.i2c_dvs = 0x8,
	.i2c_stat = 0x9,
};
struct reg_mc34704_priv mc34704_reg4_priv = {
	.v_default = REG4_V_MV,
	.dvs_min = REG4_DVS_MIN_PCT / 2.5,
	.dvs_max = REG4_DVS_MAX_PCT / 2.5,
	.i2c_dvs = 0xA,
	.i2c_stat = 0xB,
};
struct reg_mc34704_priv mc34704_reg5_priv = {
	.v_default = REG5_V_MV,
	.dvs_min = REG5_DVS_MIN_PCT / 2.5,
	.dvs_max = REG5_DVS_MAX_PCT / 2.5,
	.i2c_dvs = 0xC,
	.i2c_stat = 0xE,
	.enable = MC34704_ONOFFE,
};

static int mc34704_set_voltage(struct regulator *reg, int uV)
{
	struct reg_mc34704_priv *priv = regulator_get_drvdata(reg);
	int mV = uV / 1000;
	int dV = mV - priv->v_default;

	/* compute dynamic voltage scaling value */
	int dvs = 1000 * dV / priv->v_default / 25;

	/* clip to regulator limits */
	if (dvs > priv->dvs_max)
		dvs = priv->dvs_max;
	if (dvs < priv->dvs_min)
		dvs = priv->dvs_min;

	return pmic_write_reg(priv->i2c_dvs, dvs << 1, 0x1E);
}

static int mc34704_get_voltage(struct regulator *reg)
{
	int mV;
	struct reg_mc34704_priv *priv = regulator_get_drvdata(reg);
	int val, dvs;

	CHECK_ERROR(pmic_read_reg(priv->i2c_dvs, &val, 0x1E));

	dvs = (val >> 1) & 0xF;

	/* dvs is 4-bit 2's complement; sign-extend it */
	if (dvs & 8)
		dvs |= -1 & ~0xF;

	/* Regulator voltage is adjusted by (dvs * 2.5%) */
	mV = priv->v_default * (1000 + 25 * dvs) / 1000;

	return 1000 * mV;
}

static int mc34704_enable_reg(struct regulator *reg)
{
	struct reg_mc34704_priv *priv = regulator_get_drvdata(reg);

	if (priv->enable) {
		return pmic_write_reg(REG_MC34704_GENERAL2, priv->enable, priv->enable);
	}

	return PMIC_ERROR;
}

static int mc34704_disable_reg(struct regulator *reg)
{
	struct reg_mc34704_priv *priv = regulator_get_drvdata(reg);

	if (priv->enable)
		return pmic_write_reg(REG_MC34704_GENERAL2, 0, priv->enable);

	return PMIC_ERROR;
}

static int mc34704_is_reg_enabled(struct regulator *reg)
{
	struct reg_mc34704_priv *priv = regulator_get_drvdata(reg);
	int val;

	if (priv->enable) {
		CHECK_ERROR(pmic_read_reg(REG_MC34704_GENERAL2, &val,
					  priv->enable));
		return val ? 1 : 0;
	} else {
		return PMIC_ERROR;
	}
}

static int mc34704_always_enabled(struct regulator *reg)
{
	return 1;
}

static struct regulator_ops mc34704_full_ops = {
	.set_voltage = mc34704_set_voltage,
	.get_voltage = mc34704_get_voltage,
	.enable = mc34704_enable_reg,
	.disable = mc34704_disable_reg,
	.is_enabled = mc34704_is_reg_enabled,
};

static struct regulator_ops mc34704_partial_ops = {
	.set_voltage = mc34704_set_voltage,
	.get_voltage = mc34704_get_voltage,
	.is_enabled = mc34704_always_enabled,
};

struct regulation_constraints mc34704_reg1_constraints = {
	.min_uV = mV_to_uV(REG1_V_MV * (1000 + REG1_DVS_MIN_PCT * 10) / 1000),
	.max_uV = mV_to_uV(REG1_V_MV * (1000 + REG1_DVS_MAX_PCT * 10) / 1000),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};
struct regulation_constraints mc34704_reg2_constraints = {
	.min_uV = mV_to_uV(REG2_V_MV * (1000 + REG2_DVS_MIN_PCT * 10) / 1000),
	.max_uV = mV_to_uV(REG2_V_MV * (1000 + REG2_DVS_MAX_PCT * 10) / 1000),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};
struct regulation_constraints mc34704_reg3_constraints = {
	.min_uV = mV_to_uV(REG3_V_MV * (1000 + REG3_DVS_MIN_PCT * 10) / 1000),
	.max_uV = mV_to_uV(REG3_V_MV * (1000 + REG3_DVS_MAX_PCT * 10) / 1000),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};
struct regulation_constraints mc34704_reg4_constraints = {
	.min_uV = mV_to_uV(REG4_V_MV * (1000 + REG4_DVS_MIN_PCT * 10) / 1000),
	.max_uV = mV_to_uV(REG4_V_MV * (1000 + REG4_DVS_MAX_PCT * 10) / 1000),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};
struct regulation_constraints mc34704_reg5_constraints = {
	.min_uV = mV_to_uV(REG5_V_MV * (1000 + REG5_DVS_MIN_PCT * 10) / 1000),
	.max_uV = mV_to_uV(REG5_V_MV * (1000 + REG5_DVS_MAX_PCT * 10) / 1000),
	.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
};

static struct regulator reg_mc34704[] = {
	{
	 .name = "REG1",
	 .id = MC34704_REG1,
	 .ops = &mc34704_full_ops,
	 .constraints = &mc34704_reg1_constraints,
	 .use_count = 0,
	 .reg_data = &mc34704_reg1_priv,
	}, {
	 .name = "REG2",
	 .id = MC34704_REG2,
	 .ops = &mc34704_partial_ops,
	 .constraints = &mc34704_reg2_constraints,
	 .use_count = 1,
	 .reg_data = &mc34704_reg2_priv,
	}, {
	 .name = "REG3",
	 .id = MC34704_REG3,
	 .ops = &mc34704_partial_ops,
	 .constraints = &mc34704_reg3_constraints,
	 .use_count = 1,
	 .reg_data = &mc34704_reg3_priv,
	}, {
	 .name = "REG4",
	 .id = MC34704_REG4,
	 .ops = &mc34704_partial_ops,
	 .constraints = &mc34704_reg4_constraints,
	 .use_count = 1,
	 .reg_data = &mc34704_reg4_priv,
	}, {
	 .name = "REG5",
	 .id = MC34704_REG5,
	 .ops = &mc34704_full_ops,
	 .constraints = &mc34704_reg5_constraints,
	 .use_count = 1,
	 .reg_data = &mc34704_reg5_priv,
	},
};

int reg_mc34704_probe(void)
{
	int ret;
	int i;

	for (i = 0; i < ARRAY_SIZE(reg_mc34704); i++) {
		ret = regulator_register(reg_mc34704 + i);
		regulator_set_platform_constraints(reg_mc34704[i].name,
						   reg_mc34704[i].constraints);
		if (ret < 0) {
			printk(KERN_ERR "%s: failed to register %s err %d\n",
			       __func__, reg_mc34704[i].name, ret);
			i--;
			for (; i >= 0; i--)
				regulator_unregister(reg_mc34704 + i);

			return ret;
		}
	}

	printk(KERN_INFO "MC34704 regulator successfully probed\n");

	return 0;
}
EXPORT_SYMBOL(reg_mc34704_probe);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MC34704 Regulator Driver");
MODULE_LICENSE("GPL");
