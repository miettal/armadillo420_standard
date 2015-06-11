/*
 *  Copyright 2009 Atmark Techno, Inc. All Rights Reserved.
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

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/ioport.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>

#include <asm/mach/flash.h>
#include <asm/arch/mtd.h>

#define DRIVER_NAME "armadillo-nor"
#define AUTHOR      "Atmark Techno, Inc."
#define DESCRIPTION "MTD map and partitions for Armadillo"

#undef DEBUG
#if defined(DEBUG)
#define DEBUG_FUNC()         printk(DRIVER_NAME ": %s()\n", __FUNCTION__)
#define DEBUG_INFO(args...)  printk(DRIVER_NAME ": " args)
#else
#define DEBUG_FUNC()
#define DEBUG_INFO(args...)
#endif
#define PRINT_INFO(args...)  printk(KERN_INFO  DRIVER_NAME ": " args)
#define PRINT_DEBUG(args...) printk(KERN_DEBUG DRIVER_NAME ": " args)
#define PRINT_ERR(args...)   printk(KERN_ERR   DRIVER_NAME ": " args)

struct armadillo_flash_info {
        struct flash_platform_data *plat;
        struct resource         *res;
        struct mtd_partition    *parts;
        struct mtd_info         *mtd;
        struct map_info         map;
};

static const char *probes[] = { "cmdlinepart", NULL };

static int
armadillo_mtd_probe(struct platform_device *pdev)
{
	struct armadillo_flash_private_data *priv = pdev->dev.platform_data;
	struct flash_platform_data *plat = &priv->plat;
	struct resource *res;
	unsigned int region_size;
	struct armadillo_flash_info *info;
	int err;
	int default_parts = 0;

	info = kmalloc(sizeof(struct armadillo_flash_info), GFP_KERNEL);
	if (!info) {
		err = -ENOMEM;
		goto out;
	}

	memset(info, 0, sizeof(struct armadillo_flash_info));

	info->plat = plat;
	if (plat && plat->init) {
		err = plat->init();
		if (err)
			goto no_resource;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		err = -ENOMEM;
		goto no_resource;
	}
	region_size = res->end - res->start + 1;

	info->res = request_mem_region(res->start, region_size, "mtd");
	if (!info->res) {
		err = -EBUSY;
		goto no_resource;
	}

	/*
	 * look for CFI based flash parts fitted to this board
	 */
	info->map.size		= region_size;
	info->map.bankwidth	= plat->width;
	info->map.phys		= res->start;
	info->map.virt		= (void __iomem *)CS0_BASE_ADDR_VIRT;
	info->map.name		= priv->map_name;
	info->map.set_vpp	= 0;

	simple_map_init(&info->map);

	/*
	 * Also, the CFI layer automatically works out what size
	 * of chips we have, and does the necessary identification
	 * for us automatically.
	 */
	info->mtd = do_map_probe(plat->map_name, &info->map);
	if (!info->mtd) {
		err = -ENXIO;
		goto no_device;
	}

	info->mtd->owner = THIS_MODULE;

	err = parse_mtd_partitions(info->mtd, probes, &info->parts, 0);
	if (err > 0) {
		PRINT_INFO("use cmdline partitions(%d)\n", err);
		err = add_mtd_partitions(info->mtd, info->parts, err);
		if (err) {
			PRINT_ERR("mtd partition registration failed: %d\n",
				  err);
			default_parts = 1;
		}
	} else {
		default_parts = 1;
	}

	if (default_parts) {
		if (priv && priv->update_partitions)
			priv->update_partitions(&info->map, plat);

		PRINT_INFO("use default partitions(%d)\n", plat->nr_parts);
		err = add_mtd_partitions(info->mtd,
					 plat->parts, plat->nr_parts);
		if (err)
			PRINT_ERR("mtd partition registration failed: %d\n",
				  err);
	}

	if (err == 0)
		platform_set_drvdata(pdev, info);

	/*
	 * If we got an error, free all resources.
	 */
	if (err < 0) {
		if (info->mtd) {
			del_mtd_partitions(info->mtd);
			map_destroy(info->mtd);
		}
		if (info->parts)
			kfree(info->parts);

 no_device:
		release_mem_region(res->start, region_size);
 no_resource:
		if (plat && plat->exit)
			plat->exit();
		kfree(info);
	}
 out:
	return err;
}

static int __devexit
armadillo_mtd_remove(struct platform_device *pdev)
{
	struct armadillo_flash_info *info = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);

	if (info) {
		if (info->mtd) {
			del_mtd_partitions(info->mtd);
			map_destroy(info->mtd);
		}
		if (info->parts)
			kfree(info->parts);

		release_resource(info->res);
		kfree(info->res);

		if (info->plat && info->plat->exit)
			info->plat->exit();

		kfree(info);
	}

	return 0;
}

static struct platform_driver armadillo_mtd_driver = {
	.probe		= armadillo_mtd_probe,
	.remove		= __devexit_p(armadillo_mtd_remove),
	.driver		= {
		.name	= DRIVER_NAME,
	},
};

static int __init
armadillo_mtd_init(void)
{
	int ret;

	DEBUG_FUNC();

	ret = platform_driver_register(&armadillo_mtd_driver);
	if (ret != 0) {
		PRINT_ERR("mtd driver registration failed.\n");
		return -ENODEV;
	}
	return 0;
}

static void __exit
armadillo_mtd_exit(void)
{
	DEBUG_FUNC();
	platform_driver_unregister(&armadillo_mtd_driver);
}

module_init(armadillo_mtd_init);
module_exit(armadillo_mtd_exit);

MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESCRIPTION);
MODULE_LICENSE("GPL");
