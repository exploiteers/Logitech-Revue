#ifndef _LTT_FACILITY_ID_IPC_H_
#define _LTT_FACILITY_ID_IPC_H_

#ifdef CONFIG_LTT
#include <linux/ltt-facilities.h>

/****  facility handle  ****/

extern ltt_facility_t ltt_facility_ipc_14120A9A;
extern ltt_facility_t ltt_facility_ipc;


/****  event index  ****/

enum ipc_event {
	event_ipc_call,
	event_ipc_msg_create,
	event_ipc_sem_create,
	event_ipc_shm_create,
	facility_ipc_num_events
};

#endif //CONFIG_LTT
#endif //_LTT_FACILITY_ID_IPC_H_
