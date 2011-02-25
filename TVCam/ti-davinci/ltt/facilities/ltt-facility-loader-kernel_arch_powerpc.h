#ifndef _LTT_FACILITY_LOADER_KERNEL_ARCH_H_
#define _LTT_FACILITY_LOADER_KERNEL_ARCH_H_

#ifdef CONFIG_LTT

#include <linux/ltt-facilities.h>
#include <ltt/ltt-facility-id-kernel_arch_powerpc.h>

ltt_facility_t	ltt_facility_kernel_arch;
ltt_facility_t	ltt_facility_kernel_arch_E944278C;

#define LTT_FACILITY_SYMBOL		ltt_facility_kernel_arch
#define LTT_FACILITY_CHECKSUM_SYMBOL	ltt_facility_kernel_arch_E944278C
#define LTT_FACILITY_CHECKSUM		0xE944278C
#define LTT_FACILITY_NAME		"kernel_arch"
#define LTT_FACILITY_NUM_EVENTS	facility_kernel_arch_num_events

#endif //CONFIG_LTT

#endif //_LTT_FACILITY_LOADER_KERNEL_ARCH_H_
