#ifndef _LTT_FACILITY_SELECT_PROCESS_H_
#define _LTT_FACILITY_SELECT_PROCESS_H_

#ifdef CONFIG_LTT
#include <linux/ltt-facilities.h>
#include <ltt/ltt-tracer.h>
#include <ltt/ltt-facility-id-process.h>

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
static inline unsigned int ltt_get_index_from_facility_process(u8 eID)
{
	switch (eID) {
		case event_process_fork:
		case event_process_kernel_thread:
		case event_process_exit:
		case event_process_wait:
		case event_process_free:
			return GET_CHANNEL_INDEX(processes);
		default:
			return GET_CHANNEL_INDEX(cpu);
	}
}
#endif //CONFIG_LTT
#endif //_LTT_FACILITY_SELECT_PROCESS_H_
