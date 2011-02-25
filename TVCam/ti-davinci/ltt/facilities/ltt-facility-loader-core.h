#ifndef _LTT_FACILITY_LOADER_CORE_H_
#define _LTT_FACILITY_LOADER_CORE_H_

#ifdef CONFIG_LTT

#include <linux/ltt-facilities.h>
#include <ltt/ltt-facility-id-core.h>

ltt_facility_t	ltt_facility_core;
ltt_facility_t	ltt_facility_core_1A8DE486;

#define LTT_FACILITY_SYMBOL		ltt_facility_core
#define LTT_FACILITY_CHECKSUM_SYMBOL	ltt_facility_core_1A8DE486
#define LTT_FACILITY_CHECKSUM		0x1A8DE486
#define LTT_FACILITY_NAME		"core"
#define LTT_FACILITY_NUM_EVENTS	facility_core_num_events

#endif //CONFIG_LTT

#endif //_LTT_FACILITY_LOADER_CORE_H_
