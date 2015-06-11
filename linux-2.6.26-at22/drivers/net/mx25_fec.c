/*
 * Fast Ethernet Controller (FEC) driver for Motorola MPC8xx.
 * Copyright (c) 1997 Dan Malek (dmalek@jlc.net)
 *
 * This version of the driver is specific to the FADS implementation,
 * since the board contains control registers external to the processor
 * for the control of the LevelOne LXT970 transceiver.  The MPC860T manual
 * describes connections using the internal parallel port I/O, which
 * is basically all of Port D.
 *
 * Right now, I am very wasteful with the buffers.  I allocate memory
 * pages and then divide them into 2K frame buffers.  This way I know I
 * have buffers large enough to hold one frame within one buffer descriptor.
 * Once I get this working, I will use 64 or 128 byte CPM buffers, which
 * will be much more memory efficient and will easily handle lots of
 * small packets.
 *
 * Much better multiple PHY support by Magnus Damm.
 * Copyright (c) 2000 Ericsson Radio Systems AB.
 *
 * Support for FEC controller of ColdFire processors.
 * Copyright (c) 2001-2005 Greg Ungerer (gerg@snapgear.com)
 *
 * Bug fixes and cleanup by Philippe De Muyter (phdm@macqel.be)
 * Copyright (c) 2004-2006 Macq Electronique SA.
 *
 * Copyright 2006-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Copyright (c) 2010 Atmark-techno, Inc. All Rights Reserved.
 */

#include <linux/platform_device.h>
#include <linux/mii.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/clk.h>

#include <asm/arch/mx25_fec.h>

#define FEC_ALIGNMENT  (0x0F)          /*FEC needs 128bits(32bytes) alignment*/
#define FEC_DRV_VERSION "2010-02-04"

static unsigned char	fec_mac_default[] = {
	0x00, 0x11, 0x0c, 0xc0, 0xff, 0xee,
};

/* This will define in linux/ioport.h */
static inline resource_size_t resource_size(struct resource *res)
{
	return res->end - res->start + 1;
}

#define FEC_ADDR_ALIGNMENT(x) ((unsigned char *)(((unsigned long )(x) + (FEC_ALIGNMENT)) & (~FEC_ALIGNMENT)))

/* The number of Tx and Rx buffers.  These are allocated from the page
 * pool.  The code may assume these are power of two, so it it best
 * to keep them that size.
 * We don't need to allocate pages for the transmitter.  We just use
 * the skbuffer directly.
 */
#define FEC_ENET_RX_PAGES	8
#define FEC_ENET_RX_FRSIZE	2048
#define FEC_ENET_RX_FRPPG	(PAGE_SIZE / FEC_ENET_RX_FRSIZE)
#define RX_RING_SIZE		(FEC_ENET_RX_FRPPG * FEC_ENET_RX_PAGES)
#define FEC_ENET_TX_FRSIZE	2048
#define FEC_ENET_TX_FRPPG	(PAGE_SIZE / FEC_ENET_TX_FRSIZE)
#define TX_RING_SIZE		16	/* Must be power of two */
#define TX_RING_MOD_MASK	15	/*   for this to work */

#if (((RX_RING_SIZE + TX_RING_SIZE) * 8) > PAGE_SIZE)
#error "FEC: descriptor ring size constants too large"
#endif

/* Interrupt events/masks.
*/
#define FEC_ENET_HBERR	((uint)0x80000000)	/* Heartbeat error */
#define FEC_ENET_BABR	((uint)0x40000000)	/* Babbling receiver */
#define FEC_ENET_BABT	((uint)0x20000000)	/* Babbling transmitter */
#define FEC_ENET_GRA	((uint)0x10000000)	/* Graceful stop complete */
#define FEC_ENET_TXF	((uint)0x08000000)	/* Full frame transmitted */
#define FEC_ENET_TXB	((uint)0x04000000)	/* A buffer was transmitted */
#define FEC_ENET_RXF	((uint)0x02000000)	/* Full frame received */
#define FEC_ENET_RXB	((uint)0x01000000)	/* A buffer was received */
#define FEC_ENET_MII	((uint)0x00800000)	/* MII interrupt */
#define FEC_ENET_EBERR	((uint)0x00400000)	/* SDMA bus error */

#define FEC_ENET_MASK   ((uint)0xfff80000)

#define FEC_RCR_MII_MODE (1 << 2)
#define FEC_RCR_DRT (1 << 1)

#define FEC_TCR_FDEN (1 << 2)

#define FEC_MII_MMFR_ST		(0x1 << 30)
#define FEC_MII_MMFR_OP_READ	(0x2 << 28)
#define FEC_MII_MMFR_OP_WRITE	(0x1 << 28)
#define FEC_MII_MMFR_PA(addr)	((addr & 0x1f) << 23)
#define FEC_MII_MMFR_RA(reg)	((reg & 0x1f) << 18)
#define FEC_MII_MMFR_TA		(0x2 << 16)
#define FEC_MII_MMFR_DATA(data)	(data & 0xffff)
#define FEC_MII_MMFR_DATA_MASK	(0xffff)

/*
 * i.MX25 allows RMII mode to be configured via a gasket
 */
#define FEC_MIIGSK_CFGR_FRCONT  (1 << 6)
#define FEC_MIIGSK_CFGR_LBMODE (1 << 4)
#define FEC_MIIGSK_CFGR_EMODE (1 << 3)
#define FEC_MIIGSK_CFGR_IF_MODE_MASK (3 << 0)
#define FEC_MIIGSK_CFGR_IF_MODE_7WIRE (0 << 0)
#define FEC_MIIGSK_CFGR_IF_MODE_MII (0 << 0)
#define FEC_MIIGSK_CFGR_IF_MODE_RMII (1 << 0)

#define FEC_MIIGSK_ENR_READY (1 << 2)
#define FEC_MIIGSK_ENR_EN (1 << 1)

/* The FEC stores dest/src/type, data, and checksum for receive packets.
 */
#if defined(CONFIG_VLAN_8021Q) || defined(CONFIG_VLAN_8021Q_MODULE)
#define PKT_MAXBUF_SIZE		(1518 + 4)
#else
#define PKT_MAXBUF_SIZE		1518
#endif /* CONFIG_VLAN_802_1Q */
#define PKT_MINBUF_SIZE		64
#define PKT_MAXBLR_SIZE		((PKT_MAXBUF_SIZE + 0xf) & ~0xf)

#define	OPT_FRAME_SIZE	(PKT_MAXBUF_SIZE << 16)

struct phy_info_t {
	uint id;
	char *name;
};

enum fec_phy_if_type {
	FEC_PHY_IF_7WIRE,
	FEC_PHY_IF_MII,
	FEC_PHY_IF_RMII,
};

/* The FEC buffer descriptors track the ring buffers.  The rx_bd_base and
 * tx_bd_base always point to the base of the buffer descriptors.  The
 * cur_rx and cur_tx point to the currently available buffer.
 * The dirty_tx tracks the current buffer that is being sent by the
 * controller.  The cur_tx and dirty_tx are equal under both completely
 * empty and completely full conditions.  The empty/ready indicator in
 * the buffer descriptor determines the actual condition.
 */
struct fec_enet_private {
	/* Hardware registers of the FEC device */
	volatile fec_t	*hwp;

	struct net_device *netdev;

	struct clk *clk;

	struct mii_if_info mii;

	/* The saved address of a sent-in-place packet/buffer, for skfree(). */
	unsigned char *tx_bounce[TX_RING_SIZE];
	struct	sk_buff* tx_skbuff[TX_RING_SIZE];
	struct  sk_buff* rx_skbuff[RX_RING_SIZE];
	ushort	skb_cur;
	ushort	skb_dirty;

	/* CPM dual port RAM relative addresses */
	dma_addr_t	bd_dma;
	/* Address of Rx and Tx buffers */
	struct bufdesc	*rx_bd_base;
	struct bufdesc	*tx_bd_base;
	/* The next free ring entry */
	struct bufdesc	*cur_rx, *cur_tx;
	/* The ring entries to be free()ed */
	struct bufdesc	*dirty_tx;

	uint	tx_full;
	/* hold while accessing the HW like ringbuffer for tx/rx but not MAC */
	spinlock_t hw_lock;

	uint	phy_speed;
	struct phy_info_t const	*phy;
	struct mutex phy_lock;
	struct completion mii_cmpl;
	struct delayed_work mii_poll_task;
	struct net_device *net;

	int speed;

	unsigned opened: 1;
	unsigned link: 1;
};

static irqreturn_t fec_enet_interrupt(int irq, void * dev_id);
static void fec_enet_tx(struct net_device *dev);
static void fec_enet_rx(struct net_device *dev);

static int fec_enet_open(struct net_device *dev);
static int fec_enet_close(struct net_device *dev);
static int fec_enet_start_xmit(struct sk_buff *skb, struct net_device *dev);
static void fec_enet_timeout(struct net_device *dev);
static void fec_enet_set_multicast_list(struct net_device *dev);

static void fec_restart(struct net_device *dev);
static void fec_stop(struct net_device *dev);
static void fec_set_mac_address(struct net_device *dev);
static void fec_set_phy_if(struct net_device *ndev, enum fec_phy_if_type type);
static void fec_set_mode(struct net_device *ndev);

static u16 fec_phy_read(struct fec_enet_private *fec_priv, unsigned int index);
static void fec_phy_write(struct fec_enet_private *fec_priv, unsigned int index, u16 val);

extern void gpio_fec_active(void);
extern void gpio_fec_inactive(void);
extern void gpio_link_led_active(void);
extern void gpio_link_led_inactive(void);

/* Transmitter timeout.
*/
#define TX_TIMEOUT (2*HZ)

/* The interrupt handler.
 * This is called from the MPC core interrupt.
 */
static irqreturn_t
fec_enet_interrupt(int irq, void * dev_id)
{
	struct	net_device *dev = dev_id;
	volatile fec_t	*fecp;
	uint	int_events;
	irqreturn_t ret = IRQ_NONE;

	fecp = (volatile fec_t*)dev->base_addr;

	/* Get the interrupt events that caused us to be here.
	*/
	do {
		int_events = fecp->fec_ievent;
		fecp->fec_ievent = int_events;

		/* Handle receive event in its own function.
		 */
		if (int_events & (FEC_ENET_RXF | FEC_ENET_RXB)) {
			ret = IRQ_HANDLED;
			fec_enet_rx(dev);
		}

		/* Transmit OK, or non-fatal error. Update the buffer
		   descriptors. FEC handles all errors, we just discover
		   them as part of the transmit process.
		*/
		if (int_events & (FEC_ENET_TXF | FEC_ENET_TXB)) {
			ret = IRQ_HANDLED;
			fec_enet_tx(dev);
		}

		if (int_events & FEC_ENET_MII) {
			struct fec_enet_private *fec_priv = netdev_priv(dev);
			ret = IRQ_HANDLED;
			complete(&fec_priv->mii_cmpl);
		}
	} while (int_events);

	return ret;
}


static void
fec_enet_tx(struct net_device *dev)
{
	struct	fec_enet_private *fec_priv;
	struct bufdesc *bdp;
	unsigned short status;
	struct	sk_buff	*skb;
	unsigned long flags;

	fec_priv = netdev_priv(dev);
	spin_lock_irqsave(&fec_priv->hw_lock, flags);
	bdp = fec_priv->dirty_tx;

	while (((status = bdp->cbd_sc) & BD_ENET_TX_READY) == 0) {
		if (bdp == fec_priv->cur_tx && fec_priv->tx_full == 0)
			break;

		skb = fec_priv->tx_skbuff[fec_priv->skb_dirty];
		dma_unmap_single(&dev->dev, bdp->cbd_bufaddr,
				 roundup(skb->len, 4), DMA_TO_DEVICE);
		bdp->cbd_bufaddr = 0;

		/* Check for errors. */
		if (status & (BD_ENET_TX_HB | BD_ENET_TX_LC |
				   BD_ENET_TX_RL | BD_ENET_TX_UN |
				   BD_ENET_TX_CSL)) {
			dev->stats.tx_errors++;
			if (status & BD_ENET_TX_HB)  /* No heartbeat */
				dev->stats.tx_heartbeat_errors++;
			if (status & BD_ENET_TX_LC)  /* Late collision */
				dev->stats.tx_window_errors++;
			if (status & BD_ENET_TX_RL)  /* Retrans limit */
				dev->stats.tx_aborted_errors++;
			if (status & BD_ENET_TX_UN)  /* Underrun */
				dev->stats.tx_fifo_errors++;
			if (status & BD_ENET_TX_CSL) /* Carrier lost */
				dev->stats.tx_carrier_errors++;
		} else {
			dev->stats.tx_packets++;
		}

		if (status & BD_ENET_TX_READY)
			printk("HEY! Enet xmit interrupt and TX_READY.\n");

		/* Deferred means some collisions occurred during transmit,
		 * but we eventually sent the packet OK.
		 */
		if (status & BD_ENET_TX_DEF)
			dev->stats.collisions++;

		/* Free the sk buffer associated with this last transmit. */
		dev_kfree_skb_any(skb);
		fec_priv->tx_skbuff[fec_priv->skb_dirty] = NULL;
		fec_priv->skb_dirty = (fec_priv->skb_dirty + 1) & TX_RING_MOD_MASK;

		/* Update pointer to next buffer descriptor to be transmitted. */
		if (status & BD_ENET_TX_WRAP)
			bdp = fec_priv->tx_bd_base;
		else
			bdp++;

		/* Since we have freed up a buffer, the ring is no longer full.
		 */
		if (fec_priv->tx_full) {
			fec_priv->tx_full = 0;
			if (netif_queue_stopped(dev))
				netif_wake_queue(dev);
		}
	}
	fec_priv->dirty_tx = bdp;
	spin_unlock_irqrestore(&fec_priv->hw_lock, flags);
}


/* During a receive, the cur_rx points to the current incoming buffer.
 * When we update through the ring, if the next incoming buffer has
 * not been given to the system, we just set the empty indicator,
 * effectively tossing the packet.
 */
static void
fec_enet_rx(struct net_device *dev)
{
	struct	fec_enet_private *fec_priv;
	volatile fec_t	*fecp;
	struct bufdesc *bdp;
	unsigned short status;
	struct	sk_buff	*skb;
	ushort	pkt_len;
	__u8 *data;
	int     rx_index ;
	unsigned long flags;

	fec_priv = netdev_priv(dev);
	fecp = (volatile fec_t*)dev->base_addr;

	spin_lock_irqsave(&fec_priv->hw_lock, flags);

	/* First, grab all of the stats for the incoming packet.
	 * These get messed up if we get called due to a busy condition.
	 */
	bdp = fec_priv->cur_rx;

	while (!((status = bdp->cbd_sc) & BD_ENET_RX_EMPTY)) {
		rx_index = bdp - fec_priv->rx_bd_base;

		/* Since we have allocated space to hold a complete frame,
		 * the last indicator should be set.
		 */
		if ((status & BD_ENET_RX_LAST) == 0)
			printk("FEC ENET: rcv is not +last\n");

		if (!fec_priv->opened)
			goto rx_processing_done;

		/* Check for errors. */
		if (status & (BD_ENET_RX_LG | BD_ENET_RX_SH | BD_ENET_RX_NO |
			      BD_ENET_RX_CR | BD_ENET_RX_OV)) {
			dev->stats.rx_errors++;
			if (status & (BD_ENET_RX_LG | BD_ENET_RX_SH)) {
				/* Frame too long or too short. */
				dev->stats.rx_length_errors++;
			}
			if (status & BD_ENET_RX_NO)	/* Frame alignment */
				dev->stats.rx_frame_errors++;
			if (status & BD_ENET_RX_CR)	/* CRC Error */
				dev->stats.rx_crc_errors++;
			if (status & BD_ENET_RX_OV)	/* FIFO overrun */
				dev->stats.rx_fifo_errors++;
		}

		/* Report late collisions as a frame error.
		 * On this error, the BD is closed, but we don't know what we
		 * have in the buffer.  So, just drop this frame on the floor.
		 */
		if (status & BD_ENET_RX_CL) {
			dev->stats.rx_errors++;
			dev->stats.rx_frame_errors++;
			goto rx_processing_done;
		}

		/* Process the incoming frame. */
		dev->stats.rx_packets++;
		pkt_len = bdp->cbd_datlen;
		dev->stats.rx_bytes += pkt_len;
		data = (__u8*)__va(bdp->cbd_bufaddr);

	        dma_unmap_single(NULL, bdp->cbd_bufaddr, bdp->cbd_datlen,
				 DMA_FROM_DEVICE);

		/* This does 16 byte alignment, exactly what we need.
		 * The packet length includes FCS, but we don't want to
		 * include that when passing upstream as it messes up
		 * bridging applications.
		 */
		skb = dev_alloc_skb(pkt_len - 4 + NET_IP_ALIGN);

		if (unlikely(!skb)) {
			printk("%s: Memory squeeze, dropping packet.\n",
					dev->name);
			dev->stats.rx_dropped++;
		} else {
			skb_reserve(skb, NET_IP_ALIGN);
			skb_put(skb, pkt_len - 4);	/* Make room */
			skb_copy_to_linear_data(skb, data, pkt_len - 4);
			skb->protocol = eth_type_trans(skb, dev);
			netif_rx(skb);
		}

		bdp->cbd_bufaddr = dma_map_single(NULL, data, bdp->cbd_datlen,
			DMA_FROM_DEVICE);

rx_processing_done:
		/* Clear the status flags for this buffer. */
		status &= ~BD_ENET_RX_STATS;

		/* Mark the buffer empty. */
		status |= BD_ENET_RX_EMPTY;
		bdp->cbd_sc = status;

		/* Update BD pointer to next entry. */
		if (status & BD_ENET_RX_WRAP)
			bdp = fec_priv->rx_bd_base;
		else
			bdp++;

		/* Doing this here will keep the FEC running while we process
		 * incoming frames.  On a heavily loaded network, we should be
		 * able to keep up at the expense of system resources.
		 */
		fecp->fec_r_des_active = 0;
	}
	fec_priv->cur_rx = bdp;
	spin_unlock_irqrestore(&fec_priv->hw_lock, flags);
}

/* ------------------------------------------------------------------------- */

static struct phy_info_t phy_info_lan8720 = {
	.id = 0x0007c0f,
	.name = "LAN8720",
};

static struct phy_info_t const * const phy_info[] = {
	&phy_info_lan8720,
	NULL
};

/* ------------------------------------------------------------------------- */

/*
 * do some initializtion based architecture of this chip
 */
static void __inline__ fec_arch_init(void)
{
	gpio_fec_active();
	return;
}
/*
 * do some cleanup based architecture of this chip
 */
static void __inline__ fec_arch_exit(void)
{
	gpio_fec_inactive();
	return;
}

static void __inline__ fec_localhw_setup(struct net_device *ndev)
{
	fec_set_phy_if(ndev, FEC_PHY_IF_RMII);
}

/* ------------------------------------------------------------------------- */

#ifndef MODULE
static int fec_mac_setup(char *new_mac)
{
	char *ptr, *p = new_mac;
	int i = 0;

	while (p && (*p) && i < 6) {
		ptr = strchr(p, ':');
		if (ptr)
			*ptr++ = '\0';

		if (strlen(p)) {
			unsigned long tmp = simple_strtoul(p, NULL, 16);
			if (tmp > 0xff)
				break;
			fec_mac_default[i++] = tmp;
		}
		p = ptr;
	}

	return 0;
}

__setup("fec_mac=", fec_mac_setup);
#endif

/* ------------------------------------------------------------------------- */

/*
 * Interface Mode Selection
 * MIIGSK_CFGR[I/F_MODE] RCR[MII_MODE] Interface Mode Selected
 * 00                    0             7-Wire
 * 00                    1             MII
 * 01                    1             RMII
 */
static void fec_set_phy_if(struct net_device *ndev, enum fec_phy_if_type type)
{
	struct fec_enet_private *fec_priv = netdev_priv(ndev);
	volatile fec_t *hwp = fec_priv->hwp;

	/* disable the gasket and wait */
	hwp->fec_miigsk_enr = 0;
	while (hwp->fec_miigsk_enr & FEC_MIIGSK_ENR_READY)
		udelay(1);

	switch (type) {
	case FEC_PHY_IF_7WIRE:
		hwp->fec_r_cntrl &= ~FEC_RCR_MII_MODE;
		hwp->fec_miigsk_cfgr = FEC_MIIGSK_CFGR_IF_MODE_7WIRE;
		break;
	case FEC_PHY_IF_MII:
		hwp->fec_r_cntrl |= FEC_RCR_MII_MODE;
		hwp->fec_miigsk_cfgr = FEC_MIIGSK_CFGR_IF_MODE_MII;
		break;
	case FEC_PHY_IF_RMII:
		hwp->fec_r_cntrl |= FEC_RCR_MII_MODE;
		hwp->fec_miigsk_cfgr = FEC_MIIGSK_CFGR_IF_MODE_RMII;
		break;
	default:
		break;
	}

	/* re-enable the gasket */
	hwp->fec_miigsk_enr = FEC_MIIGSK_ENR_EN;
}

static void fec_set_mode(struct net_device *ndev)
{
	struct fec_enet_private *fec_priv = netdev_priv(ndev);
	volatile fec_t *hwp = fec_priv->hwp;

	if (fec_priv->mii.full_duplex == DUPLEX_FULL) {
		hwp->fec_r_cntrl &= ~FEC_RCR_DRT;
		hwp->fec_x_cntrl |= FEC_TCR_FDEN;
	} else {
		hwp->fec_r_cntrl |= FEC_RCR_DRT;
		hwp->fec_x_cntrl &= ~FEC_TCR_FDEN;
	}

	if (fec_priv->speed == SPEED_100) {
		hwp->fec_miigsk_cfgr &= ~FEC_MIIGSK_CFGR_FRCONT;
	} else {
		hwp->fec_miigsk_cfgr |= FEC_MIIGSK_CFGR_FRCONT;
	}
}

/* ------------------------------------------------------------------------- */
static void fec_phy_autonego(struct net_device *ndev)
{
	struct fec_enet_private *fec_priv = netdev_priv(ndev);

	if (!fec_priv->mii.force_media) {
		fec_phy_write(fec_priv, MII_BMCR, BMCR_ANENABLE);
		mii_nway_restart(&fec_priv->mii);
	}
}

static void fec_phy_detect(struct net_device *ndev)
{
	struct fec_enet_private *fec_priv;
	struct mii_if_info *mii;
	uint phy_type;
	u16 reg;
	int i;

	fec_priv = netdev_priv(ndev);
	mii = &fec_priv->mii;

	for (mii->phy_id = 0; mii->phy_id <= 32; mii->phy_id++) {
		reg = fec_phy_read(fec_priv, MII_PHYSID1);
		phy_type = reg & 0xffff;
		if (phy_type == 0xfff || phy_type == 0)
			continue;

		/* Got first part of ID, now get remainder. */
		phy_type = phy_type << 16;
		reg = fec_phy_read(fec_priv, MII_PHYSID2);

		phy_type |= (reg & 0xffff);
		pr_info("fec: PHY @ 0x%x, ID 0x%08x",
			mii->phy_id, phy_type);

		for (i = 0; phy_info[i]; i++) {
			if(phy_info[i]->id == (phy_type >> 4))
				break;
		}

		if (phy_info[i])
			printk(" -- %s\n", phy_info[i]->name);
		else
			printk(" -- unknown PHY!\n");

		fec_priv->phy = phy_info[i];

		mii->advertising = fec_phy_read(fec_priv, MII_ADVERTISE);

		return;
	}

	mii->phy_id = 0;
	/* Disable external MII interface */
	fec_priv->phy_speed = 0;
	printk("FEC: No PHY device found.\n");
}

/* ------------------------------------------------------------------------- */

static void fec_phy_update_linkmode(struct net_device *ndev)
{
	struct fec_enet_private *fec_priv = netdev_priv(ndev);
	struct mii_if_info *mii = &fec_priv->mii;
	unsigned int old_carrier, new_carrier;
	u32 lpa, media, bmcr;

	/* check current and old link status */
	old_carrier = netif_carrier_ok(mii->dev) ? 1 : 0;
	new_carrier = (unsigned int) mii_link_ok(mii);

	if (old_carrier == new_carrier)
		return;

	if (old_carrier && !new_carrier) {
		netif_carrier_off(mii->dev);
		fec_priv->link = 0;
		gpio_link_led_inactive();

		pr_info("%s: link down\n", mii->dev->name);

		return;
	}

	fec_priv->link = 1;

	bmcr = mii->mdio_read(mii->dev, mii->phy_id, MII_BMCR);

	if (bmcr & BMCR_ANENABLE) {
		/* get MII advertise and LPA values */
		mii->advertising = mii->mdio_read(mii->dev, mii->phy_id, MII_ADVERTISE);
		lpa = mii->mdio_read(mii->dev, mii->phy_id, MII_LPA);

		/* figure out media and duplex from advertise and LPA values */
		media = mii_nway_result(lpa & mii->advertising);
		mii->full_duplex = (media & ADVERTISE_FULL) ? DUPLEX_FULL : DUPLEX_HALF;

		fec_priv->speed = (media & (ADVERTISE_100FULL | ADVERTISE_100HALF)) ? SPEED_100 : SPEED_10;
	} else {
		mii->full_duplex = (bmcr & BMCR_FULLDPLX) ? DUPLEX_FULL : DUPLEX_HALF;
		fec_priv->speed = (bmcr & BMCR_SPEED100) ? SPEED_100 : SPEED_10;
	}
	fec_set_mode(ndev);

	pr_info("%s: link up, %sMbps, %s-duplex\n",
		mii->dev->name,
		(fec_priv->speed == SPEED_100) ? "100" : "10",
		(mii->full_duplex == DUPLEX_FULL) ? "full" : "half");

	netif_carrier_on(mii->dev);

	gpio_link_led_active();
}

#define MII_POLL_STATE_INTERVAL (msecs_to_jiffies(100))

static void mii_poll_state(struct work_struct *work)
{
	struct fec_enet_private *fec_priv = container_of(work, struct fec_enet_private, mii_poll_task.work);
	struct net_device *ndev = fec_priv->netdev;

	if (fec_priv->phy)
		fec_phy_update_linkmode(ndev);

	schedule_delayed_work(&fec_priv->mii_poll_task, MII_POLL_STATE_INTERVAL);
}

/* ------------------------------------------------------------------------- */

static void fec_enet_free_buffers(struct net_device *ndev)
{
	struct fec_enet_private *fec_priv = netdev_priv(ndev);
	int i;
	struct sk_buff *skb;
	struct bufdesc	*bdp;

	bdp = fec_priv->rx_bd_base;
	for (i = 0; i < RX_RING_SIZE; i++) {
		skb = fec_priv->rx_skbuff[i];

		if (bdp->cbd_bufaddr)
			dma_unmap_single(&ndev->dev, bdp->cbd_bufaddr,
					FEC_ENET_RX_FRSIZE, DMA_FROM_DEVICE);
		if (skb)
			dev_kfree_skb(skb);
		bdp++;
	}

	bdp = fec_priv->tx_bd_base;
	for (i = 0; i < TX_RING_SIZE; i++)
		kfree(fec_priv->tx_bounce[i]);
}

static int fec_enet_alloc_buffers(struct net_device *ndev)
{
	struct fec_enet_private *fec_priv = netdev_priv(ndev);
	int i;
	struct sk_buff *skb;
	struct bufdesc	*bdp;

	bdp = fec_priv->rx_bd_base;
	for (i = 0; i < RX_RING_SIZE; i++) {
		skb = dev_alloc_skb(FEC_ENET_RX_FRSIZE);
		if (!skb) {
			fec_enet_free_buffers(ndev);
			return -ENOMEM;
		}
		fec_priv->rx_skbuff[i] = skb;

		bdp->cbd_bufaddr = dma_map_single(&ndev->dev, skb->data,
				FEC_ENET_RX_FRSIZE, DMA_FROM_DEVICE);
		bdp->cbd_sc = BD_ENET_RX_EMPTY;
		bdp++;
	}

	/* Set the last buffer to wrap. */
	bdp--;
	bdp->cbd_sc |= BD_SC_WRAP;

	bdp = fec_priv->tx_bd_base;
	for (i = 0; i < TX_RING_SIZE; i++) {
		fec_priv->tx_bounce[i] = kmalloc(FEC_ENET_TX_FRSIZE, GFP_KERNEL);

		bdp->cbd_sc = 0;
		bdp->cbd_bufaddr = 0;
		bdp++;
	}

	/* Set the last buffer to wrap. */
	bdp--;
	bdp->cbd_sc |= BD_SC_WRAP;

	return 0;
}

static int fec_enet_open(struct net_device *ndev)
{
	struct fec_enet_private *fec_priv = netdev_priv(ndev);
	int ret;

	/* I should reset the ring buffers here, but I don't yet know
	 * a simple way to do that.
	 */

	ret = fec_enet_alloc_buffers(ndev);
	if (ret)
		return ret;

	fec_restart(ndev);

	fec_priv->opened = 1;

	return 0;
}

static int fec_enet_close(struct net_device *ndev)
{
	struct fec_enet_private *fec_priv = netdev_priv(ndev);

	fec_priv->opened = 0;
	if (fec_priv->link) {
		fec_stop(ndev);
	}
	else if (fec_priv->phy)
		cancel_delayed_work_sync(&fec_priv->mii_poll_task);

        fec_enet_free_buffers(ndev);

	return 0;
}

static int fec_enet_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct fec_enet_private *fec_priv;
	volatile fec_t	*fecp;
	struct bufdesc *bdp;
	void *bufaddr;
	unsigned short	status;
	unsigned long flags;

	fec_priv = netdev_priv(ndev);
	fecp = (volatile fec_t*)ndev->base_addr;

	if (!fec_priv->link) {
		/* Link is down or autonegotiation is in progress. */
		return 1;
	}

	spin_lock_irqsave(&fec_priv->hw_lock, flags);
	/* Fill in a Tx ring entry */
	bdp = fec_priv->cur_tx;

	status = bdp->cbd_sc;

	if (status & BD_ENET_TX_READY) {
		/* Ooops.  All transmit buffers are full.  Bail out.
		 * This should not happen, since dev->tbusy should be set.
		 */
		printk("%s: tx queue full!.\n", ndev->name);
		spin_unlock_irqrestore(&fec_priv->hw_lock, flags);
		return NETDEV_TX_BUSY;
	}

	/* Clear all of the status flags. */
	status &= ~BD_ENET_TX_STATS;

	/* Set buffer length and buffer pointer.
	*/
	bufaddr = skb->data;
	bdp->cbd_datlen = skb->len;

	/*
	 * On some FEC implementations data must be aligned on
	 * 4-byte boundaries. Use bounce buffers to copy data
	 * and get it aligned. Ugh.
	 */
	if (((unsigned long) bufaddr) & FEC_ALIGNMENT) {
		unsigned int index;
		index = bdp - fec_priv->tx_bd_base;
		memcpy(fec_priv->tx_bounce[index], (void *)skb->data, skb->len);
		bufaddr = fec_priv->tx_bounce[index];
	}

	/* Save skb pointer. */
	fec_priv->tx_skbuff[fec_priv->skb_cur] = skb;

	ndev->stats.tx_bytes += skb->len;
	fec_priv->skb_cur = (fec_priv->skb_cur+1) & TX_RING_MOD_MASK;

	/* Push the data cache so the CPM does not get stale memory
	 * data.
	 */
	bdp->cbd_bufaddr = dma_map_single(&ndev->dev, bufaddr,
					  roundup(skb->len, 4),
					  DMA_TO_DEVICE);

	/* Send it on its way.  Tell FEC it's ready, interrupt when done,
	 * it's the last BD of the frame, and to put the CRC on the end.
	 */

	status |= (BD_ENET_TX_READY | BD_ENET_TX_INTR
			| BD_ENET_TX_LAST | BD_ENET_TX_TC);
	bdp->cbd_sc = status;

	ndev->trans_start = jiffies;

	/* Trigger transmission start */
	fecp->fec_x_des_active = 0;

	/* If this was the last BD in the ring, start at the beginning again. */
	if (status & BD_ENET_TX_WRAP)
		bdp = fec_priv->tx_bd_base;
	else
		bdp++;

	if (bdp == fec_priv->dirty_tx) {
		fec_priv->tx_full = 1;
		netif_stop_queue(ndev);
	}

	fec_priv->cur_tx = bdp;

	spin_unlock_irqrestore(&fec_priv->hw_lock, flags);

	return NETDEV_TX_OK;
}

static void fec_enet_timeout(struct net_device *ndev)
{
	printk("%s: transmit timed out.\n", ndev->name);
	ndev->stats.tx_errors++;
#ifdef DEBUG
	{
		struct fec_enet_private *fec_priv = netdev_priv(ndev);
		int	i;
		struct bufdesc *bdp;

		printk("Ring data dump: cur_tx %lx%s, dirty_tx %lx cur_rx: %lx\n",
		       (unsigned long)fec_priv->cur_tx, fec_priv->tx_full ? " (full)" : "",
		       (unsigned long)fec_priv->dirty_tx,
		       (unsigned long)fec_priv->cur_rx);

		bdp = fec_priv->tx_bd_base;
		printk(" tx: %u buffers\n",  TX_RING_SIZE);
		for (i = 0 ; i < TX_RING_SIZE; i++) {
			printk("  %08x: %04x %04x %08x\n",
			       (uint) bdp,
			       bdp->cbd_sc,
			       bdp->cbd_datlen,
			       (int) bdp->cbd_bufaddr);
			bdp++;
		}

		bdp = fec_priv->rx_bd_base;
		printk(" rx: %lu buffers\n",  (unsigned long) RX_RING_SIZE);
		for (i = 0 ; i < RX_RING_SIZE; i++) {
			printk("  %08x: %04x %04x %08x\n",
			       (uint) bdp,
			       bdp->cbd_sc,
			       bdp->cbd_datlen,
			       (int) bdp->cbd_bufaddr);
			bdp++;
		}
	}
#endif
	fec_restart(ndev);
	netif_wake_queue(ndev);
}

/* Set or clear the multicast filter for this adaptor.
 * Skeleton taken from sunlance driver.
 * The CPM Ethernet implementation allows Multicast as well as individual
 * MAC address filtering.  Some of the drivers check to make sure it is
 * a group multicast address, and discard those that are not.  I guess I
 * will do the same for now, but just remove the test if you want
 * individual filtering as well (do the upper net layers want or support
 * this kind of feature?).
 */

#define HASH_BITS	6		/* #bits in hash */
#define CRC32_POLY	0xEDB88320

static void fec_enet_set_multicast_list(struct net_device *ndev)
{
	struct fec_enet_private *fec_priv;
	volatile fec_t *hwp;
	struct dev_mc_list *dmi;
	unsigned int i, j, bit, data, crc;
	unsigned char hash;

	fec_priv = netdev_priv(ndev);
	hwp = fec_priv->hwp;

	if (ndev->flags&IFF_PROMISC) {
		hwp->fec_r_cntrl |= 0x0008;
	} else {

		hwp->fec_r_cntrl &= ~0x0008;

		if (ndev->flags & IFF_ALLMULTI) {
			/* Catch all multicast addresses, so set the
			 * filter to all 1's.
			 */
			hwp->fec_grp_hash_table_high = 0xffffffff;
			hwp->fec_grp_hash_table_low = 0xffffffff;
		} else {
			/* Clear filter and add the addresses in hash register.
			*/
			hwp->fec_grp_hash_table_high = 0;
			hwp->fec_grp_hash_table_low = 0;

			dmi = ndev->mc_list;

			for (j = 0; j < ndev->mc_count; j++, dmi = dmi->next)
			{
				/* Only support group multicast for now.
				*/
				if (!(dmi->dmi_addr[0] & 1))
					continue;

				/* calculate crc32 value of mac address
				*/
				crc = 0xffffffff;

				for (i = 0; i < dmi->dmi_addrlen; i++)
				{
					data = dmi->dmi_addr[i];
					for (bit = 0; bit < 8; bit++, data >>= 1)
					{
						crc = (crc >> 1) ^
						(((crc ^ data) & 1) ? CRC32_POLY : 0);
					}
				}

				/* only upper 6 bits (HASH_BITS) are used
				   which point to specific bit in he hash registers
				*/
				hash = (crc >> (32 - HASH_BITS)) & 0x3f;

				if (hash > 31)
					hwp->fec_grp_hash_table_high |= 1 << (hash - 32);
				else
					hwp->fec_grp_hash_table_low |= 1 << hash;
			}
		}
	}
}

/* Set a MAC change in hardware.
 */
static void
fec_set_mac_address(struct net_device *ndev)
{
	struct fec_enet_private *fec_priv = netdev_priv(ndev);
	volatile fec_t *hwp = fec_priv->hwp;

	/* Set station address. */
	hwp->fec_addr_low = ndev->dev_addr[3] | (ndev->dev_addr[2] << 8) |
		(ndev->dev_addr[1] << 16) | (ndev->dev_addr[0] << 24);
	hwp->fec_addr_high = (ndev->dev_addr[5] << 16) |
		(ndev->dev_addr[4] << 24);

}

/* ------------------------------------------------------------------------- */

static void fec_ethtool_get_drvinfo(struct net_device *ndev, struct ethtool_drvinfo *info)
{
	strncpy(info->driver, ndev->name, sizeof(info->driver));
	strncpy(info->version, FEC_DRV_VERSION, sizeof(info->version));
	strncpy(info->bus_info, ndev->dev.parent->bus_id,
		sizeof(info->bus_info));
}

static int fec_ethtool_get_settings(struct net_device *ndev, struct ethtool_cmd *ecmd)
{
	struct fec_enet_private *fec_priv = netdev_priv(ndev);

	ecmd->maxtxpkt = 1;
	ecmd->maxrxpkt = 1;

	return mii_ethtool_gset(&fec_priv->mii, ecmd);
}

static int fec_ethtool_set_settings(struct net_device *ndev, struct ethtool_cmd *ecmd)
{
	struct fec_enet_private *fec_priv = netdev_priv(ndev);
	int ret = 0;

	ret =  mii_ethtool_sset(&fec_priv->mii, ecmd);

	fec_priv->speed = ecmd->speed;

	fec_set_mode(ndev);

	return ret;
}

static int fec_ethtool_nwayreset(struct net_device *ndev)
{
	struct fec_enet_private *fec_priv = netdev_priv(ndev);

	return mii_nway_restart(&fec_priv->mii);
}

static struct ethtool_ops fec_ethtool_ops = {
	.get_settings = fec_ethtool_get_settings,
	.set_settings = fec_ethtool_set_settings,
	.get_link = ethtool_op_get_link,
	.get_drvinfo = fec_ethtool_get_drvinfo,
	.nway_reset = fec_ethtool_nwayreset,
};

/* ------------------------------------------------------------------------- */

static u16 fec_phy_read(struct fec_enet_private *fec_priv, unsigned int index)
{
	u32 regval;

	mutex_lock(&fec_priv->phy_lock);

	init_completion(&fec_priv->mii_cmpl);

	regval = FEC_MII_MMFR_ST | FEC_MII_MMFR_OP_READ | FEC_MII_MMFR_TA |
		FEC_MII_MMFR_PA(fec_priv->mii.phy_id) | FEC_MII_MMFR_RA(index);

	fec_priv->hwp->fec_mii_data = regval;
	if (wait_for_completion_interruptible(&fec_priv->mii_cmpl) == 0)
		regval = fec_priv->hwp->fec_mii_data;
	else
		pr_err("%s: wait_for_completion interrupted\n", __func__);

	mutex_unlock(&fec_priv->phy_lock);

	return regval & FEC_MII_MMFR_DATA_MASK;
}

static void fec_phy_write(struct fec_enet_private *fec_priv, unsigned int index, u16 val)
{
	u32 regval;

	mutex_lock(&fec_priv->phy_lock);

	init_completion(&fec_priv->mii_cmpl);

	regval = FEC_MII_MMFR_ST | FEC_MII_MMFR_OP_WRITE | FEC_MII_MMFR_TA |
		FEC_MII_MMFR_PA(fec_priv->mii.phy_id) | FEC_MII_MMFR_RA(index) |
		FEC_MII_MMFR_DATA(val);

	fec_priv->hwp->fec_mii_data = regval;
	if (wait_for_completion_interruptible(&fec_priv->mii_cmpl) == 0)
		regval = fec_priv->hwp->fec_mii_data;
	else
		pr_err("%s: wait_for_completion interrupted\n", __func__);

	mutex_unlock(&fec_priv->phy_lock);
}

static int fec_mdio_read(struct net_device *ndev, int phy_id, int location)
{
	struct fec_enet_private *fec_priv = netdev_priv(ndev);
	u16 reg;

	reg = fec_phy_read(fec_priv, location);

	return reg;
}

static void fec_mdio_write(struct net_device *ndev, int phy_id,
			   int location, int val)
{
	struct fec_enet_private *fec_priv = netdev_priv(ndev);

	fec_phy_write(fec_priv, location, val);
}

/* ------------------------------------------------------------------------- */

 /*
  * XXX:  We need to clean up on failure exits here.
  *
  * index is only used in legacy code
  */
int __devinit fec_enet_init(struct net_device *ndev, int index)
{
	struct fec_enet_private *fec_priv = netdev_priv(ndev);
	struct bufdesc *cbd_base;
	volatile fec_t	*hwp;
	unsigned long l;
	unsigned char mask_ff[] = {0xff, 0xff, 0xff};
	unsigned char mask_00[] = {0x00, 0x00, 0x00};

	fec_priv->net = ndev;

	/* Allocate memory for buffer descriptors. */
	cbd_base = dma_alloc_coherent(NULL, PAGE_SIZE, &fec_priv->bd_dma,
			GFP_KERNEL);
	if (!cbd_base) {
		printk("FEC: allocate descriptor memory failed?\n");
		return -ENOMEM;
	}

	spin_lock_init(&fec_priv->hw_lock);
	mutex_init(&fec_priv->phy_lock);

	/* Create an Ethernet device instance.
	*/
	hwp = (volatile fec_t *)ndev->base_addr;

	fec_priv->hwp = hwp;
	fec_priv->netdev = ndev;
	fec_priv->speed = SPEED_10;
	fec_priv->mii.phy_id_mask = 0x1f;
	fec_priv->mii.reg_num_mask = 0x1f;
	fec_priv->mii.force_media = 0;
	fec_priv->mii.full_duplex = DUPLEX_HALF;
	fec_priv->mii.dev = ndev;
	fec_priv->mii.mdio_read = fec_mdio_read;
	fec_priv->mii.mdio_write = fec_mdio_write;

	/* Whack a reset.  We should wait for this.
	*/
	hwp->fec_ecntrl = 1;
	udelay(10);

	/* Set the Ethernet address */
	l = hwp->fec_addr_low;
	ndev->dev_addr[0] = (unsigned char)((l & 0xFF000000) >> 24);
	ndev->dev_addr[1] = (unsigned char)((l & 0x00FF0000) >> 16);
	ndev->dev_addr[2] = (unsigned char)((l & 0x0000FF00) >> 8);
	ndev->dev_addr[3] = (unsigned char)((l & 0x000000FF) >> 0);
	l = hwp->fec_addr_high;
	ndev->dev_addr[4] = (unsigned char)((l & 0xFF000000) >> 24);
	ndev->dev_addr[5] = (unsigned char)((l & 0x00FF0000) >> 16);

	if (memcmp(&ndev->dev_addr[3], mask_ff, 3) == 0 ||
	    memcmp(&ndev->dev_addr[3], mask_00, 3) == 0) {
		pr_info("invalid MAC address. using random MAC!\n");
		random_ether_addr(ndev->dev_addr);
	}

	pr_debug("MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
		 ndev->dev_addr[0], ndev->dev_addr[1], ndev->dev_addr[2],
		 ndev->dev_addr[3], ndev->dev_addr[4], ndev->dev_addr[5]);

	/* Set receive and transmit descriptor base.
	*/
	fec_priv->rx_bd_base = cbd_base;
	fec_priv->tx_bd_base = cbd_base + RX_RING_SIZE;

	/* Clear and enable interrupts */
	hwp->fec_ievent = FEC_ENET_MASK;
	hwp->fec_imask = FEC_ENET_TXF | FEC_ENET_TXB | FEC_ENET_RXF | FEC_ENET_RXB | FEC_ENET_MII;

	hwp->fec_grp_hash_table_high = 0;
	hwp->fec_grp_hash_table_low = 0;
	hwp->fec_r_buff_size = PKT_MAXBLR_SIZE;
	hwp->fec_ecntrl = 2;
	hwp->fec_r_des_active = 0x01000000;

	/* The FEC Ethernet specific entries in the device structure. */
	ndev->open = fec_enet_open;
	ndev->hard_start_xmit = fec_enet_start_xmit;
	ndev->tx_timeout = fec_enet_timeout;
	ndev->watchdog_timeo = TX_TIMEOUT;
	ndev->stop = fec_enet_close;
	ndev->set_multicast_list = fec_enet_set_multicast_list;
	ndev->ethtool_ops = &fec_ethtool_ops;

	/* setup MII interface */
	hwp->fec_r_cntrl = OPT_FRAME_SIZE | 0x04;
	hwp->fec_x_cntrl = 0x00;

	/*
	 * Set MII speed to 2.5 MHz
	 */
	fec_priv->phy_speed = ((((clk_get_rate(fec_priv->clk) / 2 + 4999999)
					/ 2500000) / 2) & 0x3F) << 1;
	hwp->fec_mii_speed = fec_priv->phy_speed;

	return 0;
}

/* This function is called to start or restart the FEC during a link
 * change.  This only happens when switching between half and full
 * duplex.
 */
static void
fec_restart(struct net_device *ndev)
{
	struct fec_enet_private *fec_priv;
	struct bufdesc *bdp;
	volatile fec_t *hwp;
	int i;

	fec_priv = netdev_priv(ndev);
	hwp = fec_priv->hwp;

	/* Whack a reset.  We should wait for this. */
	hwp->fec_ecntrl = 1;
	udelay(10);

	/* Enable interrupts we wish to service. */
	hwp->fec_imask = FEC_ENET_TXF | FEC_ENET_TXB | FEC_ENET_RXF | FEC_ENET_RXB | FEC_ENET_MII;

	/* Clear any outstanding interrupt. */
	hwp->fec_ievent = FEC_ENET_MASK;

	/* Set station address. */
	fec_set_mac_address(ndev);

	/* Reset all multicast. */
	hwp->fec_grp_hash_table_high = 0;
	hwp->fec_grp_hash_table_low = 0;

	/* Set maximum receive buffer size. */
	hwp->fec_r_buff_size = PKT_MAXBLR_SIZE;

	fec_localhw_setup(ndev);

	/* Set receive and transmit descriptor base. */
	hwp->fec_r_des_start = fec_priv->bd_dma;
	hwp->fec_x_des_start = (unsigned long)fec_priv->bd_dma +
		sizeof(struct bufdesc) * RX_RING_SIZE;

	fec_priv->dirty_tx = fec_priv->cur_tx = fec_priv->tx_bd_base;
	fec_priv->cur_rx = fec_priv->rx_bd_base;

	/* Reset SKB transmit buffers. */
	fec_priv->skb_cur = fec_priv->skb_dirty = 0;
	for (i=0; i<=TX_RING_MOD_MASK; i++) {
		if (fec_priv->tx_skbuff[i] != NULL) {
			dev_kfree_skb_any(fec_priv->tx_skbuff[i]);
			fec_priv->tx_skbuff[i] = NULL;
		}
	}

	/* Initialize the receive buffer descriptors. */
	bdp = fec_priv->rx_bd_base;
	for (i=0; i<RX_RING_SIZE; i++) {

		/* Initialize the BD for every fragment in the page. */
		bdp->cbd_sc = BD_ENET_RX_EMPTY;
		bdp++;
	}

	/* Set the last buffer to wrap.	*/
	bdp--;
	bdp->cbd_sc |= BD_SC_WRAP;

	/* ...and the same for transmmit. */
	bdp = fec_priv->tx_bd_base;
	for (i=0; i<TX_RING_SIZE; i++) {

		/* Initialize the BD for every fragment in the page. */
		bdp->cbd_sc = 0;
		bdp->cbd_bufaddr = 0;
		bdp++;
	}

	/* Set the last buffer to wrap. */
	bdp--;
	bdp->cbd_sc |= BD_SC_WRAP;

	/* Set MII speed. */
	hwp->fec_mii_speed = fec_priv->phy_speed;

	/* Enable MII mode. */
	hwp->fec_r_cntrl = OPT_FRAME_SIZE | 0x04;

	/* detect phy*/
	if (fec_priv->phy == NULL)
		fec_phy_detect(ndev);

	if (fec_priv->phy)
		fec_phy_autonego(ndev);

	/* And last, enable the transmit and receive processing. */
	hwp->fec_ecntrl = 2;
	hwp->fec_r_des_active = 0;

	if (fec_priv->link)
		netif_start_queue(ndev);

	if (fec_priv->phy) {
		INIT_DELAYED_WORK(&fec_priv->mii_poll_task, mii_poll_state);
		schedule_delayed_work(&fec_priv->mii_poll_task, MII_POLL_STATE_INTERVAL);
	}

}

static void
fec_stop(struct net_device *ndev)
{
	volatile fec_t *hwp;
	struct fec_enet_private *fec_priv;

	fec_priv = netdev_priv(ndev);
	hwp = fec_priv->hwp;

	cancel_delayed_work_sync(&fec_priv->mii_poll_task);

	netif_stop_queue(ndev);

	/*
	** We cannot expect a graceful transmit stop without link !!!
	*/
	if (fec_priv->link)
		{
		hwp->fec_x_cntrl = 0x01;	/* Graceful transmit stop */
		udelay(10);
		if (!(hwp->fec_ievent & FEC_ENET_GRA))
			printk("fec_stop : Graceful transmit stop did not complete !\n");
		}

	/* Whack a reset.  We should wait for this.
	*/
	hwp->fec_ecntrl = 1;
	udelay(10);

	/* Clear outstanding MII command interrupts.
	*/
	hwp->fec_ievent = FEC_ENET_MII;

	hwp->fec_imask = FEC_ENET_MII;
	hwp->fec_mii_speed = fec_priv->phy_speed;
}

static int __devinit
fec_probe(struct platform_device *pdev)
{
	struct fec_enet_private *fec_priv;
	volatile fec_t	*hwp;
	struct net_device *ndev;
	int i, irq, ret = 0;
	struct resource *r;

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r)
		return -ENXIO;

	r = request_mem_region(r->start, resource_size(r), pdev->name);
	if (!r)
		return -EBUSY;

	/* Init network device */
	ndev = alloc_etherdev(sizeof(struct fec_enet_private));
	if (!ndev)
		return -ENOMEM;

	SET_NETDEV_DEV(ndev, &pdev->dev);

	/* setup board info structure */
	fec_priv = netdev_priv(ndev);
	memset(fec_priv, 0, sizeof(*fec_priv));

	ndev->base_addr = (unsigned long)ioremap(r->start, resource_size(r));

	if (!ndev->base_addr) {
		ret = -ENOMEM;
		goto failed_ioremap;
	}

	platform_set_drvdata(pdev, ndev);

	fec_priv->clk = clk_get(&pdev->dev, "fec_clk");
	if (IS_ERR(fec_priv->clk)) {
		ret = PTR_ERR(fec_priv->clk);
		goto failed_clk;
	}
	clk_enable(fec_priv->clk);

	hwp = (volatile fec_t *)ndev->base_addr;
	/* Whack a reset.  We should wait for this.
	*/
	hwp->fec_ecntrl = 1;
	udelay(10);

	/* This device has up to three irqs on some platforms */
	for (i = 0; i < 3; i++) {
		irq = platform_get_irq(pdev, i);
		if (i && irq < 0)
			break;

		ret = request_irq(irq, fec_enet_interrupt, IRQF_DISABLED, pdev->name, ndev);
		if (ret) {
			while (i >= 0) {
				irq = platform_get_irq(pdev, i);
				free_irq(irq, ndev);
				i--;
			}
			goto failed_irq;
		}
	}

	fec_arch_init();

	ret = fec_enet_init(ndev, 0);
	if (ret)
		goto failed_init;

	ret = register_netdev(ndev);
	if (ret)
		goto failed_register;

	return 0;

failed_register:
failed_init:
	clk_disable(fec_priv->clk);
	clk_put(fec_priv->clk);

	for (i = 0; i < 3; i++) {
		irq = platform_get_irq(pdev, i);
		if (irq > 0)
			free_irq(irq, ndev);
	}
failed_irq:
	iounmap((void __iomem *)ndev->base_addr);
failed_clk:
failed_ioremap:
	free_netdev(ndev);

	return ret;
}

static int __devexit
fec_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct fec_enet_private *fec_priv = netdev_priv(ndev);

	platform_set_drvdata(pdev, NULL);

	fec_stop(ndev);
	clk_disable(fec_priv->clk);
	clk_put(fec_priv->clk);
	fec_arch_exit();
	iounmap((void __iomem *)ndev->base_addr);
	unregister_netdev(ndev);
	free_netdev(ndev);

	return 0;
}

#if defined(CONFIG_PM)
static int
fec_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct fec_enet_private *fec_priv;
	struct fec_platform_data *plat_data = pdev->dev.platform_data;

	if (ndev) {
		fec_priv = netdev_priv(ndev);

		if (fec_priv && fec_priv->phy) {
			fec_phy_write(fec_priv, MII_BMCR, BMCR_PDOWN | BMCR_ISOLATE);
		}

		if (netif_running(ndev)) {
			netif_device_detach(ndev);
			fec_stop(ndev);
		}
		else if (fec_priv->phy)
			cancel_delayed_work_sync(&fec_priv->mii_poll_task);

		if (plat_data) {
			if (plat_data->suspend) {
				plat_data->suspend(pdev, state);
			}
		}
	}

	return 0;
}

static int
fec_resume(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct fec_platform_data *plat_data = pdev->dev.platform_data;

	if (plat_data) {
		if (plat_data->resume) {
			plat_data->resume(pdev);
		}
	}

	if (ndev) {
		if (netif_running(ndev)) {
			fec_restart(ndev);
			netif_device_attach(ndev);
		}
	}

	return 0;
}
#else
#define fec_suspend NULL
#define fec_resume NULL
#endif /* defined(CONFIG_PM) */

static struct platform_driver fec_driver = {
	.driver	= {
		.name    = "fec",
		.owner	 = THIS_MODULE,
	},
	.probe   = fec_probe,
	.remove  = __devexit_p(fec_remove),
	.suspend = fec_suspend,
	.resume  = fec_resume,
};

static int __init
fec_enet_module_init(void)
{
	printk(KERN_INFO "FEC Ethernet Driver\n");

	return platform_driver_register(&fec_driver);
}
module_init(fec_enet_module_init);

static void __exit
fec_enet_cleanup(void)
{
	platform_driver_unregister(&fec_driver);
}
module_exit(fec_enet_cleanup);

MODULE_LICENSE("GPL");
