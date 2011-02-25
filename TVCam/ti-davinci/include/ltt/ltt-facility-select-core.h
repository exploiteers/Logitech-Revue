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
static inline unsigned int ltt_get_index_from_facility_core(u8 eID)
{
	switch (eID) {
		case event_core_facility_load:
		case event_core_facility_unload:
		case event_core_state_dump_facility_load:
			return GET_CHANNEL_INDEX(facilities);
		default:
			return GET_CHANNEL_INDEX(cpu);
	}
}
#endif //CONFIG_LTT
#endif //_LTT_FACILITY_SELECT_CORE_H_
