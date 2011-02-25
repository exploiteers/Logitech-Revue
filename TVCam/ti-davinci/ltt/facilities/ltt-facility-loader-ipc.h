#ifndef _LTT_FACILITY_LOADER_IPC_H_
#define _LTT_FACILITY_LOADER_IPC_H_

#ifdef CONFIG_LTT

#include <linux/ltt-facilities.h>
#include <ltt/ltt-facility-id-ipc.h>

ltt_facility_t	ltt_facility_ipc;
ltt_facility_t	ltt_facility_ipc_14120A9A;

#define LTT_FACILITY_SYMBOL		ltt_facility_ipc
#define LTT_FACILITY_CHECKSUM_SYMBOL	ltt_facility_ipc_14120A9A
#define LTT_FACILITY_CHECKSUM		0x14120A9A
#define LTT_FACILITY_NAME		"ipc"
#define LTT_FACILITY_NUM_EVENTS	facility_ipc_num_events

#endif //CONFIG_LTT

#endif //_LTT_FACILITY_LOADER_IPC_H_
