#ifndef __LINUX_USB_FSL_XCVR_H
#define __LINUX_USB_FSL_XCVR_H
#include <linux/platform_device.h>

struct fsl_usb2_platform_data;

/**
 * @name: transceiver name
 * @xcvr_type: one of PORTSC_PTS_{UTMI,SERIAL,ULPI}
 * @init: transceiver- and board-specific initialization function
 * @uninit: transceiver- and board-specific uninitialization function
 * @set_host:
 * @set_device:
 *
 */
struct fsl_xcvr_ops {
	char *name;
	u32 xcvr_type;

	void (*init)(struct fsl_xcvr_ops *ops);
	void (*uninit)(struct fsl_xcvr_ops *ops);
	void (*suspend)(struct fsl_xcvr_ops *ops);
	void (*set_host)(void);
	void (*set_device)(void);
	void (*set_vbus_power)(struct fsl_xcvr_ops *ops,
			       struct fsl_usb2_platform_data *pdata, int on);
	void (*set_remote_wakeup)(u32 *view);
	void (*pullup)(int on);
	int (*scratch_test)(u32 *view);
	void (*reset)(u32 *view);
};

struct fsl_xcvr_power {
	struct platform_device *usb_pdev;
	struct regulator *regu1;
	struct regulator *regu2;
};
#endif
