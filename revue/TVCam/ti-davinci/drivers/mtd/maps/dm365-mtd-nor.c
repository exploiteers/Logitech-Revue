#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <linux/config.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/flash.h>
#include <asm/arch/irqs.h>
#include <asm/arch/hardware.h>
#include <asm/arch/edma.h>
#include <asm/arch/mux.h>
#include <asm/arch/gpio.h>


#define DM365_NOR_BASE 		0x02000000 
#define NOR_WINDOW_SIZE     (SZ_16M)
#define NOR_WINDOW_ADDR     DM365_NOR_BASE
#define NOR_BUSWIDTH        2 /* NOR flash */

#ifdef CONFIG_MTD_PARTITIONS
static struct mtd_partition dm365_mtd_nor_parts[] = {
	{ name: "nor-bootloader", offset: 0x000000, size: 0x00060000, },
	{ name: "nor-params", offset: 0x00060000, size: 0x20000, },
	{ name: "nor-kernel", offset: 0x00080000, size: 0x200000, },
 	{ name: "nor-filesys", offset: 0x00280000, size: (NOR_WINDOW_SIZE - 0x00280000), },  
	{ name: NULL, },
};
#endif /* CONFIG_MTD_PARTITIONS */

static struct map_info dm365_mtd_nor_map = {
	name: "DM365 NOR-MTD",
	size: NOR_WINDOW_SIZE,
	phys: NOR_WINDOW_ADDR,
	bankwidth: NOR_BUSWIDTH,
};
static struct mtd_info *dm365_mtd_nor;

static int __init dm365_mtd_nor_map_init(void)
{
	uint window_addr = 0, window_size = 0;
	size_t size;
	int ret = 0;
#ifdef CONFIG_MTD_PARTITIONS
	int i;
#endif


#if 0
	{
		unsigned int val;

		val = davinci_readl(PINMUX4);
		val &= 0xf0ffffff;
		val |= 0x0a000000;
	}
#endif

	dm365_mtd_nor_map.map_priv_1 = 0;
	dm365_mtd_nor_map.map_priv_2 = 0;
	window_addr = dm365_mtd_nor_map.phys;
	window_size = dm365_mtd_nor_map.size;

	dm365_mtd_nor_map.virt = ioremap(window_addr, window_size);

	if (!dm365_mtd_nor_map.virt) {
		printk(KERN_ERR "init_dm365_map: ioremap failed\n");
		ret = -EIO;
		goto probe_nor_fail;
	}

	if (!(dm365_mtd_nor = do_map_probe("cfi_probe", &dm365_mtd_nor_map))) {
		if (!(dm365_mtd_nor = do_map_probe("jedec_probe", &dm365_mtd_nor_map))) {
			printk(KERN_ERR "init_dm365_mtd_map: CFI and JEDEC probe failed\n");
			ret = -ENXIO;
			goto probe_nor_fail;
		}
	}

	dm365_mtd_nor->owner = THIS_MODULE;
	size = dm365_mtd_nor->size;
	printk(KERN_NOTICE "DM365 NOR-MTD device: 0x%x at 0x%x\n", size, window_addr);

#ifdef CONFIG_MTD_PARTITIONS
	for (i = 0; dm365_mtd_nor_parts[i].name; i++);
	ret = add_mtd_partitions(dm365_mtd_nor, dm365_mtd_nor_parts, i);
	if (!ret) goto probe_nor_end;

	printk(KERN_ERR "NOR-MTD: add_mtd_partitions failed\n");
#else
	ret = add_mtd_device(dm365_mtd_nor);
	if (!ret) goto probe_nor_end;

	printk(KERN_ERR "NOR-MTD: add_mtd failed\n");
#endif

probe_nor_fail:
	if (dm365_mtd_nor)
		map_destroy(dm365_mtd_nor);
	if (dm365_mtd_nor_map.virt)
		iounmap((void *) dm365_mtd_nor_map.virt);
	dm365_mtd_nor_map.virt = 0;

probe_nor_end:

	return ret;
}

static void __exit dm365_mtd_nor_map_exit(void)
{
#ifdef CONFIG_MTD_PARTITIONS
	del_mtd_partitions(dm365_mtd_nor);
#else /* CONFIG_MTD_PARTITIONS */
	del_mtd_device(dm365_mtd_nor);
#endif
	map_destroy(dm365_mtd_nor);
	iounmap((void *)dm365_mtd_nor_map.virt);
	dm365_mtd_nor_map.virt = 0;
}

module_init(dm365_mtd_nor_map_init);
module_exit(dm365_mtd_nor_map_exit);

MODULE_LICENSE("GPL");
