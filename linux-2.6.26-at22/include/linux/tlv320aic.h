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

#ifndef __LINUX_TLV320AIC_H__
#define __LINUX_TLV320AIC_H__

extern u16 tlv320aic_getreg(u8 addr);
extern int tlv320aic_setreg(u8 addr, u16 val);

/* Register Name */
#define TLV_LIVOL	0x00 /* Left line input channel volume control */
#define TLV_RIVOL	0x01 /* Right line input channel volume control */
#define TLV_LHVOL	0x02 /* Left channel headphone volume control */
#define TLV_RHVOL	0x03 /* Right channel headphone volume control */
#define TLV_APATH	0x04 /* Analog audio path control */
#define TLV_DPATH	0x05 /* Digital audio path control */
#define TLV_PWR		0x06 /* Power down control */
#define TLV_DIFORM	0x07 /* Digital audio interface format */
#define TLV_SRATE	0x08 /* Sample rate control */
#define TLV_DACT	0x09 /* Digital interface activation */
#define TLV_RESET	0x0f /* Reset Register */


#define TLV_LINE_INPUT_VOL_MASK (0x1f)
#define TLV_LINE_INPUT_VOL_MIN  (0x00)
#define TLV_LINE_INPUT_VOL_MAX  (0x1f)
#define TLV_HEADPHONE_VOL_MASK  (0x7f)
#define TLV_HEADPHONE_VOL_MIN   (0x30)
#define TLV_HEADPHONE_VOL_MAX   (0x7f)

#endif /* __LINUX_TLV320AIC_H__ */
