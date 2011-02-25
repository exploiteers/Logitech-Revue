#ifndef _LTT_FACILITY_ID_STATEDUMP_H_
#define _LTT_FACILITY_ID_STATEDUMP_H_

#ifdef CONFIG_LTT
#include <linux/ltt-facilities.h>

/****  facility handle  ****/

extern ltt_facility_t ltt_facility_statedump_5E184DBD;
extern ltt_facility_t ltt_facility_statedump;


/****  event index  ****/

enum statedump_event {
	event_statedump_enumerate_file_descriptors,
	event_statedump_enumerate_vm_maps,
	event_statedump_enumerate_modules,
	event_statedump_enumerate_interrupts,
	event_statedump_enumerate_process_state,
	event_statedump_enumerate_network_ip_interface,
	event_statedump_statedump_end,
	facility_statedump_num_events
};

#endif //CONFIG_LTT
#endif //_LTT_FACILITY_ID_STATEDUMP_H_
