#ifndef _LTT_FACILITY_LOADER_PROCESS_H_
#define _LTT_FACILITY_LOADER_PROCESS_H_

#ifdef CONFIG_LTT

#include <linux/ltt-facilities.h>
#include <ltt/ltt-facility-id-process.h>

ltt_facility_t	ltt_facility_process;
ltt_facility_t	ltt_facility_process_2905B6EB;

#define LTT_FACILITY_SYMBOL		ltt_facility_process
#define LTT_FACILITY_CHECKSUM_SYMBOL	ltt_facility_process_2905B6EB
#define LTT_FACILITY_CHECKSUM		0x2905B6EB
#define LTT_FACILITY_NAME		"process"
#define LTT_FACILITY_NUM_EVENTS	facility_process_num_events

#endif //CONFIG_LTT

#endif //_LTT_FACILITY_LOADER_PROCESS_H_
