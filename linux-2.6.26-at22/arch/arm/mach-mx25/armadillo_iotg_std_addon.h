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

#ifndef __ASM_ARCH_ARMADILLO_IOTG_STD_ADD_ON_H__
#define __ASM_ARCH_ARMADILLO_IOTG_STD_ADD_ON_H__

/* EEPROM I2C slave address */
#define AISA_ADDR_CON1	(0x50) /* CON1(Ext.I/F1) */
#define AISA_ADDR_CON2	(0x51) /* CON2(Ext.I/F2) */

/* EEPROM data offset */
#define AISA_VENDOR_ID		(0x00) /*  2 bytes  */
#define AISA_PRODUCT_ID		(0x02) /*  2 bytes  */
#define AISA_REVISION		(0x04) /*  2 bytes  */
#define AISA_SERIAL_NO		(0x06) /*  4 bytes  */
#define AISA_RESERVED		(0x0A) /* 22 bytes  */
#define AISA_VENDOR_SPECIFIC	(0x20) /* 96 bytes  */

/* EEPROM vendor ID */
#define AISA_VENDOR_ID_ATMARK_TECHNO	(0x0001)

/* EEPROM product ID(assigned by vendor) */
#define AISA_PRODUCT_ID_ATMARK_TECHNO_WI_SUN	(0x0001)
#define AISA_PRODUCT_ID_ATMARK_TECHNO_EN_OCEAN	(0x0002)
#define AISA_PRODUCT_ID_ATMARK_TECHNO_SERIAL	(0x0003)
#define AISA_PRODUCT_ID_ATMARK_TECHNO_DIDOAD	(0x0004)
#define AISA_PRODUCT_ID_ATMARK_TECHNO_BLE	(0x0005)
#define AISA_PRODUCT_ID_ATMARK_TECHNO_CAN	(0x0006)
#define AISA_PRODUCT_ID_ATMARK_TECHNO_ZIGBEE	(0x0007)
#define AISA_PRODUCT_ID_ATMARK_TECHNO_RS232C	(0x0008)

#endif
