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

/*!
 * @file mbm.c
 *
 * @brief Driver for Freescale CAN Controller FlexCAN.
 *
 * @ingroup can
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/netdevice.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <linux/platform_device.h>

#include <asm/io.h>
#include <asm/irq.h>
#include "flexcan.h"

#define CAN_SFF_ID_SHIFT 18

#define CAN_SFF_ID_LENGTH 3
#define CAN_EFF_ID_LENGTH 8
#define CAN_MAX_DLC 8

#define CANID_DELIM '#'
#define DATA_SEPERATOR '.'

#define flexcan_swab32(x)	\
	(((x) << 24) | ((x) >> 24) |\
		(((x) & (__u32)0x0000ff00UL) << 8) |\
		(((x) & (__u32)0x00ff0000UL) >> 8))

static inline void flexcan_memcpy(void *dst, void *src, int len)
{
	int i;
	unsigned int *d = (unsigned int *)dst, *s = (unsigned int *)src;
	len = (len + 3) >> 2;
	for (i = 0; i < len; i++, s++, d++)
		*d = flexcan_swab32(*s);
}

static inline int can_strtoid(const char *str, int *ret)
{
	int len = strlen(str);

	if (str[len-1] == '\n')
		len--;

	if (len == CAN_SFF_ID_LENGTH) {
		*ret = simple_strtoul(str, NULL, 16) & CAN_SFF_MASK;
	} else if (len == CAN_EFF_ID_LENGTH) {
		*ret = (simple_strtoul(str, NULL, 16) & CAN_EFF_MASK)
			| CAN_EFF_FLAG;
	} else {
		return 1;
	}

	return 0;
}

static inline unsigned int get_frame_id(struct can_hw_mb *hwmb)
{
	if (hwmb->mb_cs.cs.ide == 1)
		return (hwmb->mb_id & CAN_EFF_MASK) | CAN_EFF_FLAG;
	else
		return (hwmb->mb_id >> CAN_SFF_ID_SHIFT) & CAN_SFF_MASK;
}

static unsigned char asc2nibble(char c)
{

	if ((c >= '0') && (c <= '9'))
		return c - '0';

	if ((c >= 'A') && (c <= 'F'))
		return c - 'A' + 10;

	if ((c >= 'a') && (c <= 'f'))
		return c - 'a' + 10;

	return 16; /* error */
}

static int parse_canframe(const char *cs, struct can_frame *cf) {
	int i, idx, dlc, len;
	unsigned char tmp;

	len = strlen(cs);

	memset(cf, 0, sizeof(*cf)); /* init CAN frame, e.g. DLC = 0 */

	if (len <= CAN_SFF_ID_LENGTH)
		return 1;

	if (cs[CAN_SFF_ID_LENGTH] == CANID_DELIM) {
		idx = CAN_SFF_ID_LENGTH + 1;
		for (i = 0; i < CAN_SFF_ID_LENGTH; i++) {
			if ((tmp = asc2nibble(cs[i])) > 0x0F)
				return 1;
			cf->can_id |= (tmp << (CAN_SFF_ID_LENGTH - i - 1) * 4);
		}
	} else if (len > CAN_EFF_ID_LENGTH &&
		   cs[CAN_EFF_ID_LENGTH] == CANID_DELIM) {
		idx = CAN_EFF_ID_LENGTH + 1;
		for (i = 0; i < CAN_EFF_ID_LENGTH; i++) {
			if ((tmp = asc2nibble(cs[i])) > 0x0F)
				return 1;
			cf->can_id |= (tmp << (CAN_EFF_ID_LENGTH - i - 1) * 4);
		}
		/* 8 digits but no errorframe?  */
		if (!(cf->can_id & CAN_ERR_FLAG))
			/* then it is an extended frame */
			cf->can_id |= CAN_EFF_FLAG;
	} else
		return 1;

	if ((cs[idx] == 'R') || (cs[idx] == 'r')) { /* RTR frame */
		cf->can_id |= CAN_RTR_FLAG;
		return 0;
	}

	for (i = 0, dlc = 0; i < CAN_MAX_DLC; i++) {
		/* skip (optional) seperator */
		if(cs[idx] == DATA_SEPERATOR || cs[idx] == '\n')
			idx++;

		if(idx >= len) /* end of string => end of data */
			break;

		if ((tmp = asc2nibble(cs[idx++])) > 0x0F)
			return 1;
		cf->data[i] = (tmp << 4);
		if ((tmp = asc2nibble(cs[idx++])) > 0x0F)
			return 1;
		cf->data[i] |= tmp;
		dlc++;
	}

	cf->can_dlc = dlc;

	return 0;
}


int flexcan_mbm_set_resframe(struct flexcan_device *flexcan, const char *buf)
{
	struct can_hw_mb *hwmb = flexcan->hwmb;
	struct can_frame frame;
	int xmit_mb = 0;
	int i;

	if (parse_canframe(buf, &frame)){
		return -EINVAL;
	}

	if (flexcan->fifo)
		i = FLEXCAN_MAX_FIFO_MB;
	else
		i = flexcan->rx_maxmb + 1;

	mutex_lock(&flexcan->xmit_mb_mutex);
	for (; i <= flexcan->maxmb; i++) {
		if (xmit_mb == 0 && hwmb[i].mb_cs.cs.code == CAN_MB_TX_INACTIVE)
			xmit_mb = i;
		if (frame.can_id == get_frame_id(&hwmb[i])) {
			xmit_mb = i;
			break;
		}
	}
	if (xmit_mb == 0) {
		mutex_unlock(&flexcan->xmit_mb_mutex);
		return -ENOBUFS;
	}

	if (frame.can_id & CAN_RTR_FLAG)
		hwmb[xmit_mb].mb_cs.cs.rtr = 1;
	else
		hwmb[xmit_mb].mb_cs.cs.rtr = 0;

	if (frame.can_id & CAN_EFF_FLAG) {
		hwmb[xmit_mb].mb_cs.cs.ide = 1;
		hwmb[xmit_mb].mb_cs.cs.srr = 1;
		hwmb[xmit_mb].mb_id = frame.can_id & CAN_EFF_MASK;
	} else {
		hwmb[xmit_mb].mb_cs.cs.ide = 0;
		hwmb[xmit_mb].mb_id = (frame.can_id & CAN_SFF_MASK) << 18;
	}

	hwmb[xmit_mb].mb_cs.cs.length = frame.can_dlc;
	flexcan_memcpy(hwmb[xmit_mb].mb_data, frame.data, frame.can_dlc);
	hwmb[xmit_mb].mb_cs.cs.code = CAN_MB_TX_REMOTE;

	mutex_unlock(&flexcan->xmit_mb_mutex);
	return 0;
}
EXPORT_SYMBOL(flexcan_mbm_set_resframe);

int flexcan_mbm_del_resframe(struct flexcan_device *flexcan, const char *buf)
{
	struct can_hw_mb *hwmb = flexcan->hwmb;
	unsigned int del_id;
	unsigned int id;
	int i;

	if (can_strtoid(buf, &del_id))
		return -EINVAL;

	if (flexcan->fifo)
		i = FLEXCAN_MAX_FIFO_MB;
	else
		i = flexcan->rx_maxmb + 1;

	mutex_lock(&flexcan->xmit_mb_mutex);
	for (; i <= flexcan->maxmb; i++) {
		if (hwmb[i].mb_cs.cs.code == CAN_MB_TX_REMOTE) {
			id = get_frame_id(&hwmb[i]);

			if (id == del_id) {
				hwmb[i].mb_cs.cs.code = CAN_MB_TX_INACTIVE;
				mutex_unlock(&flexcan->xmit_mb_mutex);
				return 0;
			}
		}
	}
	mutex_unlock(&flexcan->xmit_mb_mutex);
	return -EINVAL;
}
EXPORT_SYMBOL(flexcan_mbm_del_resframe);

ssize_t flexcan_mbm_show_resframe(struct flexcan_device *flexcan, char *buf)
{
	struct can_hw_mb *hwmb = flexcan->hwmb;
	unsigned int id, dlc;
	unsigned char data[CAN_MAX_DLC];
	int i, j;
	int ret = 0;

	if (flexcan->fifo)
		i = FLEXCAN_MAX_FIFO_MB;
	else
		i = flexcan->rx_maxmb + 1;

	mutex_lock(&flexcan->xmit_mb_mutex);
	for (; i <= flexcan->maxmb; i++) {
		if (hwmb[i].mb_cs.cs.code == CAN_MB_TX_REMOTE) {
			id = get_frame_id(&hwmb[i]);
			dlc = hwmb[i].mb_cs.cs.length;

			if (hwmb[i].mb_cs.cs.ide == 1)
				ret += sprintf(buf + ret, "ID:0x%08x DLC:%d DATA:",
						id & CAN_EFF_MASK, dlc);
			else
				ret += sprintf(buf + ret, "ID:0x%03x DLC:%d DATA:",
						id & CAN_SFF_MASK, dlc);

			flexcan_memcpy(data, hwmb[i].mb_data, dlc);
			for (j = 0; j < dlc; j++)
				ret += sprintf(buf + ret, " 0x%02x", data[j]);
			ret += sprintf(buf + ret, "\n");
		}
	}
	mutex_unlock(&flexcan->xmit_mb_mutex);

	return ret;
}
EXPORT_SYMBOL(flexcan_mbm_show_resframe);


static void flexcan_mb_bottom(struct net_device *dev, int index)
{
	struct flexcan_device *flexcan = netdev_priv(dev);
	struct net_device_stats *stats = dev->get_stats(dev);
	struct can_hw_mb *hwmb;
	struct can_frame *frame;
	struct sk_buff *skb;
	unsigned int tmp;

	hwmb = flexcan->hwmb + index;
	if (flexcan->fifo || index > flexcan->rx_maxmb) {
		if (hwmb->mb_cs.cs.code == CAN_MB_TX_ABORT)
			hwmb->mb_cs.cs.code = CAN_MB_TX_INACTIVE;

		if (hwmb->mb_cs.cs.code & CAN_MB_TX_INACTIVE) {
			if (netif_queue_stopped(dev))
				netif_start_queue(dev);
			return;
		}
	}

	skb = dev_alloc_skb(sizeof(struct can_frame));
	if (skb) {
		frame = (struct can_frame *)skb_put(skb, sizeof(*frame));
		memset(frame, 0, sizeof(*frame));
		if (hwmb->mb_cs.cs.ide)
			frame->can_id =
			    (hwmb->mb_id & CAN_EFF_MASK) | CAN_EFF_FLAG;
		else
			frame->can_id = (hwmb->mb_id >> 18) & CAN_SFF_MASK;

		if (hwmb->mb_cs.cs.rtr)
			frame->can_id |= CAN_RTR_FLAG;

		frame->can_dlc = hwmb->mb_cs.cs.length;

		if (frame->can_dlc && frame->can_dlc)
			flexcan_memcpy(frame->data, hwmb->mb_data,
				       frame->can_dlc);

		if (flexcan->fifo || index > flexcan->rx_maxmb) {
			hwmb->mb_cs.cs.code = CAN_MB_TX_INACTIVE;
			if (netif_queue_stopped(dev))
				netif_start_queue(dev);
		}

		tmp = __raw_readl(flexcan->io_base + CAN_HW_REG_TIMER);

		dev->last_rx = jiffies;
		stats->rx_packets++;
		stats->rx_bytes += frame->can_dlc;

		skb->dev = dev;
		skb->protocol = __constant_htons(ETH_P_CAN);
		skb->ip_summed = CHECKSUM_UNNECESSARY;
		netif_rx(skb);
	} else {
		tmp = hwmb->mb_cs.data;
		tmp = hwmb->mb_id;
		tmp = hwmb->mb_data[0];
		if (flexcan->fifo || index > flexcan->rx_maxmb) {
			hwmb->mb_cs.cs.code = CAN_MB_TX_INACTIVE;
			if (netif_queue_stopped(dev))
				netif_start_queue(dev);
		}
		tmp = __raw_readl(flexcan->io_base + CAN_HW_REG_TIMER);
		stats->rx_dropped++;
	}
}

static void flexcan_fifo_isr(struct net_device *dev, unsigned int iflag1)
{
	struct flexcan_device *flexcan = dev ? netdev_priv(dev) : NULL;
	struct net_device_stats *stats = dev->get_stats(dev);
	struct sk_buff *skb;
	struct can_hw_mb *hwmb = flexcan->hwmb;
	struct can_frame *frame;
	unsigned int tmp;

	if (iflag1 & __FIFO_RDY_INT) {
		skb = dev_alloc_skb(sizeof(struct can_frame));
		if (skb) {
			frame =
			    (struct can_frame *)skb_put(skb, sizeof(*frame));
			memset(frame, 0, sizeof(*frame));
			if (hwmb->mb_cs.cs.ide)
				frame->can_id =
				    (hwmb->mb_id & CAN_EFF_MASK) | CAN_EFF_FLAG;
			else
				frame->can_id =
				    (hwmb->mb_id >> 18) & CAN_SFF_MASK;

			if (hwmb->mb_cs.cs.rtr)
				frame->can_id |= CAN_RTR_FLAG;

			frame->can_dlc = hwmb->mb_cs.cs.length;

			if (frame->can_dlc && (frame->can_dlc <= 8))
				flexcan_memcpy(frame->data, hwmb->mb_data,
					       frame->can_dlc);
			tmp = __raw_readl(flexcan->io_base + CAN_HW_REG_TIMER);

			dev->last_rx = jiffies;

			stats->rx_packets++;
			stats->rx_bytes += frame->can_dlc;

			skb->dev = dev;
			skb->protocol = __constant_htons(ETH_P_CAN);
			skb->ip_summed = CHECKSUM_UNNECESSARY;
			netif_rx(skb);
		} else {
			tmp = hwmb->mb_cs.data;
			tmp = hwmb->mb_id;
			tmp = hwmb->mb_data[0];
			tmp = __raw_readl(flexcan->io_base + CAN_HW_REG_TIMER);
		}
	}

	if (iflag1 & (__FIFO_OV_INT | __FIFO_WARN_INT)) {
		skb = dev_alloc_skb(sizeof(struct can_frame));
		if (skb) {
			frame =
			    (struct can_frame *)skb_put(skb, sizeof(*frame));
			memset(frame, 0, sizeof(*frame));
			frame->can_id = CAN_ERR_FLAG | CAN_ERR_CRTL;
			frame->can_dlc = CAN_ERR_DLC;
			if (iflag1 & __FIFO_WARN_INT)
				frame->data[1] |=
				    CAN_ERR_CRTL_TX_WARNING |
				    CAN_ERR_CRTL_RX_WARNING;
			if (iflag1 & __FIFO_OV_INT)
				frame->data[1] |= CAN_ERR_CRTL_RX_OVERFLOW;

			skb->dev = dev;
			skb->protocol = __constant_htons(ETH_P_CAN);
			skb->ip_summed = CHECKSUM_UNNECESSARY;
			netif_rx(skb);
		}
	}
}

/*!
 * @brief The function call by CAN ISR to handle mb events.
 *
 * @param dev		the pointer of network device.
 *
 * @return none
 */
void flexcan_mbm_isr(struct net_device *dev)
{
	int i, iflag1, iflag2, maxmb;
	struct flexcan_device *flexcan = dev ? netdev_priv(dev) : NULL;

	if (flexcan->maxmb > 31) {
		maxmb = flexcan->maxmb + 1 - 32;
		iflag1 = __raw_readl(flexcan->io_base + CAN_HW_REG_IFLAG1) &
		    __raw_readl(flexcan->io_base + CAN_HW_REG_IMASK1);
		iflag2 = __raw_readl(flexcan->io_base + CAN_HW_REG_IFLAG2) &
		    __raw_readl(flexcan->io_base + CAN_HW_REG_IMASK2);
		iflag2 &= (1 << maxmb) - 1;
		maxmb = 32;
	} else {
		maxmb = flexcan->maxmb + 1;
		iflag1 = __raw_readl(flexcan->io_base + CAN_HW_REG_IFLAG1) &
		    __raw_readl(flexcan->io_base + CAN_HW_REG_IMASK1);
		iflag1 &= (1 << maxmb) - 1;
		iflag2 = 0;
	}

	__raw_writel(iflag1, flexcan->io_base + CAN_HW_REG_IFLAG1);
	__raw_writel(iflag2, flexcan->io_base + CAN_HW_REG_IFLAG2);

	if (flexcan->fifo) {
		flexcan_fifo_isr(dev, iflag1);
		iflag1 &= 0xFFFFFF00;
	}
	for (i = 0; iflag1 && (i < maxmb); i++) {
		if (iflag1 & (1 << i)) {
			iflag1 &= ~(1 << i);
			flexcan_mb_bottom(dev, i);
		}
	}

	for (i = maxmb; iflag2 && (i <= flexcan->maxmb); i++) {
		if (iflag2 & (1 << (i - 32))) {
			iflag2 &= ~(1 << (i - 32));
			flexcan_mb_bottom(dev, i);
		}
	}
}

/*!
 * @brief function to xmit message buffer
 *
 * @param flexcan	the pointer of can hardware device.
 * @param frame		the pointer of can message frame.
 *
 * @return	Returns 0 if xmit is success. otherwise returns non-zero.
 */
int flexcan_mbm_xmit(struct flexcan_device *flexcan, struct can_frame *frame)
{
	int i = flexcan->xmit_mb;
	struct can_hw_mb *hwmb = flexcan->hwmb;

	mutex_lock(&flexcan->xmit_mb_mutex);
	do {
		if (hwmb[i].mb_cs.cs.code == CAN_MB_TX_INACTIVE)
			break;
		if ((++i) > flexcan->maxmb) {
			if (flexcan->fifo)
				i = FLEXCAN_MAX_FIFO_MB;
			else
				i = flexcan->rx_maxmb + 1;
		}
		if (i == flexcan->xmit_mb) {
			mutex_unlock(&flexcan->xmit_mb_mutex);
			return -1;
		}
	} while (1);

	flexcan->xmit_mb = i + 1;
	if (flexcan->xmit_mb > flexcan->maxmb) {
		if (flexcan->fifo)
			flexcan->xmit_mb = FLEXCAN_MAX_FIFO_MB;
		else
			flexcan->xmit_mb = flexcan->rx_maxmb + 1;
	}

	if (frame->can_id & CAN_RTR_FLAG)
		hwmb[i].mb_cs.cs.rtr = 1;
	else
		hwmb[i].mb_cs.cs.rtr = 0;

	if (frame->can_id & CAN_EFF_FLAG) {
		hwmb[i].mb_cs.cs.ide = 1;
		hwmb[i].mb_cs.cs.srr = 1;
		hwmb[i].mb_id = frame->can_id & CAN_EFF_MASK;
	} else {
		hwmb[i].mb_cs.cs.ide = 0;
		hwmb[i].mb_id = (frame->can_id & CAN_SFF_MASK) << 18;
	}

	hwmb[i].mb_cs.cs.length = frame->can_dlc;
	flexcan_memcpy(hwmb[i].mb_data, frame->data, frame->can_dlc);
	hwmb[i].mb_cs.cs.code = CAN_MB_TX_ONCE;
	mutex_unlock(&flexcan->xmit_mb_mutex);
	return 0;
}

/*!
 * @brief function to initial message buffer
 *
 * @param flexcan	the pointer of can hardware device.
 *
 * @return	none
 */
void flexcan_mbm_init(struct flexcan_device *flexcan)
{
	struct can_hw_mb *hwmb;
	int xmit_mb_start, i;

	/* Set global mask to receive all messages */
	__raw_writel(0, flexcan->io_base + CAN_HW_REG_RXGMASK);
	__raw_writel(0, flexcan->io_base + CAN_HW_REG_RX14MASK);
	__raw_writel(0, flexcan->io_base + CAN_HW_REG_RX15MASK);

	memset(flexcan->hwmb, 0, sizeof(*hwmb) * FLEXCAN_MAX_MB);
	/* Set individual mask to receive all messages */
	memset(flexcan->rx_mask, 0, sizeof(unsigned int) * FLEXCAN_MAX_MB);

	if (flexcan->fifo)
		xmit_mb_start = FLEXCAN_MAX_FIFO_MB;
	else
		xmit_mb_start = flexcan->rx_maxmb + 1;

	hwmb = flexcan->hwmb;
	if (flexcan->fifo) {
		unsigned long *id_table = flexcan->io_base + CAN_FIFO_BASE;
		for (i = 0; i < xmit_mb_start; i++)
			id_table[i] = 0;
	} else {
		for (i = 0; i < xmit_mb_start; i++) {
			hwmb[i].mb_cs.cs.code = CAN_MB_RX_EMPTY;
			/*
			 * IDE bit can not control by mask registers
			 * So set message buffer to receive extend
			 * or standard message.
			 */
			if (flexcan->ext_msg && flexcan->std_msg)
				hwmb[i].mb_cs.cs.ide = i & 1;
			else {
				if (flexcan->ext_msg)
					hwmb[i].mb_cs.cs.ide = 1;
			}
		}
	}

	for (; i <= flexcan->maxmb; i++)
		hwmb[i].mb_cs.cs.code = CAN_MB_TX_INACTIVE;

	flexcan->xmit_mb = xmit_mb_start;
}
