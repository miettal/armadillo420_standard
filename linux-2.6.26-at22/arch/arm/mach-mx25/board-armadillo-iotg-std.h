/*
 * Copyright (C) 2014 Atmark Techno, Inc. All Rights Reserved.
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
#ifndef __ASM_ARCH_BOARD_ARMADILLO_IOTG_STD_H__
#define __ASM_ARCH_BOARD_ARMADILLO_IOTG_STD_H__

#define USB_PWREN_GPIO	GPIO(3, 26)
#define USB_PWREN_OFF	1
#define USB_PWREN_ON	0

#define EXT_USB_SEL_GPIO		(MXC_MAX_GPIO_LINES + 5)

#define AIOTG_STD_SDHC2_CD_GPIO		GPIO(1, 7)
#define AIOTG_STD_SDHC2_WP_GPIO		GPIO(2, 21)
#define AIOTG_STD_SDHC2_SDHC2_PWRE	GPIO(2, 20)
#define AIOTG_STD_SD_AWLAN_SEL		(MXC_MAX_GPIO_LINES + 4)

#define AIOTG_STD_RESET_N_3G_GPIO	GPIO(3, 15)
#define AIOTG_STD_W_DISABLE_3G_GPIO	GPIO(1, 16)

#define AIOTG_STD_TEMP_SENSOR_OS_GPIO		GPIO(2, 22)
#define AIOTG_STD_AD_CONVERTER_ALERT_GPIO	GPIO(3, 17)

enum armadillo_iot_std_i2c_id {
	AIOTG_STD_GPIO_I2C1_ID = MX25_NR_I2C_IDS,
	AIOTG_STD_GPIO_I2C2_ID,
};

#endif				/* __ASM_ARCH_BOARD_ARMADILLO_IOTG_STD_H__ */
