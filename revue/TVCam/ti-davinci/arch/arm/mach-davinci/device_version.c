/*****************************************************************************/
/*                                                                           */
/*  Name:           device_version.c                                                 */
/*  Autor:          hw.huang                                            */
/*  Description:    device version                                           */
/*  Project:        IPNC DM365					                              */
/*                                                                           */
/*****************************************************************************/

/**************************************************************************
 * Included Files
 **************************************************************************/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/proc_fs.h>

#include <asm/arch/io.h>
#include <asm/arch/hardware.h>
#include <asm/arch/device_version.h>


#if CONFIG_PROC_FS

#define MAX_BUF		4096

int device_version_get( void )
{
	unsigned int *pPINMUX1 = (unsigned int *)IO_ADDRESS(PINMUX1);
	
	if( ((*pPINMUX1) & (3<<20)) == 0 )
	{
		/*old version - gio*/
		return 0;
	}else{
		/*new version - extclk*/
		return 1;
	}

}

static int proc_device_version(char *page, char **start, off_t off,
		       int count, int *eof, void *data)
{
	int len = 0;
	char buffer[MAX_BUF];
	
	len +=
	    sprintf(&buffer[len],"%d\n",device_version_get());

	if (count > len)
		count = len;
	memcpy(page, &buffer[off], count);

	*eof = 1;
	*start = NULL;
	return len;
}

int __init device_version_proc_client_create(void)
{
	create_proc_read_entry("device_version", 0, NULL, proc_device_version, NULL);

	
	return 0;
}

#else				/* CONFIG_PROC_FS */

#define device_version_proc_client_create() do {} while(0);

#endif				/* CONFIG_PROC_FS */

EXPORT_SYMBOL(device_version_get);

module_init(device_version_proc_client_create);
