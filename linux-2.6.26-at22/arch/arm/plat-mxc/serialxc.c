/*
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
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
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/fsl_devices.h>
#include <linux/usb/fsl_xcvr.h>
#include <linux/regulator/regulator.h>

#include <asm/hardware.h>
#include <asm/arch/arc_otg.h>

#if defined(CONFIG_MACH_ARMADILLO410) || defined(CONFIG_MACH_ARMADILLO420) || defined(CONFIG_MACH_ARMADILLO440) || defined(CONFIG_MACH_ARMADILLO460)
extern void armadillo400_set_vbus_power(struct fsl_usb2_platform_data *pdata,
					int on);
#endif

static void usb_serial_init(struct fsl_xcvr_ops *this)
{
}

static void usb_serial_uninit(struct fsl_xcvr_ops *this)
{
}

static void usb_serial_set_vbus_power(struct fsl_xcvr_ops *ops,
				      struct fsl_usb2_platform_data *pdata,
				      int on)
{
#if defined(CONFIG_MACH_ARMADILLO410) || defined(CONFIG_MACH_ARMADILLO420) || defined(CONFIG_MACH_ARMADILLO440) || defined(CONFIG_MACH_ARMADILLO460)
	armadillo400_set_vbus_power(pdata, on);
#endif /* defined(CONFIG_MACH_ARMADILLO410) || defined(CONFIG_MACH_ARMADILLO420) || defined(CONFIG_MACH_ARMADILLO440) || defined(CONFIG_MACH_ARMADILLO460)*/
}

static struct fsl_xcvr_ops serial_ops = {
	.name = "serial",
	.xcvr_type = PORTSC_PTS_SERIAL,
	.init = usb_serial_init,
	.uninit = usb_serial_uninit,
	.set_vbus_power = usb_serial_set_vbus_power,
};

extern void fsl_usb_xcvr_register(struct fsl_xcvr_ops *xcvr_ops);

static int __init serialxc_init(void)
{
	pr_debug("%s\n", __FUNCTION__);

	fsl_usb_xcvr_register(&serial_ops);

	return 0;
}

extern void fsl_usb_xcvr_unregister(struct fsl_xcvr_ops *xcvr_ops);

static void __exit serialxc_exit(void)
{
	fsl_usb_xcvr_unregister(&serial_ops);
}

module_init(serialxc_init);
module_exit(serialxc_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("serial xcvr driver");
MODULE_LICENSE("GPL");
