#ifndef _LTT_FACILITY_ID_NETWORK_H_
#define _LTT_FACILITY_ID_NETWORK_H_

#ifdef CONFIG_LTT
#include <linux/ltt-facilities.h>

/****  facility handle  ****/

extern ltt_facility_t ltt_facility_network_51F19296;
extern ltt_facility_t ltt_facility_network;


/****  event index  ****/

enum network_event {
	event_network_packet_in,
	event_network_packet_out,
	facility_network_num_events
};

#endif //CONFIG_LTT
#endif //_LTT_FACILITY_ID_NETWORK_H_
