#ifndef _LTT_FACILITY_ID_FS_DATA_H_
#define _LTT_FACILITY_ID_FS_DATA_H_

#ifdef CONFIG_LTT
#include <linux/ltt-facilities.h>

/****  facility handle  ****/

extern ltt_facility_t ltt_facility_fs_data_155CEE69;
extern ltt_facility_t ltt_facility_fs_data;


/****  event index  ****/

enum fs_data_event {
	event_fs_data_read,
	event_fs_data_write,
	facility_fs_data_num_events
};

#endif //CONFIG_LTT
#endif //_LTT_FACILITY_ID_FS_DATA_H_
