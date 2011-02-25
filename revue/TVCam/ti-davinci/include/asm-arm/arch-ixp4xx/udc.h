/*
 * linux/include/asm-arm/arch-ixp4xx/udc.h
 *
 * It is set in linux/arch/arm/mach-ixp4xx/<machine>.c and used in
 * the probe routine of linux/drivers/usb/gadget/pxa2xx_udc.c
 */

#include <asm/mach/udc_pxa2xx.h>

extern void ixp4xx_set_udc_info(struct pxa2xx_udc_mach_info *info);

