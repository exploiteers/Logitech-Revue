#ifndef _LTT_FACILITY_ID_SOCKET_H_
#define _LTT_FACILITY_ID_SOCKET_H_

#ifdef CONFIG_LTT
#include <linux/ltt-facilities.h>

/****  facility handle  ****/

extern ltt_facility_t ltt_facility_socket_5E76ED18;
extern ltt_facility_t ltt_facility_socket;


/****  event index  ****/

enum socket_event {
	event_socket_call,
	event_socket_create,
	event_socket_sendmsg,
	event_socket_recvmsg,
	facility_socket_num_events
};

#endif //CONFIG_LTT
#endif //_LTT_FACILITY_ID_SOCKET_H_
