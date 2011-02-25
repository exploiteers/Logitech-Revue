/*
 * linux/include/asm-arm/mach/udc_pxa2xx.h
 *
 * This supports machine-specific differences in how the PXA2xx / IXP4xx
 * USB Device Controller (UDC) is wired.
 *
 */
struct pxa2xx_udc_mach_info {
	int  (*udc_is_connected)(void);		/* do we see host? */
	void (*udc_command)(int cmd);
#define	PXA2XX_UDC_CMD_CONNECT		0	/* let host see us */
#define	PXA2XX_UDC_CMD_DISCONNECT	1	/* so host won't see us */
};

