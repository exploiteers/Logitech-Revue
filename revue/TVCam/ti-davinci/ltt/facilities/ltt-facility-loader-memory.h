#ifndef _LTT_FACILITY_LOADER_MEMORY_H_
#define _LTT_FACILITY_LOADER_MEMORY_H_

#ifdef CONFIG_LTT

#include <linux/ltt-facilities.h>
#include <ltt/ltt-facility-id-memory.h>

ltt_facility_t	ltt_facility_memory;
ltt_facility_t	ltt_facility_memory_7F007B54;

#define LTT_FACILITY_SYMBOL		ltt_facility_memory
#define LTT_FACILITY_CHECKSUM_SYMBOL	ltt_facility_memory_7F007B54
#define LTT_FACILITY_CHECKSUM		0x7F007B54
#define LTT_FACILITY_NAME		"memory"
#define LTT_FACILITY_NUM_EVENTS	facility_memory_num_events

#endif //CONFIG_LTT

#endif //_LTT_FACILITY_LOADER_MEMORY_H_
