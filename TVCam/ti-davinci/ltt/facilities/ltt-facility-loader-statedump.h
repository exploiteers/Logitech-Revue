#ifndef _LTT_FACILITY_LOADER_STATEDUMP_H_
#define _LTT_FACILITY_LOADER_STATEDUMP_H_

#ifdef CONFIG_LTT

#include <linux/ltt-facilities.h>
#include <ltt/ltt-facility-id-statedump.h>

ltt_facility_t	ltt_facility_statedump;
ltt_facility_t	ltt_facility_statedump_5E184DBD;

#define LTT_FACILITY_SYMBOL		ltt_facility_statedump
#define LTT_FACILITY_CHECKSUM_SYMBOL	ltt_facility_statedump_5E184DBD
#define LTT_FACILITY_CHECKSUM		0x5E184DBD
#define LTT_FACILITY_NAME		"statedump"
#define LTT_FACILITY_NUM_EVENTS	facility_statedump_num_events

#endif //CONFIG_LTT

#endif //_LTT_FACILITY_LOADER_STATEDUMP_H_
