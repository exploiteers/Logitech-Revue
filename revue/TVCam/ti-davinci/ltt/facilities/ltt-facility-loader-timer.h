#ifndef _LTT_FACILITY_LOADER_TIMER_H_
#define _LTT_FACILITY_LOADER_TIMER_H_

#ifdef CONFIG_LTT

#include <linux/ltt-facilities.h>
#include <ltt/ltt-facility-id-timer.h>

ltt_facility_t	ltt_facility_timer;
ltt_facility_t	ltt_facility_timer_DBFF2E10;

#define LTT_FACILITY_SYMBOL		ltt_facility_timer
#define LTT_FACILITY_CHECKSUM_SYMBOL	ltt_facility_timer_DBFF2E10
#define LTT_FACILITY_CHECKSUM		0xDBFF2E10
#define LTT_FACILITY_NAME		"timer"
#define LTT_FACILITY_NUM_EVENTS	facility_timer_num_events

#endif //CONFIG_LTT

#endif //_LTT_FACILITY_LOADER_TIMER_H_
