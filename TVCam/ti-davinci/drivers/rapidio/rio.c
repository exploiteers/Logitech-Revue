/*
 * RapidIO interconnect services
 * (RapidIO Interconnect Specification, http://www.rapidio.org)
 *
 * Copyright 2005 MontaVista Software, Inc.
 * Matt Porter <mporter@kernel.crashing.org>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/types.h>
#include <linux/kernel.h>

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/rio.h>
#include <linux/rio_drv.h>
#include <linux/rio_ids.h>
#include <linux/rio_regs.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/dma-mapping.h>
#include <linux/hardirq.h>

#include "rio.h"

#define ERR(fmt, arg...) \
	printk(KERN_ERR "%s:%s: " fmt,  __FILE__, __FUNCTION__, ## arg)

static LIST_HEAD(rio_mports);
static LIST_HEAD(rio_inb_mems);
static LIST_HEAD(rio_outb_mems);

static DEFINE_SPINLOCK(rio_config_lock);

/**
 * rio_local_get_device_id - Get the base/extended device id for a port
 * @port: RIO master port from which to get the deviceid
 *
 * Reads the base/extended device id from the local device
 * implementing the master port. Returns the 8/16-bit device
 * id.
 */
u16 rio_local_get_device_id(struct rio_mport *port)
{
	u32 result;

	rio_local_read_config_32(port, RIO_DID_CSR, &result);

	return (RIO_GET_DID(port->sys_size, result));
}

/**
 * rio_request_inb_mbox - request inbound mailbox service
 * @mport: RIO master port from which to allocate the mailbox resource
 * @dev_id: Device specific pointer to pass on event
 * @mbox: Mailbox number to claim
 * @entries: Number of entries in inbound mailbox queue
 * @minb: Callback to execute when inbound message is received
 *
 * Requests ownership of an inbound mailbox resource and binds
 * a callback function to the resource. Returns %0 on success.
 */
int rio_request_inb_mbox(struct rio_mport *mport,
			 void *dev_id,
			 int mbox,
			 int entries,
			 void (*minb) (struct rio_mport * mport, void *dev_id, int mbox,
				       int slot))
{
	int rc = 0;

	struct resource *res = kmalloc(sizeof(struct resource), GFP_KERNEL);

	if (res) {
		rio_init_mbox_res(res, mbox, mbox);

		/* Make sure this mailbox isn't in use */
		if ((rc =
		     request_resource(&mport->riores[RIO_INB_MBOX_RESOURCE],
				      res)) < 0) {
			kfree(res);
			goto out;
		}

		mport->inb_msg[mbox].res = res;

		/* Hook the inbound message callback */
		mport->inb_msg[mbox].mcback = minb;

		rc = rio_open_inb_mbox(mport, dev_id, mbox, entries);
	} else
		rc = -ENOMEM;

      out:
	return rc;
}

/**
 * rio_release_inb_mbox - release inbound mailbox message service
 * @mport: RIO master port from which to release the mailbox resource
 * @mbox: Mailbox number to release
 *
 * Releases ownership of an inbound mailbox resource. Returns 0
 * if the request has been satisfied.
 */
int rio_release_inb_mbox(struct rio_mport *mport, int mbox)
{
	rio_close_inb_mbox(mport, mbox);

	/* Release the mailbox resource */
	return release_resource(mport->inb_msg[mbox].res);
}

/**
 * rio_request_outb_mbox - request outbound mailbox service
 * @mport: RIO master port from which to allocate the mailbox resource
 * @dev_id: Device specific pointer to pass on event
 * @mbox: Mailbox number to claim
 * @entries: Number of entries in outbound mailbox queue
 * @moutb: Callback to execute when outbound message is sent
 *
 * Requests ownership of an outbound mailbox resource and binds
 * a callback function to the resource. Returns 0 on success.
 */
int rio_request_outb_mbox(struct rio_mport *mport,
			  void *dev_id,
			  int mbox,
			  int entries,
			  void (*moutb) (struct rio_mport * mport, void *dev_id, int mbox, int slot))
{
	int rc = 0;

	struct resource *res = kmalloc(sizeof(struct resource), GFP_KERNEL);

	if (res) {
		rio_init_mbox_res(res, mbox, mbox);

		/* Make sure this outbound mailbox isn't in use */
		if ((rc =
		     request_resource(&mport->riores[RIO_OUTB_MBOX_RESOURCE],
				      res)) < 0) {
			kfree(res);
			goto out;
		}

		mport->outb_msg[mbox].res = res;

		/* Hook the inbound message callback */
		mport->outb_msg[mbox].mcback = moutb;

		rc = rio_open_outb_mbox(mport, dev_id, mbox, entries);
	} else
		rc = -ENOMEM;

      out:
	return rc;
}

/**
 * rio_release_outb_mbox - release outbound mailbox message service
 * @mport: RIO master port from which to release the mailbox resource
 * @mbox: Mailbox number to release
 *
 * Releases ownership of an inbound mailbox resource. Returns 0
 * if the request has been satisfied.
 */
int rio_release_outb_mbox(struct rio_mport *mport, int mbox)
{
	rio_close_outb_mbox(mport, mbox);

	/* Release the mailbox resource */
	return release_resource(mport->outb_msg[mbox].res);
}

/**
 * rio_setup_inb_dbell - bind inbound doorbell callback
 * @mport: RIO master port to bind the doorbell callback
 * @dev_id: Device specific pointer to pass on event
 * @res: Doorbell message resource
 * @dinb: Callback to execute when doorbell is received
 *
 * Adds a doorbell resource/callback pair into a port's
 * doorbell event list. Returns 0 if the request has been
 * satisfied.
 */
static int
rio_setup_inb_dbell(struct rio_mport *mport, void *dev_id, struct resource *res,
		    void (*dinb) (struct rio_mport * mport, void *dev_id, u16 src, u16 dst,
				  u16 info))
{
	int rc = 0;
	struct rio_dbell *dbell;

	if (!(dbell = kmalloc(sizeof(struct rio_dbell), GFP_KERNEL))) {
		rc = -ENOMEM;
		goto out;
	}

	dbell->res = res;
	dbell->dinb = dinb;
	dbell->dev_id = dev_id;

	list_add_tail(&dbell->node, &mport->dbells);

      out:
	return rc;
}

/**
 * rio_request_inb_dbell - request inbound doorbell message service
 * @mport: RIO master port from which to allocate the doorbell resource
 * @dev_id: Device specific pointer to pass on event
 * @start: Doorbell info range start
 * @end: Doorbell info range end
 * @dinb: Callback to execute when doorbell is received
 *
 * Requests ownership of an inbound doorbell resource and binds
 * a callback function to the resource. Returns 0 if the request
 * has been satisfied.
 */
int rio_request_inb_dbell(struct rio_mport *mport,
			  void *dev_id,
			  u16 start,
			  u16 end,
			  void (*dinb) (struct rio_mport * mport, void *dev_id, u16 src,
					u16 dst, u16 info))
{
	int rc = 0;

	struct resource *res = kmalloc(sizeof(struct resource), GFP_KERNEL);

	if (res) {
		rio_init_dbell_res(res, start, end);

		/* Make sure these doorbells aren't in use */
		if ((rc =
		     request_resource(&mport->riores[RIO_DOORBELL_RESOURCE],
				      res)) < 0) {
			kfree(res);
			goto out;
		}

		/* Hook the doorbell callback */
		rc = rio_setup_inb_dbell(mport, dev_id, res, dinb);
	} else
		rc = -ENOMEM;

      out:
	return rc;
}

/**
 * rio_release_inb_dbell - release inbound doorbell message service
 * @mport: RIO master port from which to release the doorbell resource
 * @start: Doorbell info range start
 * @end: Doorbell info range end
 *
 * Releases ownership of an inbound doorbell resource and removes
 * callback from the doorbell event list. Returns 0 if the request
 * has been satisfied.
 */
int rio_release_inb_dbell(struct rio_mport *mport, u16 start, u16 end)
{
	int rc = 0, found = 0;
	struct rio_dbell *dbell;

	list_for_each_entry(dbell, &mport->dbells, node) {
		if ((dbell->res->start == start) && (dbell->res->end == end)) {
			found = 1;
			break;
		}
	}

	/* If we can't find an exact match, fail */
	if (!found) {
		rc = -EINVAL;
		goto out;
	}

	/* Delete from list */
	list_del(&dbell->node);

	/* Release the doorbell resource */
	rc = release_resource(dbell->res);

	/* Free the doorbell event */
	kfree(dbell);

      out:
	return rc;
}

/**
 * rio_request_outb_dbell - request outbound doorbell message range
 * @rdev: RIO device from which to allocate the doorbell resource
 * @start: Doorbell message range start
 * @end: Doorbell message range end
 *
 * Requests ownership of a doorbell message range. Returns a resource
 * if the request has been satisfied or %NULL on failure.
 */
struct resource *rio_request_outb_dbell(struct rio_dev *rdev, u16 start,
					u16 end)
{
	struct resource *res = kmalloc(sizeof(struct resource), GFP_KERNEL);

	if (res) {
		rio_init_dbell_res(res, start, end);

		/* Make sure these doorbells aren't in use */
		if (request_resource(&rdev->riores[RIO_DOORBELL_RESOURCE], res)
		    < 0) {
			kfree(res);
			res = NULL;
		}
	}

	return res;
}

/**
 * rio_release_outb_dbell - release outbound doorbell message range
 * @rdev: RIO device from which to release the doorbell resource
 * @res: Doorbell resource to be freed
 *
 * Releases ownership of a doorbell message range. Returns 0 if the
 * request has been satisfied.
 */
int rio_release_outb_dbell(struct rio_dev *rdev, struct resource *res)
{
	int rc = release_resource(res);

	kfree(res);

	return rc;
}

/**
 * rio_request_io_region -- request resource in RapidIO IO region
 * @mport: Master port
 * @devid: Device specific pointer to pass
 * @start: IO resource start address
 * @size: IO resource size
 * @name: Resource name
 * @flags: Flag for resource
 * @res: Return resource which has been allocated. If res == NULL,
 *       the function will alloc the memory for return resource.
 *
 * Return: The resource which has been allocated.
 */
struct resource *rio_request_io_region(struct rio_mport *mport, void *devid,
		resource_size_t start, resource_size_t size,
		const char *name, unsigned long flags,
		struct resource *res)
{
	if (!res && !(res = kmalloc(sizeof(struct resource), GFP_KERNEL))) {
		ERR("No free memory for res alloc!\n");
		goto err;
	}
	memset(res, 0, sizeof(struct resource));
	size = (size < 0x1000) ? 0x1000 : 1 << (__ilog2(size - 1) + 1);

	/* if start == 0 then auto locate the start address */
	if (!start) {
		if (allocate_resource(&mport->iores, res, size,
				mport->iores.start, mport->iores.end,
				size, NULL, NULL) < 0) {
			ERR("allocte resource error!\n");
			goto err;
		}
		res->name = name;
		res->flags = flags;
	} else {
		rio_init_io_res(res, start, start + size - 1, name, flags);
		if (request_resource(&mport->iores, res) < 0) {
			ERR("Can't get SRIO IO resource!\n");
			goto err;
		}
	}
	return res;

err:
	if (res)
		kfree(res);
	return NULL;
}
EXPORT_SYMBOL_GPL(rio_request_io_region);

/**
 * rio_map_inb_region -- Mapping inbound memory region.
 * @mport: Master port.
 * @mem: Memory struction for mapping.
 * @rflags: Flags for mapping.
 *
 * Return: 0 -- Success.
 *
 * This function will create the mapping from the mem->riores to mem->iores.
 */
int rio_map_inb_region(struct rio_mport *mport, struct rio_mem *mem, u32 rflags)
{
	int rc = 0;
	unsigned long flags;

	if (!mport->mops)
		return -1;
	spin_lock_irqsave(&rio_config_lock, flags);
	rc = mport->mops->map_inb(mport, mem->iores.start, mem->riores.start, mem->size, rflags);
	spin_unlock_irqrestore(&rio_config_lock, flags);
	return rc;
}

/**
 * rio_map_outb_region -- Mapping outbound memory region.
 * @mport: Master port.
 * @tid: Target RapidIO device id.
 * @mem: Memory struction for mapping.
 * @rflags: Flags for mapping.
 *
 * Return: 0 -- Success.
 *
 * This function will create the mapping from the mem->iores to mem->riores.
 */
int rio_map_outb_region(struct rio_mport *mport, u16 tid,
		struct rio_mem *mem, u32 rflags)
{
	int rc = 0;
	unsigned long flags;

	if (!mport->mops)
		return -1;
	spin_lock_irqsave(&rio_config_lock, flags);
	rc = mport->mops->map_outb(mport, mem->iores.start, mem->riores.start, mem->size, tid, rflags);
	spin_unlock_irqrestore(&rio_config_lock, flags);
	return rc;
}

/**
 * rio_unmap_inb_region -- Unmap the inbound memory region
 * @mport: Master port
 * @mem: Memory struction for unmapping.
 */
void rio_unmap_inb_region(struct rio_mport *mport, struct rio_mem *mem)
{
	unsigned long flags;
	if (!mport->mops)
		return;
	spin_lock_irqsave(&rio_config_lock, flags);
	mport->mops->unmap_inb(mport, mem->iores.start);
	spin_unlock_irqrestore(&rio_config_lock, flags);
}

/**
 * rio_unmap_outb_region -- Unmap the outbound memory region
 * @mport: Master port
 * @mem: Memory struction for unmapping.
 */
void rio_unmap_outb_region(struct rio_mport *mport, struct rio_mem *mem)
{
	unsigned long flags;
	if (!mport->mops)
		return;
	spin_lock_irqsave(&rio_config_lock, flags);
	mport->mops->unmap_outb(mport, mem->iores.start);
	spin_unlock_irqrestore(&rio_config_lock, flags);
}

/**
 * rio_release_inb_region -- Release the inbound region resource.
 * @mport: Master port
 * @mem: Inbound region descriptor
 *
 * Return 0 is successed.
 */
int rio_release_inb_region(struct rio_mport *mport, struct rio_mem *mem)
{
	int rc = 0;
	if (!mem)
		return rc;
	rio_unmap_inb_region(mport, mem);
	if (mem->virt)
		dma_free_coherent(NULL, mem->size, mem->virt, mem->iores.start);

	if (mem->iores.parent)
		rc = release_resource(&mem->iores);
	if (mem->riores.parent && !rc)
		rc = release_resource(&mem->riores);

	if (mem->node.prev)
		list_del(&mem->node);

	kfree(mem);

	return rc;
}

/**
 * rio_request_inb_region -- Request inbound memory region
 * @mport: Master port
 * @dev_id: Device specific pointer to pass
 * @size: The request memory windows size
 * @name: The region name
 * @owner: The region owner driver id
 *
 * Retrun: The rio_mem struction for inbound memory descriptor.
 *
 * This function is used for request RapidIO space inbound region. If the size
 * less than 4096 or not aligned to 2^N, it will be adjusted. The function will
 * alloc a block of local DMA memory of the size for inbound region target and
 * request a RapidIO region for inbound region source. Then the inbound region
 * will be claimed in RapidIO space and the local DMA memory will be added to
 * local inbound memory list. The rio_mem with the inbound relationship will
 * be returned.
 */
struct rio_mem *rio_request_inb_region(struct rio_mport *mport, void *dev_id,
		resource_size_t size, const char *name, u32 owner)
{
	struct rio_mem *rmem = NULL;
	int ret;

	rmem = kzalloc(sizeof(struct rio_mem), GFP_KERNEL);
	if (!rmem)
		goto err;

	/* Align the size to 2^N */
	size = (size < 0x1000) ? 0x1000 : 1 << (__ilog2(size - 1) + 1);

	/* Alloc the RapidIO space */
	ret = rio_space_request(mport, size, &rmem->riores);
	if (ret) {
		printk(KERN_ERR "RIO space request error! ret = %d\n", ret);
		goto err;
	}

	rmem->riores.name = name;
	rmem->size = rmem->riores.end - rmem->riores.start + 1;

	/* Initialize inbound memory */
	if (!(rmem->virt = dma_alloc_coherent(NULL, rmem->size,
				&rmem->iores.start, GFP_KERNEL))) {
		ERR("Inbound memory alloc error\n");
		goto err;
	}
	rmem->iores.end = rmem->iores.start + rmem->size - 1;
	rmem->owner = owner;

	/* Map RIO space to local DMA memory */
	if ((ret = rio_map_inb_region(mport, rmem, 0))) {
		printk(KERN_ERR "RIO map inbound mem error, ret = %d\n", ret);
		goto err;
	}

	/* Claim the region */
	if ((ret = rio_space_claim(rmem))) {
		printk(KERN_ERR "RIO inbound mem claim error, ret = %d\n", ret);
		goto err;
	}
	list_add(&rmem->node, &rio_inb_mems);

	return rmem;

err:
	rio_release_inb_region(mport, rmem);
	return NULL;
}

/**
 * rio_release_outb_region -- Release the outbound region resource.
 * @mport: Master port
 * @mem: Outbound region descriptor
 *
 * Return 0 is successed.
 */
int rio_release_outb_region(struct rio_mport *mport, struct rio_mem *mem)
{
	int rc = 0;
	if (!mem)
		return rc;
	rio_unmap_outb_region(mport, mem);
	rio_space_release(mem);
	if (mem->virt)
		iounmap(mem->virt);

	if (mem->iores.parent)
		rc = release_resource(&mem->iores);
	if (mem->riores.parent && !rc)
		rc = release_resource(&mem->riores);

	if (mem->node.prev)
		list_del(&mem->node);

	kfree(mem);

	return rc;
}

/** rio_prepare_io_mem -- Prepare IO region for RapidIO outbound mapping
 * @mport: Master port
 * @dev: RIO device specific pointer to pass
 * @size: Request IO size
 * @name: The request IO resource name
 *
 * Return: The rio_mem descriptor with IO region resource.
 *
 * This function request IO region firstly and ioremap it for preparing
 * outbound window mapping. The function do not map the outbound region
 * because ioremap can not located at the interrupt action function.
 * The function can be called in the initialization for just prepared.
 */
struct rio_mem *rio_prepare_io_mem(struct rio_mport *mport,
		struct rio_dev *dev, resource_size_t size, const char *name)
{
	struct rio_mem *rmem = NULL;

	rmem = kzalloc(sizeof(struct rio_mem), GFP_KERNEL);
	if (!rmem)
		goto err;

	/* Align the size to 2^N */
	size = (size < 0x1000) ? 0x1000 : 1 << (__ilog2(size - 1) + 1);

	/* Request RapidIO IO region */
	if (!(rio_request_io_region(mport, dev, 0, size,
				name, RIO_RESOURCE_MEM, &rmem->iores))) {
		ERR("RIO io region request error!\n");
		goto err;
	}

	rmem->virt = ioremap((phys_addr_t)(rmem->iores.start), size);
	rmem->size = size;

	list_add(&rmem->node, &rio_outb_mems);
	return rmem;
err:
	rio_release_outb_region(mport, rmem);
	return NULL;
}

/** rio_request_outb_region -- Request IO region and get outbound region
 *                             for RapidIO outbound mapping
 * @mport: Master port
 * @dev_id: RIO device specific pointer to pass
 * @size: Request IO size
 * @name: The request IO resource name
 * @owner: The outbound region owned driver
 *
 * Return: The rio_mem descriptor with IO region resource.
 *
 * This function request IO region firstly and ioremap it for preparing
 * outbound window mapping. And it will find the RapidIO region owned by
 * the driver id. Then map it. Be careful about that the ioremap can not
 * be called in the interrupt event action function.
 */
struct rio_mem *rio_request_outb_region(struct rio_mport *mport, void *dev_id,
			resource_size_t size, const char *name, u32 owner)
{
	struct rio_mem *rmem = NULL;
	struct rio_dev *dev = dev_id;

	if (!dev)
		goto err;

	rmem = rio_prepare_io_mem(mport, dev, size, name);
	if (!rmem)
		goto err;

	if (rio_space_find_mem(mport, dev->destid, owner, &rmem->riores)) {
		ERR("Can not find RIO region meet the ownerid %x\n", owner);
		goto err;
	}

	/* Map the rio space to local */
	if (rio_map_outb_region(mport, dev->destid, rmem, 0)) {
		ERR("RIO map outb error!\n");
		goto err;
	}
	return rmem;
err:
	rio_release_outb_region(mport, rmem);
	return NULL;
}

/**
 * rio_mport_get_feature - query for devices' extended features
 * @port: Master port to issue transaction
 * @local: Indicate a local master port or remote device access
 * @destid: Destination ID of the device
 * @hopcount: Number of switch hops to the device
 * @ftr: Extended feature code
 *
 * Tell if a device supports a given RapidIO capability.
 * Returns the offset of the requested extended feature
 * block within the device's RIO configuration space or
 * 0 in case the device does not support it.  Possible
 * values for @ftr:
 *
 * %RIO_EFB_PAR_EP_ID		LP/LVDS EP Devices
 *
 * %RIO_EFB_PAR_EP_REC_ID	LP/LVDS EP Recovery Devices
 *
 * %RIO_EFB_PAR_EP_FREE_ID	LP/LVDS EP Free Devices
 *
 * %RIO_EFB_SER_EP_ID		LP/Serial EP Devices
 *
 * %RIO_EFB_SER_EP_REC_ID	LP/Serial EP Recovery Devices
 *
 * %RIO_EFB_SER_EP_FREE_ID	LP/Serial EP Free Devices
 */
u32
rio_mport_get_feature(struct rio_mport * port, int local, u16 destid,
		      u8 hopcount, int ftr)
{
	u32 asm_info, ext_ftr_ptr, ftr_header;

	if (local)
		rio_local_read_config_32(port, RIO_ASM_INFO_CAR, &asm_info);
	else
		rio_mport_read_config_32(port, destid, hopcount,
					 RIO_ASM_INFO_CAR, &asm_info);

	ext_ftr_ptr = asm_info & RIO_EXT_FTR_PTR_MASK;

	while (ext_ftr_ptr) {
		if (local)
			rio_local_read_config_32(port, ext_ftr_ptr,
						 &ftr_header);
		else
			rio_mport_read_config_32(port, destid, hopcount,
						 ext_ftr_ptr, &ftr_header);
		if (RIO_GET_BLOCK_ID(ftr_header) == ftr)
			return ext_ftr_ptr;
		if (!(ext_ftr_ptr = RIO_GET_BLOCK_PTR(ftr_header)))
			break;
	}

	return 0;
}

/**
 * rio_get_asm - Begin or continue searching for a RIO device by vid/did/asm_vid/asm_did
 * @vid: RIO vid to match or %RIO_ANY_ID to match all vids
 * @did: RIO did to match or %RIO_ANY_ID to match all dids
 * @asm_vid: RIO asm_vid to match or %RIO_ANY_ID to match all asm_vids
 * @asm_did: RIO asm_did to match or %RIO_ANY_ID to match all asm_dids
 * @from: Previous RIO device found in search, or %NULL for new search
 *
 * Iterates through the list of known RIO devices. If a RIO device is
 * found with a matching @vid, @did, @asm_vid, @asm_did, the reference
 * count to the device is incrememted and a pointer to its device
 * structure is returned. Otherwise, %NULL is returned. A new search
 * is initiated by passing %NULL to the @from argument. Otherwise, if
 * @from is not %NULL, searches continue from next device on the global
 * list. The reference count for @from is always decremented if it is
 * not %NULL.
 */
struct rio_dev *rio_get_asm(u16 vid, u16 did,
			    u16 asm_vid, u16 asm_did, struct rio_dev *from)
{
	struct list_head *n;
	struct rio_dev *rdev;

	WARN_ON(in_interrupt());
	spin_lock(&rio_global_list_lock);
	n = from ? from->global_list.next : rio_devices.next;

	while (n && (n != &rio_devices)) {
		rdev = rio_dev_g(n);
		if ((vid == RIO_ANY_ID || rdev->vid == vid) &&
		    (did == RIO_ANY_ID || rdev->did == did) &&
		    (asm_vid == RIO_ANY_ID || rdev->asm_vid == asm_vid) &&
		    (asm_did == RIO_ANY_ID || rdev->asm_did == asm_did))
			goto exit;
		n = n->next;
	}
	rdev = NULL;
      exit:
	rio_dev_put(from);
	rdev = rio_dev_get(rdev);
	spin_unlock(&rio_global_list_lock);
	return rdev;
}

/**
 * rio_get_device - Begin or continue searching for a RIO device by vid/did
 * @vid: RIO vid to match or %RIO_ANY_ID to match all vids
 * @did: RIO did to match or %RIO_ANY_ID to match all dids
 * @from: Previous RIO device found in search, or %NULL for new search
 *
 * Iterates through the list of known RIO devices. If a RIO device is
 * found with a matching @vid and @did, the reference count to the
 * device is incrememted and a pointer to its device structure is returned.
 * Otherwise, %NULL is returned. A new search is initiated by passing %NULL
 * to the @from argument. Otherwise, if @from is not %NULL, searches
 * continue from next device on the global list. The reference count for
 * @from is always decremented if it is not %NULL.
 */
struct rio_dev *rio_get_device(u16 vid, u16 did, struct rio_dev *from)
{
	return rio_get_asm(vid, did, RIO_ANY_ID, RIO_ANY_ID, from);
}

static void rio_fixup_device(struct rio_dev *dev)
{
}

static int __devinit rio_init(void)
{
	struct rio_dev *dev = NULL;

	while ((dev = rio_get_device(RIO_ANY_ID, RIO_ANY_ID, dev)) != NULL) {
		rio_fixup_device(dev);
	}
	return 0;
}

device_initcall(rio_init);

int rio_init_mports(void)
{
	int rc = 0;
	struct rio_mport *port;

	list_for_each_entry(port, &rio_mports, node) {
		if (!request_mem_region(port->iores.start,
					port->iores.end - port->iores.start,
					port->name)) {
			printk(KERN_ERR
			       "RIO: Error requesting master port region %016llx-%016llx\n",
			       (u64)port->iores.start, (u64)port->iores.end - 1);
			rc = -ENOMEM;
			goto out;
		}

		if (port->host_deviceid >= 0)
			rio_enum_mport(port);
		else
			rio_disc_mport(port);
		rio_space_init(port);
	}

      out:
	return rc;
}

void rio_register_mport(struct rio_mport *port)
{
	list_add_tail(&port->node, &rio_mports);
}

EXPORT_SYMBOL_GPL(rio_local_get_device_id);
EXPORT_SYMBOL_GPL(rio_get_device);
EXPORT_SYMBOL_GPL(rio_get_asm);
EXPORT_SYMBOL_GPL(rio_request_inb_dbell);
EXPORT_SYMBOL_GPL(rio_release_inb_dbell);
EXPORT_SYMBOL_GPL(rio_request_outb_dbell);
EXPORT_SYMBOL_GPL(rio_release_outb_dbell);
EXPORT_SYMBOL_GPL(rio_request_inb_mbox);
EXPORT_SYMBOL_GPL(rio_release_inb_mbox);
EXPORT_SYMBOL_GPL(rio_request_outb_mbox);
EXPORT_SYMBOL_GPL(rio_release_outb_mbox);

#ifdef CONFIG_RAPIDIO_PROC_FS
enum { MAX_IORES_LEVEL = 5 };

struct riors {
	struct rio_mport *mp;
	int res;
	struct resource *p;
} riomres;

static void *r_next(struct seq_file *m, void *v, loff_t *pos)
{
	struct resource *p = v;
	struct riors *rs = m->private;

	(*pos)++;
	if (p->child)
		return p->child;
	while (!p->sibling && p->parent)
		p = p->parent;
	if (p->sibling)
		return p->sibling;
	else {
		rs->res++;
		if(rs->res >= RIO_MAX_MPORT_RESOURCES) {
			rs->mp = list_entry(rs->mp->node.next, struct rio_mport, node);
			rs->res = 0;
			if (&rs->mp->node == &rio_mports)
				return NULL;
		}
		seq_printf(m, "%2d: ", rs->res);
		rs->p = &rs->mp->riores[rs->res];
		p = rs->p;

		return p;
	}
}

static void *r_start(struct seq_file *m, loff_t *pos)
{
	struct riors *rs = m->private;
	struct resource *p;

	if (*pos) {
		*pos = 0;
		return NULL;
	}

	rs->mp = list_entry(rio_mports.next, struct rio_mport, node);
	rs->res = -1;
	rs->p = &rs->mp->iores;
	p = rs->p;

	seq_printf(m, "IO: ");

	return p;
}

static void r_stop(struct seq_file *m, void *v)
{
}

static int r_show(struct seq_file *m, void *v)
{
	struct riors *rs = m->private;
	struct resource *root = rs->p;
	struct resource *r = v, *p;
	int width = root->end < 0x10000 ? 4 : 8;
	int depth;

	for (depth = 0, p = r; p->parent && depth < MAX_IORES_LEVEL; depth++, p = p->parent)
		if (p == root)
			break;
	seq_printf(m, "%*s%0*llx-%0*llx : %s\n",
			depth * 2, "",
			width, (unsigned long long) r->start,
			width, (unsigned long long) r->end,
			r->name ? r->name : "<BAD>");
	return 0;
}

static const struct seq_operations resource_op = {
	.start	= r_start,
	.next	= r_next,
	.stop	= r_stop,
	.show	= r_show,
};

static int riores_open(struct inode *inode, struct file *file)
{
	int res = seq_open(file, &resource_op);
	if (!res) {
		struct seq_file *m = file->private_data;
		m->private = &riomres;
	}
	return res;
}

static const struct file_operations proc_riores_operations = {
	.open		= riores_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static int __init rioresources_init(void)
{
	struct proc_dir_entry *entry;

	entry = create_proc_entry("riores", 0, NULL);
	if (entry)
		entry->proc_fops = &proc_riores_operations;
	return 0;
}
__initcall(rioresources_init);
#endif
