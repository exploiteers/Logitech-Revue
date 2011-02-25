/*
 * ltt-facility-loader-process.c
 *
 * (C) Copyright  2005 - 
 *          Mathieu Desnoyers (mathieu.desnoyers@polymtl.ca)
 *
 * Contains the LTT facility loader.
 *
 */


#include <linux/ltt-facilities.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/config.h>
#include "ltt-facility-loader-process.h"


#ifdef CONFIG_LTT

EXPORT_SYMBOL(LTT_FACILITY_SYMBOL);
EXPORT_SYMBOL(LTT_FACILITY_CHECKSUM_SYMBOL);

static const char ltt_facility_name[] = LTT_FACILITY_NAME;

#define SYMBOL_STRING(sym) #sym

static struct ltt_facility facility = {
	.name = ltt_facility_name,
	.num_events = LTT_FACILITY_NUM_EVENTS,
	.checksum = LTT_FACILITY_CHECKSUM,
	.symbol = SYMBOL_STRING(LTT_FACILITY_SYMBOL),
};

static int __init facility_init(void)
{
	printk(KERN_INFO "LTT : ltt-facility-process init in kernel\n");

	LTT_FACILITY_SYMBOL = ltt_facility_kernel_register(&facility);
	LTT_FACILITY_CHECKSUM_SYMBOL = LTT_FACILITY_SYMBOL;
	
	return LTT_FACILITY_SYMBOL;
}

#ifndef MODULE
__initcall(facility_init);
#else
module_init(facility_init);
static void __exit facility_exit(void)
{
	int err;

	err = ltt_facility_unregister(LTT_FACILITY_SYMBOL);
	if (err != 0)
		printk(KERN_ERR "LTT : Error in unregistering facility.\n");

}
module_exit(facility_exit)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Linux Trace Toolkit Facility");

#endif //MODULE

#endif //CONFIG_LTT
