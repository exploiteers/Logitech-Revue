#ifndef _LTT_FACILITY_SELECT_CORE_H_
#define _LTT_FACILITY_SELECT_CORE_H_

#ifdef CONFIG_LTT
#include <linux/ltt-facilities.h>
#include <ltt/ltt-facility-id-core.h>
#include <ltt/ltt-tracer.h>

/* ltt_get_index_from_facility
 *
 * Get channel index from facility and event id.
 * 
 * @fID : facility ID
 * @eID : event number
 *
 * Get the channel index into which events must be written for the given
 * facility and event number. We get this structure offset as soon as possible
 * and remember it so we pass through this logic only once per trace call (not
 * for every trace).
 */
static inline unsigned int ltt_get_index_from_facility(u8 eID)
{
	//TODO
	// add modules events
	// add interrupts register/unregister events.
	
	return GET_CHANNEL_INDEX(cpu);
}

#define ltt_get_index_from_facility_kernel_arch ltt_get_index_from_facility
#define ltt_get_index_from_facility_debug ltt_get_index_from_facility
#define ltt_get_index_from_facility_fs_data ltt_get_index_from_facility
#define ltt_get_index_from_facility_fs ltt_get_index_from_facility
#define ltt_get_index_from_facility_ipc ltt_get_index_from_facility
#define ltt_get_index_from_facility_locking ltt_get_index_from_facility
#define ltt_get_index_from_facility_memory ltt_get_index_from_facility
#define ltt_get_index_from_facility_network ltt_get_index_from_facility
#define ltt_get_index_from_facility_socket ltt_get_index_from_facility
#define ltt_get_index_from_facility_timer ltt_get_index_from_facility
#define ltt_get_index_from_facility_user_generic ltt_get_index_from_facility
#define ltt_get_index_from_facility_stack ltt_get_index_from_facility

#endif //CONFIG_LTT
#endif //_LTT_FACILITY_SELECT_CORE_H_
