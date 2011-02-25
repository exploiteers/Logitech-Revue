#ifndef _LTT_FACILITY_ID_NETWORK_IP_INTERFACE_H_
#define _LTT_FACILITY_ID_NETWORK_IP_INTERFACE_H_

#ifdef CONFIG_LTT
#include <linux/ltt-facilities.h>

/****  facility handle  ****/

extern ltt_facility_t ltt_facility_network_ip_interface_7A3120EF;
extern ltt_facility_t ltt_facility_network_ip_interface;


/****  event index  ****/

enum network_ip_interface_event {
	event_network_ip_interface_dev_up,
	event_network_ip_interface_dev_down,
	facility_network_ip_interface_num_events
};

#endif //CONFIG_LTT
#endif //_LTT_FACILITY_ID_NETWORK_IP_INTERFACE_H_
