#ifndef _LTT_FACILITY_LOADER_KERNEL_H_
#define _LTT_FACILITY_LOADER_KERNEL_H_

#ifdef CONFIG_LTT

#include <linux/ltt-facilities.h>
#include <ltt/ltt-facility-id-kernel.h>

ltt_facility_t	ltt_facility_kernel;
ltt_facility_t	ltt_facility_kernel_25D21CBD;

#define LTT_FACILITY_SYMBOL		ltt_facility_kernel
#define LTT_FACILITY_CHECKSUM_SYMBOL	ltt_facility_kernel_25D21CBD
#define LTT_FACILITY_CHECKSUM		0x25D21CBD
#define LTT_FACILITY_NAME		"kernel"
#define LTT_FACILITY_NUM_EVENTS	facility_kernel_num_events

#endif //CONFIG_LTT

#endif //_LTT_FACILITY_LOADER_KERNEL_H_
