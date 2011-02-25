/*
 * SPI FLASH driver
 *
 * Author: Steve Chen <schen@mvista.com>
 *
 * 2008 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

#include <linux/uaccess.h>

#include <linux/spi/mtd_spi_flash.h>


void spi_flash_write_enable(struct mtd_info *mtd)
{
	char buf;
	struct mtd_spi_flash_info *priv_dat = mtd->priv;

	/* set write enable */
	buf = MTD_SPI_FLASH_WREN;
	spi_write(priv_dat->spi, &buf, 1);
}

static u8 spi_flash_read_status(struct spi_device *spi)
{
	return spi_w8r8(spi, MTD_SPI_FLASH_RDSTAT);
}

static void spi_flash_wait_complete(struct spi_device *spi)
{
	int i;

	for (i = 0; i < 5000; i++) {
		if ((spi_flash_read_status(spi) & SPI_FLASH_STAT_BUSY) == 0)
			return;
	}
	printk(KERN_WARNING "SPI FLASH operation timeout\n");
}


static int spi_flash_read(struct mtd_info *mtd, loff_t from,
			  size_t count, size_t *retlen, u_char *buf)
{
	int rx_cnt;
	u8 *ptr;
	unsigned int addr;
	unsigned long flags;
	struct spi_transfer x[2];
	struct spi_message msg;
	struct mtd_spi_flash_info *priv_dat = mtd->priv;

	addr = (u32) from;
	*retlen = 0;
	if ((count <= 0) || ((addr + count) > mtd->size))
		return -EINVAL;

	memset(x, 0, sizeof x);
	mutex_lock(&priv_dat->lock);
	x[0].tx_buf = ptr = priv_dat->tx_buffer;
	ptr[0] = MTD_SPI_FLASH_FRD;
	x[0].len = SPI_FLASH_CMD_SIZE;

	/* Handle data return from FLASH */
	x[1].rx_buf = priv_dat->rx_buffer;

	while (count > 0) {
		if (likely(count > SPI_FLASH_BUFFER_SIZE))
			rx_cnt = SPI_FLASH_BUFFER_SIZE;
		else
			rx_cnt = count;

		spi_message_init(&msg);
		/* setup read command */
		ptr[1] = (addr >> 16) & 0xFF;
		ptr[2] = (addr >> 8) & 0xFF;
		ptr[3] = (addr & 0xFF);

		local_irq_save(flags);
		spi_message_add_tail(&x[0], &msg);

		/* read the device */
		x[1].len = rx_cnt + 1;
		spi_message_add_tail(&x[1], &msg);
		local_irq_restore(flags);

		spi_sync(priv_dat->spi, &msg);

		/* skip over the dummy byte because fast read is used */
		memcpy(buf, x[1].rx_buf + 1, rx_cnt);

		buf += rx_cnt;
		count -= rx_cnt;
		addr += rx_cnt;
		*retlen += rx_cnt;
	}
	mutex_unlock(&priv_dat->lock);

	return 0;
}

static int spi_flash_write(struct mtd_info *mtd, loff_t to,
			   size_t count, size_t *retlen,
			   const u_char *buf)
{
	char *ptr;
	int status;
	int tx_cnt;
	int size_limit;
	unsigned int addr;
	struct spi_transfer xfer;
	struct spi_message msg;
	struct mtd_spi_flash_info *priv_dat = mtd->priv;

	addr = (u32) (to);
	*retlen = 0;
	memset(&xfer, 0, sizeof xfer);

	if ((count <= 0) || ((addr + count) > mtd->size))
		return -EINVAL;

	/* take the smaller of buffer size and page size */
	/* Want to make buffer size > than page size for better performance */
	if (priv_dat->page_size <= SPI_FLASH_BUFFER_SIZE)
		size_limit = priv_dat->page_size;
	else
		size_limit = SPI_FLASH_BUFFER_SIZE;

	mutex_lock(&priv_dat->lock);
	while (count > 0) {
		spi_flash_write_enable(mtd);

		spi_message_init(&msg);
		xfer.tx_buf = ptr = priv_dat->tx_buffer;

		/* set the write command */
		ptr[0] = MTD_SPI_FLASH_WRITE;
		ptr[1] = (addr >> 16) & 0xFF;
		ptr[2] = (addr >> 8) & 0xFF;
		ptr[3] = (addr & 0xFF);

		/* figure out the max data able to transfer */
		tx_cnt = size_limit - (addr & (priv_dat->page_size - 1));
		if (count < tx_cnt)
			tx_cnt = count;

		/* copy over the write data */
		ptr = &ptr[SPI_FLASH_CMD_SIZE];
		memcpy(ptr, buf, tx_cnt);
		xfer.len = SPI_FLASH_CMD_SIZE + tx_cnt;

		spi_message_add_tail(&xfer, &msg);
		status = spi_sync(priv_dat->spi, &msg);

		count -= tx_cnt;
		buf += tx_cnt;
		addr += tx_cnt;
		*retlen += tx_cnt;

		spi_flash_wait_complete(priv_dat->spi);
	}
	mutex_unlock(&priv_dat->lock);

	return (0);
}

static int spi_flash_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	u8 op_code;
	char *ptr;
	struct spi_transfer x;
	struct spi_message msg;
	struct mtd_spi_flash_info *priv_dat = mtd->priv;

	if (((instr->addr + instr->len) > mtd->size) ||
	    ((instr->addr % priv_dat->sector_erase_size) != 0) ||
	    ((instr->len % priv_dat->sector_erase_size) != 0))
		return -EINVAL;

	memset(&x, 0, sizeof(x));
	x.tx_buf = ptr = priv_dat->tx_buffer;

	mutex_lock(&priv_dat->lock);
	while (instr->len > 0) {
		spi_flash_write_enable(mtd);
		spi_message_init(&msg);

		ptr[1] = (u8) ((instr->addr >> 16) & 0xFF);
		ptr[2] = (u8) ((instr->addr >> 8) & 0xFF);
		ptr[3] = (u8) (instr->addr & 0xFF);
		x.len = 4;

		if (instr->len < priv_dat->block_erase_size) {
			op_code = MTD_SPI_FLASH_SECERA;
			instr->addr += priv_dat->sector_erase_size;
			instr->len -= priv_dat->sector_erase_size;
		} else if (instr->len < mtd->size) {
			op_code = MTD_SPI_FLASH_BKERA;
			instr->addr += priv_dat->block_erase_size;
			instr->len -= priv_dat->block_erase_size;
		} else {
			op_code = MTD_SPI_FLASH_CHPERA;
			instr->len = 0;
			x.len = 1;
		}
		ptr[0] = op_code;

		spi_message_add_tail(&x, &msg);
		spi_sync(priv_dat->spi, &msg);
		spi_flash_wait_complete(priv_dat->spi);
	}
	mutex_unlock(&priv_dat->lock);

	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);

	return 0;
}

static int spi_flash_read_mfc_id(struct spi_device *spi, char *mfr, char *id)
{
	unsigned long flags;
	char tx_buf[1];
	char rx_buf[5];
	struct spi_transfer x[2];
	struct spi_message msg;

	memset(x, 0, sizeof x);
	x[0].tx_buf = tx_buf;
	tx_buf[0] = MTD_SPI_FLASH_MFRID;
	x[0].len = 1;

	/* Send command to read manufacture and device ID */
	spi_message_init(&msg);

	local_irq_save(flags);
	spi_message_add_tail(&x[0], &msg);

	/* prepare the receive buffer */
	x[1].rx_buf = rx_buf;
	x[1].len = 5;
	spi_message_add_tail(&x[1], &msg);
	local_irq_restore(flags);

	spi_sync(spi, &msg);
	spi_flash_wait_complete(spi);

	/*
	 * Receive buffer format is
	 * Byte0  Byte1  Byte2  Byte3  Byte4
	 * dummy  dummy  0x0    MFC_ID DEV_ID
	 */
	*mfr = rx_buf[3];
	*id = rx_buf[4];

	return 0;
}

static int spi_flash_init_device(struct spi_device *spi, u32 dev_size,
				u32 page_size, u32 sec_size, u32 blk_size)
{
	struct mtd_spi_flash_info *info;
	static struct mtd_info *mtd;
	int ret;

	info = spi->dev.platform_data;
	info->spi = spi;
	info->spi_data = spi_master_get_devdata(spi->master);
	info->page_size = page_size;
	info->sector_erase_size = sec_size;
	info->block_erase_size = blk_size;
	mutex_init(&info->lock);

	mtd = &info->mtd;
	memset(mtd, 0, sizeof(struct mtd_info));

	mtd->priv = info;
	mtd->size = dev_size;
	mtd->flags = MTD_CAP_NORFLASH;
	mtd->name = info->name;
	mtd->erasesize = sec_size;
	mtd->writesize = 1;

	mtd->type = MTD_DATAFLASH;
	mtd->read = spi_flash_read;
	mtd->write = spi_flash_write;
	mtd->erase = spi_flash_erase;

#ifdef CONFIG_MTD_PARTITIONS
	if (info->nr_parts)
		ret = add_mtd_partitions(mtd, info->parts, info->nr_parts);
	else
		ret = add_mtd_device(mtd);
#else
	ret = add_mtd_device(mtd);
#endif

	if (ret < 0)
		pr_info("SPI FLASH device register failed\n");

	return ret;

}

static int spi_flash_init_winbond(struct spi_device *spi, char id)
{
	int ret;

	switch (id) {
	case SPI_FLASH_W25X16:
		ret = spi_flash_init_device(spi, 0x200000, 0x100, 0x1000,
					0x10000);
		break;
	case SPI_FLASH_W25X32:
		ret = spi_flash_init_device(spi, 0x400000, 0x100, 0x1000,
					0x10000);
		break;
	case SPI_FLASH_W25X64:
		ret = spi_flash_init_device(spi, 0x800000, 0x100, 0x1000,
					0x10000);
		break;
	default:
		printk(KERN_WARNING "Winbond SPI FLASH %x not supported", id);
		ret = -1;
	}
	return ret;
}

static int __devinit spi_flash_probe(struct spi_device *spi)
{
	char mfr, id;
	int ret;

	spi_flash_read_mfc_id(spi, &mfr, &id);

	switch (mfr) {
	case SPI_FLASH_MFR_WINBOND:
		ret = spi_flash_init_winbond(spi, id);
		break;
	default:
		printk(KERN_WARNING "SPI FLASH Manufacture code %x not "
			"supported", mfr);
		ret = -1;
	}
	return ret;
}

static int __devexit spi_flash_remove(struct spi_device *spi)
{
	int ret;
	struct mtd_info *mtd;
	struct mtd_spi_flash_info *info;

	info = spi->dev.platform_data;
	mtd = &info->mtd;

#ifdef CONFIG_MTD_PARTITIONS
	if (info->nr_parts)
		ret = del_mtd_partitions(mtd);
	else
		ret = del_mtd_device(mtd);
#else
	ret = del_mtd_device(mtd);
#endif

	return ret;
}

static struct spi_driver spi_flash_driver = {
	.driver = {
		.name = MTD_SPI_FLASH_NAME,
		.bus = &spi_bus_type,
		.owner = THIS_MODULE,
	},
	.probe = spi_flash_probe,
	.remove = spi_flash_remove,
};

static int __init spi_flash_init(void)
{
	return spi_register_driver(&spi_flash_driver);
}

module_init(spi_flash_init);

static void __exit spi_flash_exit(void)
{
	spi_unregister_driver(&spi_flash_driver);
}

module_exit(spi_flash_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Steve Chen");
MODULE_DESCRIPTION("SPI FLASH driver");
