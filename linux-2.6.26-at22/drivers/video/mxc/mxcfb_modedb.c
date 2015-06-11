/*
 * Copyright 2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/kernel.h>
#include <asm/arch/mxcfb.h>

struct fb_videomode mxcfb_modedb[] = {
	{
	 /* 240x320 @ 60 Hz */
	 "Sharp-QVGA", 60, 240, 320, 185925, 9, 16, 7, 9, 1, 1,
	 FB_SYNC_HOR_HIGH_ACT,
	 FB_VMODE_NONINTERLACED,
	 0,},
	{
	 /* 240x33 @ 60 Hz */
	 "Sharp-CLI", 60, 240, 33, 185925, 9, 16, 7, 9 + 287, 1, 1,
	 FB_SYNC_HOR_HIGH_ACT,
	 FB_VMODE_NONINTERLACED,
	 0,},
	{
	 "Sharp-LS037V7DW01-VGA", 60, 480, 640, 39700, 78, 88, 1, 6, 2, 1,
	 0,
	 FB_VMODE_NONINTERLACED,
	 0,},
	{
	 /* 640x480 @ 60 Hz */
	 "NEC-VGA", 60, 640, 480, 38255, 144, 0, 34, 40, 1, 1,
	 FB_SYNC_VERT_HIGH_ACT,
	 FB_VMODE_NONINTERLACED,
	 0,},
	{
	 /* NTSC TV output */
	 "TV-NTSC", 60, 640, 480, 37538,
	 38, 858 - 640 - 38 - 3,
	 36, 518 - 480 - 36 - 1,
	 3, 1,
	 0,
	 FB_VMODE_NONINTERLACED,
	 0,},
	{
	 /* PAL TV output */
	 "TV-PAL", 50, 640, 480, 37538,
	 38, 960 - 640 - 38 - 32,
	 32, 555 - 480 - 32 - 3,
	 32, 3,
	 0,
	 FB_VMODE_NONINTERLACED,
	 0,},
	{
	 /* TV output VGA mode, 640x480 @ 65 Hz */
	 "TV-VGA", 60, 640, 480, 40574, 35, 45, 9, 1, 46, 5,
	 0, FB_VMODE_NONINTERLACED, 0,
	 },
	{
		"CRT-VGA", 60, 640, 480, 39721, 32, 64, 40, 3, 64, 2,
		0,
		FB_VMODE_NONINTERLACED,
		0,
	},
	{
		"CRT-SVGA", 56, 800, 600, 30000, 45, 39, 10, 12, 64, 2,
		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
		FB_VMODE_NONINTERLACED,
		0,
	},
	{ /* for HSP: 177MHz */
		"CRT-SVGA56", 56, 800, 600, 27778, 28, 132, 21, 2, 64, 2,
		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
		FB_VMODE_NONINTERLACED,
		0,
	},
	{ /* for HSP: 177MHz */
		"CRT-SVGA60", 60, 800, 600, 25000, 56, 104, 35, 16, 64, 4,
		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
		FB_VMODE_NONINTERLACED,
		0,
	},
	{
		"KYOCERA-VGA", 60, 640, 480, 39714, 115, 15, 35, 7, 30, 3,
		0,
		FB_VMODE_NONINTERLACED,
		0,
	},
        {
		/* 640x480 @ 60 Hz */
		"CPT-VGA", 60, 640, 480, 39683, 45, 114, 33, 11, 1, 1,
		0,
		FB_VMODE_NONINTERLACED,
		0,
	},
	{
		/* ET043005DH6 */
		"ET043005DH6", 60, 480, 272, 100000, 31, 31, 2, 2, 41, 10,
		0,
		FB_VMODE_NONINTERLACED,
		0,
	},
	{
		/* VGG322423 */
		"VGG322423", 75, 320, 240, 125000, 20, 19, 4, 3, 49, 15,
		0,
		FB_VMODE_NONINTERLACED,
		0,
	},
	{
		"FG040341DSSWBGT1", 60, 480, 272, 100000, 35, 63, 5, 6, 5, 2,
		0,
		FB_VMODE_NONINTERLACED,
		0,
	},
	{
		"FG040360DSSWBG03", 60, 480, 272, 110229 ,35, 5, 6, 8, 5, 2,
		0, /* sync */
		FB_VMODE_NONINTERLACED, /* vmode */
		0, /* flag */
	},
	{
		"FG050720DSSWDGT1", 60, 640, 480, 40000, 114, 16, 32, 10, 30, 3,
		0, /* sync */
		FB_VMODE_NONINTERLACED, /* vmode */
		0, /* flag */
	},
	{
		"FG100410DNCWBGT1", 60, 640, 480, 39683, 0, 0, 0, 0, 45, 160,
		0, /* sync */
		FB_VMODE_NONINTERLACED, /* vmode */
		0, /* flag */
	},
	{
		"AA043MA01", 60, 800, 480, 32895, 0, 0, 0, 0, 45, 160,
		0, /* sync */
		FB_VMODE_NONINTERLACED, /* vmode */
		0, /* flag */
	},
};

int mxcfb_modedb_sz = ARRAY_SIZE(mxcfb_modedb);
EXPORT_SYMBOL(mxcfb_modedb);
EXPORT_SYMBOL(mxcfb_modedb_sz);

static struct mxcfb_mode_disp mxcfb_mode_disp_db[] = {
	{
		.name = "Sharp-QVGA",
		.disp_iface = MXCFB_DISP_SHARP_MODE |
				MXCFB_DISP_CLK_INVERT |
				MXCFB_DISP_DATA_INVERT |
				MXCFB_DISP_CLK_IDLE_EN,
	},
	{
		.name = "Sharp-CLI",
		.disp_iface = MXCFB_DISP_SHARP_MODE |
				MXCFB_DISP_CLK_INVERT |
				MXCFB_DISP_DATA_INVERT |
				MXCFB_DISP_CLK_IDLE_EN,
	},
	{
		.name = "Sharp-LS037V7DW01-VGA",
		.disp_iface = MXCFB_DISP_OE_ACT_HIGH,
	},
	{
		.name = "NEC-VGA",
		.disp_iface = MXCFB_DISP_OE_ACT_HIGH,
	},
	{
		.name = "CRT-VGA",
		.disp_iface = MXCFB_DISP_OE_ACT_HIGH,
	},
	{
		.name = "CRT-SVGA",
		.disp_iface = MXCFB_DISP_OE_ACT_HIGH,
	},
	{
		.name = "CRT-SVGA56",
		.disp_iface = MXCFB_DISP_OE_ACT_HIGH,
	},
	{
		.name = "CRT-SVGA60",
		.disp_iface = MXCFB_DISP_OE_ACT_HIGH,
	},
	{
		.name = "KYOCERA-VGA",
		.disp_iface = MXCFB_DISP_OE_ACT_HIGH,
	},
	{
		.name = "CPT-VGA",
		.disp_iface = MXCFB_DISP_OE_ACT_HIGH,
	},
	{
		.name = "ET043005DH6",
		.disp_iface = MXCFB_DISP_OE_ACT_HIGH,
	},
	{
		.name = "VGG322423",
		.disp_iface = MXCFB_DISP_CLK_INVERT,
	},
	{
		.name = "FG040341DSSWBGT1",
		.disp_iface = MXCFB_DISP_OE_ACT_HIGH,
	},
	{
		.name = "FG040360DSSWBG03",
		.disp_iface = MXCFB_DISP_OE_ACT_HIGH | MXCFB_DISP_CLK_INVERT,
	},
	{
		.name = "FG050720DSSWDGT1",
		.disp_iface = MXCFB_DISP_OE_ACT_HIGH,
	},
	{
		.name = "FG100410DNCWBGT1",
		.disp_iface = MXCFB_DISP_OE_ACT_HIGH,
	},
	{
		.name = "AA043MA01",
		.disp_iface = MXCFB_DISP_OE_ACT_HIGH,
	},
};

int mxcfb_disp_iface_from_mode(char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mxcfb_mode_disp_db); i++) {
		if (strcmp(mxcfb_mode_disp_db[i].name, name) == 0)
			return mxcfb_mode_disp_db[i].disp_iface;
	}

	return 0;
}
EXPORT_SYMBOL(mxcfb_disp_iface_from_mode);

MODULE_LICENSE("GPL");
