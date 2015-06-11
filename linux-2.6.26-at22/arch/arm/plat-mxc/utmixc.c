/*
 * Copyright 2005-2009 Freescale Semiconductor, Inc. All Rights Reserved.
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
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/fsl_devices.h>
#include <linux/usb/fsl_xcvr.h>
#include <linux/pmic_external.h>
#include <linux/regulator/regulator.h>

#include <asm/hardware.h>
#include <asm/arch/arc_otg.h>
#include <asm/mach-types.h>

#if defined(CONFIG_MACH_ARMADILLO410) || defined(CONFIG_MACH_ARMADILLO420) || defined(CONFIG_MACH_ARMADILLO440) || defined(CONFIG_MACH_ARMADILLO460)
extern void armadillo400_set_vbus_power(struct fsl_usb2_platform_data *pdata,
					int on);
#endif

static void usb_utmi_init(struct fsl_xcvr_ops *this)
{
}

static void usb_utmi_uninit(struct fsl_xcvr_ops *this)
{
}

static 	void usb_utmi_set_vbus_power(struct fsl_xcvr_ops *ops,
				     struct fsl_usb2_platform_data *pdata,
				     int on)
{
#if defined(CONFIG_MACH_ARMADILLO410) || defined(CONFIG_MACH_ARMADILLO420) || defined(CONFIG_MACH_ARMADILLO440) || defined(CONFIG_MACH_ARMADILLO460)
	armadillo400_set_vbus_power(pdata, on);
#endif /* defined(CONFIG_MACH_ARMADILLO410) || defined(CONFIG_MACH_ARMADILLO420) || defined(CONFIG_MACH_ARMADILLO440) || defined(CONFIG_MACH_ARMADILLO460) */
}

static struct fsl_xcvr_ops utmi_ops = {
	.name = "utmi",
	.xcvr_type = PORTSC_PTS_UTMI,
	.init = usb_utmi_init,
	.uninit = usb_utmi_uninit,
	.set_vbus_power = usb_utmi_set_vbus_power,
};

extern void fsl_usb_xcvr_register(struct fsl_xcvr_ops *xcvr_ops);

static int __init utmixc_init(void)
{
	fsl_usb_xcvr_register(&utmi_ops);
	return 0;
}

extern void fsl_usb_xcvr_unregister(struct fsl_xcvr_ops *xcvr_ops);

static void __exit utmixc_exit(void)
{
	fsl_usb_xcvr_unregister(&utmi_ops);
}

module_init(utmixc_init);
module_exit(utmixc_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("utmi xcvr driver");
MODULE_LICENSE("GPL");
