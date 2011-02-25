/*
 * TI eQEP driver
 *
 * Author: Mark A. Greer <mgreer@mvista.com>
 *
 * 2008 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
/*
 * The eQEP supports numerous types of encoders/applications, several
 * combinations of when to latch the position count, timers, etc.
 * Too numerous to try to guess in a driver.  So, instead of trying
 * to guess, the driver provides a very low-level interface to the
 * user.  The hardware will be configured based on values that
 * the application provides through sysfs before opening the device.
 * On each interrupt, the driver sends up all of the latched register
 * values along with the values of the interrupt flag and status registers.
 * Therefore, it is up to the application to know how it wants to interface
 * to the external encoder how it wants the eQEP to act.
 *
 * When an interrupt occurs, the values of QFLG, QEPSTS, QPOSLAT, QPOSILAT,
 * QPOSSLAT, QCTMRLAT, and QCPRDLAT registers will be sent up as
 * EV_MSC/MSC_RAW events. An EV_SYN/SYN_REPORT event will be sent up
 * afterwards to delineate one set of interrupt events from another.
 */
#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/io.h>


#define EQEP_QPOSCNT	0x00
#define EQEP_QPOSINIT	0x04
#define EQEP_QPOSMAX	0x08
#define EQEP_QPOSCMP	0x0c
#define EQEP_QPOSILAT	0x10
#define EQEP_QPOSSLAT	0x14
#define EQEP_QPOSLAT	0x18
#define EQEP_QUTMR	0x1c
#define EQEP_QUPRD	0x20
#define EQEP_QWDTMR	0x24
#define EQEP_QWDPRD	0x26
#define EQEP_QDECCTL	0x28
#define		EQEP_QDECCTL_QSRC(x)	(((x) & 0x3) << 14)
#define		EQEP_QDECCTL_SOEN	(1 << 13)
#define		EQEP_QDECCTL_SPSEL	(1 << 12)
#define		EQEP_QDECCTL_XCR	(1 << 11)
#define		EQEP_QDECCTL_SWAP	(1 << 10)
#define		EQEP_QDECCTL_IGATE	(1 << 9)
#define		EQEP_QDECCTL_QAP	(1 << 8)
#define		EQEP_QDECCTL_QBP	(1 << 7)
#define		EQEP_QDECCTL_QIP	(1 << 6)
#define		EQEP_QDECCTL_QSP	(1 << 5)
#define EQEP_QEPCTL	0x2a
#define		EQEP_QEPCTL_PCRM(x)	(((x) & 0x3) << 12)
#define		EQEP_QEPCTL_SEI(x)	(((x) & 0x3) << 10)
#define		EQEP_QEPCTL_IEI(x)	(((x) & 0x3) << 8)
#define		EQEP_QEPCTL_SWI		(1 << 7)
#define		EQEP_QEPCTL_SEL		(1 << 6)
#define		EQEP_QEPCTL_IEL(x)	(((x) & 0x3) << 4)
#define		EQEP_QEPCTL_PHEN	(1 << 3)
#define		EQEP_QEPCTL_QCLM	(1 << 2)
#define		EQEP_QEPCTL_UTE		(1 << 1)
#define		EQEP_QEPCTL_WDE		(1 << 0)
#define EQEP_QCAPCTL	0x2c
#define EQEP_QPOSCTL	0x2e
#define EQEP_QEINT	0x30
#define EQEP_QFLG	0x32
#define EQEP_QCLR	0x34
#define EQEP_QFRC	0x36
#define		EQEP_INT_UTO		(1 << 11) /* Same for all intr regs */
#define		EQEP_INT_IEL		(1 << 10)
#define		EQEP_INT_SEL		(1 << 9)
#define		EQEP_INT_PCM		(1 << 8)
#define		EQEP_INT_PCR		(1 << 7)
#define		EQEP_INT_PCO		(1 << 6)
#define		EQEP_INT_PCU		(1 << 5)
#define		EQEP_INT_WTO		(1 << 4)
#define		EQEP_INT_QDC		(1 << 3)
#define		EQEP_INT_PHE		(1 << 2)
#define		EQEP_INT_PCE		(1 << 1)
#define		EQEP_INT_INT		(1 << 0)
#define		EQEP_INT_ENABLE_ALL	(EQEP_INT_UTO | EQEP_INT_IEL \
			| EQEP_INT_SEL | EQEP_INT_PCM | EQEP_INT_PCR \
			| EQEP_INT_PCO | EQEP_INT_PCU | EQEP_INT_WTO \
			| EQEP_INT_QDC | EQEP_INT_PHE | EQEP_INT_PCE)
#define		EQEP_INT_MASK		(EQEP_INT_ENABLE_ALL | EQEP_INT_INT)
#define EQEP_QEPSTS	0x38
#define		EQEP_QEPSTS_UPEVNT	(1 << 7)
#define		EQEP_QEPSTS_FDF		(1 << 6)
#define		EQEP_QEPSTS_QDF		(1 << 5)
#define		EQEP_QEPSTS_QDLF	(1 << 4)
#define		EQEP_QEPSTS_COEF	(1 << 3)
#define		EQEP_QEPSTS_CDEF	(1 << 2)
#define		EQEP_QEPSTS_FIMF	(1 << 1)
#define		EQEP_QEPSTS_PCEF	(1 << 0)
#define EQEP_QCTMR	0x3a
#define EQEP_QCPRD	0x3c
#define EQEP_QCTMRLAT	0x3e
#define EQEP_QCPRDLAT	0x40
#define EQEP_REVID	0x5c

#define EQEP_INPUT_DEV_PHYS_SIZE	32

enum eqeq_irq_data {
	ID_QPOSLAT,
	ID_QPOSILAT,
	ID_QPOSSLAT,
	ID_QCTMRLAT,
	ID_QCPRDLAT,
	ID_NUM,
};

struct eqep_info {
	struct input_dev	*input_dev;
	struct resource		*res;
	resource_size_t		pbase;
	void __iomem		*vbase;
	size_t			base_size;
	int			irq;
	u16			qeint_save;
	u32			irq_data[ID_NUM];
	char			input_dev_buf[EQEP_INPUT_DEV_PHYS_SIZE];
};

#ifdef CONFIG_SYSFS
struct eqep_name_map {
	char	*attr_name;
	u32	offset;
	u8	reg_size;
};

static struct eqep_name_map eqep_name_map[] = {
	{
		.attr_name = "qposcnt",
		.offset = EQEP_QPOSCNT,
		.reg_size = 32,
	},
	{
		.attr_name = "qposinit",
		.offset = EQEP_QPOSINIT,
		.reg_size = 32,
	},
	{
		.attr_name = "qposmax",
		.offset = EQEP_QPOSMAX,
		.reg_size = 32,
	},
	{
		.attr_name = "qposcmp",
		.offset = EQEP_QPOSCMP,
		.reg_size = 32,
	},
	{
		.attr_name = "qutmr",
		.offset = EQEP_QUTMR,
		.reg_size = 32,
	},
	{
		.attr_name = "quprd",
		.offset = EQEP_QUPRD,
		.reg_size = 32,
	},
	{
		.attr_name = "qwdtmr",
		.offset = EQEP_QWDTMR,
		.reg_size = 16,
	},
	{
		.attr_name = "qwdprd",
		.offset = EQEP_QWDPRD,
		.reg_size = 16,
	},
	{
		.attr_name = "qdecctl",
		.offset = EQEP_QDECCTL,
		.reg_size = 16,
	},
	{
		.attr_name = "qepctl",
		.offset = EQEP_QEPCTL,
		.reg_size = 16,
	},
	{
		.attr_name = "qcapctl",
		.offset = EQEP_QCAPCTL,
		.reg_size = 16,
	},
	{
		.attr_name = "qposctl",
		.offset = EQEP_QPOSCTL,
		.reg_size = 16,
	},
	{
		.attr_name = "qeint",
		.offset = EQEP_QEINT,
		.reg_size = 16,
	},
	{
		.attr_name = "qctmr",
		.offset = EQEP_QCTMR,
		.reg_size = 16,
	},
	{
		.attr_name = "qcprd",
		.offset = EQEP_QCPRD,
		.reg_size = 16,
	},
	{
		.attr_name = "revid",
		.offset = EQEP_REVID,
		.reg_size = 32,
	},
};

static int eqep_find_offset(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(eqep_name_map); i++)
		if (!strcmp(name, eqep_name_map[i].attr_name))
			return i;

	return -1;
}

static ssize_t eqep_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct eqep_info *eip = dev_get_drvdata(dev);
	struct eqep_name_map *nmp;
	int posn;
	u32 v;

	if (eip == NULL)
		goto err_out;

	posn = eqep_find_offset(attr->attr.name);
	if (posn < 0)
		goto err_out;

	nmp = &eqep_name_map[posn];

	if (nmp->reg_size == 16)
		v = ioread16(eip->vbase + nmp->offset);
	else
		v = ioread32(eip->vbase + nmp->offset);

	return sprintf(buf, "0x%08x", v);

err_out:
	return -ENXIO;
}

static ssize_t eqep_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct eqep_info *eip = dev_get_drvdata(dev);
	struct eqep_name_map *nmp;
	int posn, ret;
	unsigned long v;

	if (eip == NULL)
		goto err_out;

	posn = eqep_find_offset(attr->attr.name);
	if (posn < 0)
		goto err_out;

	nmp = &eqep_name_map[posn];

	ret = strict_strtoul(buf, 0, &v);
	if (ret < 0)
		goto err_out;

	if (nmp->reg_size == 16)
		iowrite16(v, eip->vbase + nmp->offset);
	else
		iowrite32(v, eip->vbase + nmp->offset);

	return strlen(buf);

err_out:
	return -ENXIO;
}
static DEVICE_ATTR(qposcnt,	(S_IRUGO | S_IWUSR), eqep_show, eqep_store);
static DEVICE_ATTR(qposinit,	(S_IRUGO | S_IWUSR), eqep_show, eqep_store);
static DEVICE_ATTR(qposmax,	(S_IRUGO | S_IWUSR), eqep_show, eqep_store);
static DEVICE_ATTR(qposcmp,	(S_IRUGO | S_IWUSR), eqep_show, eqep_store);
static DEVICE_ATTR(qutmr,	(S_IRUGO | S_IWUSR), eqep_show, eqep_store);
static DEVICE_ATTR(quprd,	(S_IRUGO | S_IWUSR), eqep_show, eqep_store);
static DEVICE_ATTR(qwdtmr,	(S_IRUGO | S_IWUSR), eqep_show, eqep_store);
static DEVICE_ATTR(qwdprd,	(S_IRUGO | S_IWUSR), eqep_show, eqep_store);
static DEVICE_ATTR(qdecctl,	(S_IRUGO | S_IWUSR), eqep_show, eqep_store);
static DEVICE_ATTR(qepctl,	(S_IRUGO | S_IWUSR), eqep_show, eqep_store);
static DEVICE_ATTR(qcapctl,	(S_IRUGO | S_IWUSR), eqep_show, eqep_store);
static DEVICE_ATTR(qposctl,	(S_IRUGO | S_IWUSR), eqep_show, eqep_store);
static DEVICE_ATTR(qctmr,	(S_IRUGO | S_IWUSR), eqep_show, eqep_store);
static DEVICE_ATTR(qcprd,	(S_IRUGO | S_IWUSR), eqep_show, eqep_store);
static DEVICE_ATTR(revid,	S_IRUGO, eqep_show, NULL);

static ssize_t eqep_qeint_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct eqep_info *eip = dev_get_drvdata(dev);

	if (eip == NULL)
		return -ENXIO;

	return sprintf(buf, "0x%08x", eip->qeint_save);
}

static ssize_t eqep_qeint_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct eqep_info *eip = dev_get_drvdata(dev);
	unsigned long v;
	int ret;

	if (eip == NULL)
		goto err_out;

	ret = strict_strtoul(buf, 0, &v);
	if (ret < 0)
		goto err_out;
	eip->qeint_save = v;

	return eqep_store(dev, attr, buf, count);

err_out:
	return -ENXIO;
}
static DEVICE_ATTR(qeint, (S_IRUGO | S_IWUSR), eqep_qeint_show,
		eqep_qeint_store);

static struct attribute *eqep_device_id_attrs[] = {
	&dev_attr_qposcnt.attr,
	&dev_attr_qposinit.attr,
	&dev_attr_qposmax.attr,
	&dev_attr_qposcmp.attr,
	&dev_attr_qutmr.attr,
	&dev_attr_quprd.attr,
	&dev_attr_qwdtmr.attr,
	&dev_attr_qwdprd.attr,
	&dev_attr_qdecctl.attr,
	&dev_attr_qepctl.attr,
	&dev_attr_qcapctl.attr,
	&dev_attr_qposctl.attr,
	&dev_attr_qctmr.attr,
	&dev_attr_qcprd.attr,
	&dev_attr_revid.attr,
	&dev_attr_qeint.attr,
	NULL
};

static struct attribute_group eqep_id_attr_group = {
	.attrs	= eqep_device_id_attrs,
};
#endif

static void eqep_read_irq_data(struct eqep_info *eip)
{
	u32 *idp = eip->irq_data;

	idp[ID_QPOSLAT] = ioread32(eip->vbase + EQEP_QPOSLAT);
	idp[ID_QPOSILAT] = ioread32(eip->vbase + EQEP_QPOSILAT);
	idp[ID_QPOSSLAT] = ioread32(eip->vbase + EQEP_QPOSSLAT);
	idp[ID_QCTMRLAT] = ioread16(eip->vbase + EQEP_QCTMRLAT);
	idp[ID_QCPRDLAT] = ioread16(eip->vbase + EQEP_QCPRDLAT);
}

/* Send events up to user-space */
static void eqep_report_events(struct eqep_info *eip, u16 qflg, u16 qepsts)
{
	u32 *idp = eip->irq_data;

	input_event(eip->input_dev, EV_MSC, MSC_RAW, qflg);
	input_event(eip->input_dev, EV_MSC, MSC_RAW, qepsts);
	input_event(eip->input_dev, EV_MSC, MSC_RAW, idp[ID_QPOSLAT]);
	input_event(eip->input_dev, EV_MSC, MSC_RAW, idp[ID_QPOSILAT]);
	input_event(eip->input_dev, EV_MSC, MSC_RAW, idp[ID_QPOSSLAT]);
	input_event(eip->input_dev, EV_MSC, MSC_RAW, idp[ID_QCTMRLAT]);
	input_event(eip->input_dev, EV_MSC, MSC_RAW, idp[ID_QCPRDLAT]);

	input_sync(eip->input_dev);
}

static irqreturn_t eqep_intr(int irq, void *dev_id, struct pt_regs *regs)
{
	struct eqep_info *eip = (struct eqep_info *)dev_id;
	irqreturn_t ret = IRQ_NONE;
	u16 qflg, qepsts, v;

	qflg = ioread16(eip->vbase + EQEP_QFLG);
	while (qflg & EQEP_INT_INT) {
		ret = IRQ_HANDLED;

		eqep_read_irq_data(eip);

		/* Clear status/QEPSTS bits */
		qepsts = ioread16(eip->vbase + EQEP_QEPSTS);
		v = qepsts & (EQEP_QEPSTS_UPEVNT | EQEP_QEPSTS_COEF
				| EQEP_QEPSTS_CDEF | EQEP_QEPSTS_FIMF);
		iowrite16(v, eip->vbase + EQEP_QEPSTS);

		/* Clear interrupt/QFLG bits */
		iowrite16(qflg, eip->vbase + EQEP_QCLR);

		eqep_report_events(eip, qflg, qepsts);

		qflg = ioread16(eip->vbase + EQEP_QFLG);
	}

	return ret;
}

static void eqep_hw_reset(struct eqep_info *eip)
{
	u16 v;

	iowrite16(0, eip->vbase + EQEP_QEINT);

	v = ioread16(eip->vbase + EQEP_QEPCTL);
	v &= ~EQEP_QEPCTL_PHEN;
	iowrite16(v, eip->vbase + EQEP_QEPCTL);
	udelay(10);
	iowrite16(v | EQEP_QEPCTL_PHEN, eip->vbase + EQEP_QEPCTL);
}

static void eqep_hw_init(struct eqep_info *eip)
{
	iowrite16(eip->qeint_save, eip->vbase + EQEP_QEINT);
}

static int eqep_open(struct input_dev *input_dev)
{
	struct platform_device *pdev = input_dev->private;
	struct eqep_info *eip;
	int irq, ret = -ENXIO;

	if (pdev == NULL)
		goto err_out;

	eip = platform_get_drvdata(pdev);
	if (eip == NULL)
		goto err_out;

	if (eip->irq == 0) {
		eqep_hw_reset(eip);

		irq = platform_get_irq(pdev, 0);
		if (irq < 0) {
			pr_debug("%s%d: Can't get IRQ resource.\n",
					pdev->name, pdev->id);
			ret = irq;
			goto err_out;
		}

		ret = request_irq(irq, eqep_intr, IRQF_DISABLED, pdev->name,
				eip);
		if (ret) {
			pr_debug("%s: Failed to register handler for irq %d.\n",
					pdev->name, irq);
			ret = -EIO;
			goto err_out;
		}
		eip->irq = irq;

		eqep_hw_init(eip);
	}
	return 0;

err_out:
	return ret;
}

static void eqep_close(struct input_dev *input_dev)
{
	struct platform_device *pdev = input_dev->private;
	struct eqep_info *eip;

	if (pdev == NULL)
		return;

	eip = platform_get_drvdata(pdev);
	if (eip == NULL)
		return;

	if (eip->irq != 0) {
		eqep_hw_reset(eip);

		free_irq(eip->irq, eip);
		eip->irq = 0;
	}
}

static int __devinit eqep_probe(struct platform_device *pdev)
{
	struct input_dev *input_dev;
	struct eqep_info *eip;
	struct resource *res, *mem;
	int ret;

	eip = kmalloc(sizeof(struct eqep_info), GFP_KERNEL);
	if (!eip) {
		pr_debug("%s%d: Can't alloc eip.\n", pdev->name, pdev->id);
		ret = -ENOMEM;
		goto err_out1;
	}
	platform_set_drvdata(pdev, eip);

	memset(eip, 0, sizeof(struct eqep_info));

	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_debug("%s%d: Can't alloc input_dev.\n",
				pdev->name, pdev->id);
		ret = -ENOMEM;
		goto err_out2;
	}

	snprintf(eip->input_dev_buf, EQEP_INPUT_DEV_PHYS_SIZE,
			"eqep/input%d", pdev->id);

	input_dev->private = (void *)pdev;
	input_dev->name = "eqep",
	input_dev->phys = eip->input_dev_buf;
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x001f;
	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0100;
	input_dev->cdev.dev = &pdev->dev;

	input_dev->evbit[0] = BIT(EV_SYN) | BIT(EV_MSC);
	input_dev->mscbit[0] = BIT(MSC_RAW);

	input_dev->open = eqep_open;
	input_dev->close = eqep_close;

	ret = input_register_device(input_dev);
	if (ret) {
		pr_debug("%s%d: Can't register input_dev.\n",
				pdev->name, pdev->id);
		goto err_out3;
	}
	eip->input_dev = input_dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_debug("%s%d: No MEM resource in platform data.\n",
				pdev->name, pdev->id);
		ret = -ENOENT;
		goto err_out4;
	}
	eip->pbase = res->start;
	eip->base_size = res->start - res->start + 1;

	mem = request_mem_region(eip->pbase, eip->base_size, pdev->name);
	if (!mem) {
		pr_debug("%s%d: Can't reserve MEM resource.\n",
				pdev->name, pdev->id);
		ret = -EBUSY;
		goto err_out4;
	}

	eip->vbase = ioremap(eip->pbase, eip->base_size);
	if (eip->vbase == NULL) {
		pr_debug("%s%d: Can't ioremap MEM resource.\n",
				pdev->name, pdev->id);
		ret = -ENOMEM;
		goto err_out5;
	}

	ret = sysfs_create_group(&pdev->dev.kobj, &eqep_id_attr_group);
	if (ret) {
		pr_debug("%s%d: Can't create sysfs group.\n",
				pdev->name, pdev->id);
		goto err_out6;
	}

	dev_info(&pdev->dev, "TI eQEP driver.\n");
	return 0;

err_out6:
	iounmap(eip->vbase);
err_out5:
	release_mem_region(eip->pbase, eip->base_size);
err_out4:
	input_unregister_device(input_dev);
err_out3:
	input_free_device(input_dev);
err_out2:
	platform_set_drvdata(pdev, NULL);
	kfree(eip);
err_out1:
	return ret;
}

static int __devexit eqep_remove(struct platform_device *pdev)
{
	struct eqep_info *eip = platform_get_drvdata(pdev);
	struct input_dev *input_dev;

	if (eip == NULL)
		return -ENXIO;

	input_dev = eip->input_dev;
	if (input_dev == NULL)
		return -ENXIO;

	sysfs_remove_group(&pdev->dev.kobj, &eqep_id_attr_group);
	iounmap(eip->vbase);
	release_mem_region(eip->pbase, eip->base_size);
	input_unregister_device(input_dev);
	input_free_device(input_dev);
	platform_set_drvdata(pdev, NULL);
	kfree(eip);

	return 0;
}

static int eqep_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct eqep_info *eip = platform_get_drvdata(pdev);

	if (eip == NULL)
		return -ENXIO;

	iowrite16(0, eip->vbase + EQEP_QEINT); /* Mask off intrs */
	return 0;
}

static int eqep_resume(struct platform_device *pdev)
{
	struct eqep_info *eip = platform_get_drvdata(pdev);

	if (eip == NULL)
		return -ENXIO;

	iowrite16(eip->qeint_save, eip->vbase + EQEP_QEINT); /* Restore mask */
	return 0;
}

static struct platform_driver eqep_platform_driver = {
	.driver		= {
		.name	= "eqep",
	},
	.probe		= eqep_probe,
	.remove		= __devexit_p(eqep_remove),
	.suspend	= eqep_suspend,
	.resume		= eqep_resume,
};

static int __init eqep_init(void)
{
	return platform_driver_register(&eqep_platform_driver);
}

static void __exit eqep_exit(void)
{
	platform_driver_unregister(&eqep_platform_driver);
}

module_init(eqep_init);
module_exit(eqep_exit);

MODULE_AUTHOR("Mark A. Greer");
MODULE_DESCRIPTION("TI Enhanced Quadrature Encoder Pulse (eQEP) Driver");
MODULE_LICENSE("GPL");
