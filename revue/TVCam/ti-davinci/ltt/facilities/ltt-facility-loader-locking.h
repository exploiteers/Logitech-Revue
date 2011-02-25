#ifndef _LTT_FACILITY_LOADER_LOCKING_H_
#define _LTT_FACILITY_LOADER_LOCKING_H_

#ifdef CONFIG_LTT

#include <linux/ltt-facilities.h>
#include <ltt/ltt-facility-id-locking.h>

ltt_facility_t	ltt_facility_locking;
ltt_facility_t	ltt_facility_locking_89053D28;

#define LTT_FACILITY_SYMBOL		ltt_facility_locking
#define LTT_FACILITY_CHECKSUM_SYMBOL	ltt_facility_locking_89053D28
#define LTT_FACILITY_CHECKSUM		0x89053D28
#define LTT_FACILITY_NAME		"locking"
#define LTT_FACILITY_NUM_EVENTS	facility_locking_num_events

#endif //CONFIG_LTT

#endif //_LTT_FACILITY_LOADER_LOCKING_H_
