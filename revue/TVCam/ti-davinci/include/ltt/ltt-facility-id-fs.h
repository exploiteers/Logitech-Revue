#ifndef _LTT_FACILITY_ID_FS_H_
#define _LTT_FACILITY_ID_FS_H_

#ifdef CONFIG_LTT
#include <linux/ltt-facilities.h>

/****  facility handle  ****/

extern ltt_facility_t ltt_facility_fs_A86EAA61;
extern ltt_facility_t ltt_facility_fs;


/****  event index  ****/

enum fs_event {
	event_fs_buf_wait_start,
	event_fs_buf_wait_end,
	event_fs_exec,
	event_fs_open,
	event_fs_close,
	event_fs_read,
	event_fs_pread64,
	event_fs_readv,
	event_fs_write,
	event_fs_pwrite64,
	event_fs_writev,
	event_fs_lseek,
	event_fs_llseek,
	event_fs_ioctl,
	event_fs_select,
	event_fs_poll,
	facility_fs_num_events
};

#endif //CONFIG_LTT
#endif //_LTT_FACILITY_ID_FS_H_
