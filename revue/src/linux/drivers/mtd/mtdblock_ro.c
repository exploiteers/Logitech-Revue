/*
 * $Id: mtdblock_ro.c,v 1.19 2004/11/16 18:28:59 dwmw2 Exp $
 *
 * (C) 2003 David Woodhouse <dwmw2@infradead.org>
 *
 * Simple read-only (writable only for RAM) mtdblock driver
 *
 * Bad-block handling by Eugene Surovegin <es@google.com>
 * Copyright (c) 2010 Google, Inc
 *
 * Limitation: bad blocks are scanned on initialization only,
 * we don't support new bad-blocks being added dynamically after that,
 * which shouldn't be a problem for a use-case we are interesed in,
 * namely using MTD block device for squashfs/cramfs r/o mounts.
 */

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/blktrans.h>

struct mtdblock_priv {
	struct mtd_blktrans_dev dev;

	/* number of 512-byte sectors in erase block */
	unsigned int sectors_per_block;

	/* logical to physical erase block map */
	u16 *bb_map;
};

#define dev2priv(dev)		container_of(dev, struct mtdblock_priv, dev)

#if defined(CONFIG_MTD_BLOCK_RO_SKIP_BAD_BLOCKS)
static int mtdblock_bb_init(struct mtdblock_priv *priv)
{
	struct mtd_info *mtd = priv->dev.mtd;
	int i, total_blocks, good_blocks = 0;

	/* basic sanity checks */
	if (mtd->type != MTD_NANDFLASH || mtd->numeraseregions || !mtd->size
	    || !mtd->erasesize)
		return 0;

	total_blocks = mtd->size / mtd->erasesize;

	/* MTD layer currenly cannot support huge devices anyways,
	 * so we use u16 for bb_map to save some space.
	 */
	if (unlikely(total_blocks > 0xffff))
		return -EINVAL;

	priv->bb_map = kmalloc(total_blocks * sizeof(u16), GFP_KERNEL);
	if (unlikely(!priv->bb_map)) {
		printk(KERN_ERR
		       "mtdblock: failed to allocate %d bad-block map\n",
			total_blocks);
		return -ENOMEM;
	}

	for (i = 0; i < total_blocks; ++i) {
		if (mtd->block_isbad(mtd, i * mtd->erasesize))
			continue;
		priv->bb_map[good_blocks++] = (u16)i;
	}

	priv->sectors_per_block = mtd->erasesize / 512;
	if (good_blocks != total_blocks) {
		/* fill the rest of the map with something deterministic */
		for (i = good_blocks; i < total_blocks; ++i)
			priv->bb_map[i] = 0xffff;

		priv->dev.size = good_blocks * priv->sectors_per_block;
		printk(KERN_INFO "mtdblock%d: %d bad block(s) found\n",
		       priv->dev.devnum, total_blocks - good_blocks);
	}

	return 0;
}

static int mtdblock_open(struct mtd_blktrans_dev *dev)
{
	struct mtdblock_priv *priv = dev2priv(dev);
	struct mtd_info *mtd = dev->mtd;
	int i, total_blocks, good_blocks = 0;

	if (!priv->bb_map)
		return 0;

	/* Make sure that no new bad blocks were added behind our back */
	total_blocks = mtd->size / mtd->erasesize;

	for (i = 0; i < total_blocks; ++i) {
		if (mtd->block_isbad(mtd, i * mtd->erasesize))
			continue;
		if (unlikely(priv->bb_map[good_blocks++] != (u16)i))
			goto mismatch;
	}

	if (good_blocks != total_blocks) {
		for (i = good_blocks; i < total_blocks; ++i)
			if (unlikely(priv->bb_map[i] != 0xffff))
				goto mismatch;
	}

	return 0;

    mismatch:
	printk(KERN_ERR "mtdblock%d: bad block layout has changed\n",
	       dev->devnum);
	return -ENODEV;
}

static inline unsigned long mtdblock_bb_remap(struct mtd_blktrans_dev *dev,
				       	      unsigned long sector)
{
	struct mtdblock_priv *priv = dev2priv(dev);
	if (priv->bb_map)
		sector = priv->sectors_per_block *
			 priv->bb_map[sector / priv->sectors_per_block] +
			 sector % priv->sectors_per_block;

	return sector;
}
#else
static int mtdblock_open(struct mtd_blktrans_dev *dev) { return 0; }
static inline int mtdblock_bb_init(struct mtdblock_priv *priv) { return 0; }
static inline unsigned long mtdblock_bb_remap(struct mtd_blktrans_dev *dev,
				    	      unsigned long sector)
{
	return sector;
}
#endif /* MTD_BLOCK_RO_SKIP_BAD_BLOCKS */

static int mtdblock_readsect(struct mtd_blktrans_dev *dev,
			     unsigned long block, char *buf)
{
	size_t retlen;

	block = mtdblock_bb_remap(dev, block);
	if (dev->mtd->read(dev->mtd, (block * 512), 512, &retlen, buf))
		return 1;
	return 0;
}

static int mtdblock_writesect(struct mtd_blktrans_dev *dev,
			      unsigned long block, char *buf)
{
	size_t retlen;

	block = mtdblock_bb_remap(dev, block);
	if (dev->mtd->write(dev->mtd, (block * 512), 512, &retlen, buf))
		return 1;
	return 0;
}

static void mtdblock_add_mtd(struct mtd_blktrans_ops *tr, struct mtd_info *mtd)
{
	struct mtdblock_priv *priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	struct mtd_blktrans_dev *dev;

	if (!priv)
		return;

	dev = &priv->dev;
	dev->mtd = mtd;
	dev->devnum = mtd->index;

	dev->size = mtd->size >> 9;
	dev->tr = tr;
	if (!(mtd->flags & MTD_WRITEABLE))
		dev->readonly = 1;

	if (unlikely(mtdblock_bb_init(priv))) {
		kfree(priv);
		return;
	}

	add_mtd_blktrans_dev(dev);
}

static void mtdblock_remove_dev(struct mtd_blktrans_dev *dev)
{
	struct mtdblock_priv *priv = dev2priv(dev);

	del_mtd_blktrans_dev(dev);
	kfree(priv->bb_map);
	kfree(priv);
}

static struct mtd_blktrans_ops mtdblock_tr = {
	.name		= "mtdblock",
	.major		= 31,
	.part_bits	= 0,
	.blksize 	= 512,
	.open		= mtdblock_open,
	.readsect	= mtdblock_readsect,
	.writesect	= mtdblock_writesect,
	.add_mtd	= mtdblock_add_mtd,
	.remove_dev	= mtdblock_remove_dev,
	.owner		= THIS_MODULE,
};

static int __init mtdblock_init(void)
{
	return register_mtd_blktrans(&mtdblock_tr);
}

static void __exit mtdblock_exit(void)
{
	deregister_mtd_blktrans(&mtdblock_tr);
}

module_init(mtdblock_init);
module_exit(mtdblock_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("David Woodhouse <dwmw2@infradead.org>");
MODULE_DESCRIPTION("Simple read-only block device emulation access to MTD devices");
