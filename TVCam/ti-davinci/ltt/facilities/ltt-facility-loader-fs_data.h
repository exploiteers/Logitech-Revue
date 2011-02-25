#ifndef _LTT_FACILITY_LOADER_FS_DATA_H_
#define _LTT_FACILITY_LOADER_FS_DATA_H_

#ifdef CONFIG_LTT

#include <linux/ltt-facilities.h>
#include <ltt/ltt-facility-id-fs_data.h>

ltt_facility_t	ltt_facility_fs_data;
ltt_facility_t	ltt_facility_fs_data_155CEE69;

#define LTT_FACILITY_SYMBOL		ltt_facility_fs_data
#define LTT_FACILITY_CHECKSUM_SYMBOL	ltt_facility_fs_data_155CEE69
#define LTT_FACILITY_CHECKSUM		0x155CEE69
#define LTT_FACILITY_NAME		"fs_data"
#define LTT_FACILITY_NUM_EVENTS	facility_fs_data_num_events

#endif //CONFIG_LTT

#endif //_LTT_FACILITY_LOADER_FS_DATA_H_
