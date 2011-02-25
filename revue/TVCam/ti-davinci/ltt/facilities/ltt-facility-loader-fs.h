#ifndef _LTT_FACILITY_LOADER_FS_H_
#define _LTT_FACILITY_LOADER_FS_H_

#ifdef CONFIG_LTT

#include <linux/ltt-facilities.h>
#include <ltt/ltt-facility-id-fs.h>

ltt_facility_t	ltt_facility_fs;
ltt_facility_t	ltt_facility_fs_A86EAA61;

#define LTT_FACILITY_SYMBOL		ltt_facility_fs
#define LTT_FACILITY_CHECKSUM_SYMBOL	ltt_facility_fs_A86EAA61
#define LTT_FACILITY_CHECKSUM		0xA86EAA61
#define LTT_FACILITY_NAME		"fs"
#define LTT_FACILITY_NUM_EVENTS	facility_fs_num_events

#endif //CONFIG_LTT

#endif //_LTT_FACILITY_LOADER_FS_H_
