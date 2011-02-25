/*
 * ltt-facilities.c
 *
 * (C) Copyright	2005 -
 *		Mathieu Desnoyers (mathieu.desnoyers@polymtl.ca)
 *
 * Contains the kernel core code for Linux Trace Toolkit facilities.
 *
 * Facilities are a group of events that can be recorded by instrumentation
 * points to a trace. We keep track of the active facilities to know which type
 * of information can be present in a trace.
 *
 * We never reuse a facility id.
 * 
 * Author:
 *	Mathieu Desnoyers (mathieu.desnoyers@polymtl.ca)
 *
 */

#include <ltt/ltt-facility-select-core.h>
#include <ltt/ltt-facility-core.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ltt-facilities.h>
#include <linux/spinlock.h>
#include <linux/string.h>

static struct ltt_facility_info facilities[LTT_MAX_NUM_FACILITIES];
static spinlock_t facilities_lock;

/* Facility registration :
 * hash based on the checksum, except for the core facility, which is 0.
 * This function assumes that the facility has never been registered before.
 * User ltt_facility_verify for this. */
int ltt_facility_register(enum ltt_facility_type type,
		const char *name,
		const unsigned int num_events,
		const u32 checksum,
		size_t int_size,
		size_t long_size,
		size_t pointer_size,
		size_t size_t_size,
		size_t alignment)
{
	int fac_id;
	int chk_fac_id;
	int found=0;
  
	spin_lock(&facilities_lock);
	if (type == LTT_FACILITY_TYPE_KERNEL &&
			strncmp(name, "core", sizeof("core")) == 0) {
		fac_id = 0;
	} else {
		/* fac_id based on checksum%LTT_MAX_NUM_FACILITIES
		 * find an empty slot */
		chk_fac_id = fac_id = checksum % LTT_MAX_NUM_FACILITIES;
		do {
			if (atomic_read(&facilities[fac_id].ref) == 0) {
				found = 1;
				break;
			}
			fac_id = (fac_id+1) % LTT_MAX_NUM_FACILITIES;
		} while (fac_id != chk_fac_id);

		if (!found) {
			fac_id = -EPERM;
			goto unlock;
		}
	}
	switch (type) {
		case LTT_FACILITY_TYPE_USER:
			if (strncmp(name, "user_", sizeof("user_")-1) != 0) {
				fac_id = -EPERM;
				goto unlock;
			}
			break;
		case LTT_FACILITY_TYPE_KERNEL:
			break;
	}
	strncpy(facilities[fac_id].name, name, FACNAME_LEN-1);
	facilities[fac_id].name[FACNAME_LEN-1] = '\0';
	facilities[fac_id].type = type;
	facilities[fac_id].num_events = num_events;
	facilities[fac_id].alignment = alignment;
	facilities[fac_id].checksum = checksum;
	facilities[fac_id].int_size = int_size;
	facilities[fac_id].long_size = long_size;
	facilities[fac_id].pointer_size = pointer_size;
	facilities[fac_id].size_t_size = size_t_size;

	if (atomic_read(&facilities[fac_id].ref) == 0) {
		atomic_add(2, &facilities[fac_id].ref);
		trace_core_facility_load(
				facilities[fac_id].name,
				checksum,
				fac_id,
				int_size,
				long_size,
				pointer_size,
				size_t_size,
				alignment);
	} else {
		atomic_inc(&facilities[fac_id].ref);
	}
unlock:
	spin_unlock(&facilities_lock);
	return fac_id;
}

/* Verifies if a facility is already registered. If it is,
 * it returns the facility id. If it is not registered, it returns 0. (0 is the
 * core facility which must never use ltt_facility_verify).
 * If the facility is already registered, increment its refcount. */
int ltt_facility_verify(enum ltt_facility_type type,
		const char *name,
		const unsigned int num_events,
		const u32 checksum,
		size_t int_size,
		size_t long_size,
		size_t pointer_size,
		size_t size_t_size,
		size_t alignment)
{
	int fac_id;
	int chk_fac_id;
	int found=0;
	
	spin_lock(&facilities_lock);
	if (type == LTT_FACILITY_TYPE_KERNEL && 
			strncmp(name, "core", sizeof("core")) == 0) {
		fac_id = 0; /* Core facility */
		goto unlock;
	} else {
		switch (type) {
			case LTT_FACILITY_TYPE_USER:
				if (strncmp(name,
					"user_", sizeof("user_")-1) != 0) {
					fac_id = 0;
					goto unlock;
				}
				break;
			case LTT_FACILITY_TYPE_KERNEL:
				break;
		}
		/* fac_id based on checksum%LTT_MAX_NUM_FACILITIES */
		chk_fac_id = fac_id = checksum % LTT_MAX_NUM_FACILITIES;
		do {
			if (facilities[fac_id].checksum == checksum) {
				/* Possibly found : check carefully */
				if ((atomic_read(&facilities[fac_id].ref) > 0)
				&& strncmp(facilities[fac_id].name,
						name, FACNAME_LEN-1) == 0 &&
				facilities[fac_id].type == type &&
				facilities[fac_id].num_events == num_events &&
				facilities[fac_id].alignment == alignment &&
				facilities[fac_id].checksum == checksum &&
				facilities[fac_id].int_size == int_size &&
				facilities[fac_id].long_size == long_size &&
				facilities[fac_id].pointer_size
					== pointer_size &&
				facilities[fac_id].size_t_size == size_t_size) {
					found = 1;
					break;
				}
			}
			fac_id = (fac_id+1) % LTT_MAX_NUM_FACILITIES;
		} while (fac_id != chk_fac_id);
		
		if (!found) {
			fac_id = 0;
			goto unlock;
		}
		atomic_inc(&facilities[fac_id].ref);
	}
unlock:
	spin_unlock(&facilities_lock);
	return fac_id;
}


unsigned int ltt_facility_kernel_register(struct ltt_facility *facility)
{
	size_t alignment;
#ifdef CONFIG_LTT_ALIGNMENT
	alignment = sizeof(void*);
#else
	alignment = 0;
#endif
	return ltt_facility_register(LTT_FACILITY_TYPE_KERNEL,
			facility->name, facility->num_events,
			facility->checksum,
			sizeof(int), sizeof(long), sizeof(void*),
			sizeof(size_t), alignment);
}

void ltt_facility_ref(ltt_facility_t facility_id)
{
	atomic_inc(&facilities[facility_id].ref);
}

int ltt_facility_unregister(ltt_facility_t facility_id)
{
	int ret;
	int freed = 0;

	if (facility_id >= LTT_MAX_NUM_FACILITIES)
		return -EPERM;
	
	spin_lock(&facilities_lock);
	if (atomic_read(&facilities[facility_id].ref) == 0) {
		ret = -EPERM;
		goto unlock;
	}
	printk(KERN_DEBUG "LTT unregister facility %lu\n", facility_id);
	atomic_dec(&facilities[facility_id].ref);

	/* Disable preemption because we read the traces list, and want it to be
	 * RCU coherent. */
	preempt_disable();
	ltt_nesting[smp_processor_id()]++;
	barrier();

	/* If no more trace in the list, we can free the unused facility,
	 * otherwise it will be freed later when the last trace is destroyed (by
	 * ltt_facility_free_unused()). */
	if (list_empty(&ltt_traces.head))
		if (atomic_read(&facilities[facility_id].ref) == 1) {
			atomic_dec(&facilities[facility_id].ref);
			freed = 1;
		}

	barrier();
	ltt_nesting[smp_processor_id()]--;
	preempt_enable();

	/* Ok, if we think about it, it's never going to be traced as there are
	 * no traces in the list. In fact, we never really want to free a
	 * facility id when there is tracing active.
	 * FIXME : this unload could go away. */
	if (freed)
		trace_core_facility_unload(facility_id);
	ret = 0;
unlock:
	spin_unlock(&facilities_lock);
	return ret;
}

int ltt_facility_user_access_ok(ltt_facility_t fac_id)
{
	if (atomic_read(&facilities[fac_id].ref) == 0)
		return 0;
	if (facilities[fac_id].type == LTT_FACILITY_TYPE_KERNEL)
		return 0;

	return 1;
}

/* Cleanup all the unregistered facilities. This must be done when all the
 * traces are destroyed.
 */
void ltt_facility_free_unused(void)
{
	int fac_id;
	
	spin_lock(&facilities_lock);
	for (fac_id = 0; fac_id < LTT_MAX_NUM_FACILITIES; fac_id++)
		if (atomic_read(&facilities[fac_id].ref) == 1)
			atomic_dec(&facilities[fac_id].ref);
	spin_unlock(&facilities_lock);
}

void ltt_facility_state_dump(struct ltt_trace_struct *trace)
{
	int fac_id;
	struct ltt_facility_info *facility;
	u32 int_size, long_size, pointer_size, size_t_size;

	int_size = sizeof(int);
	long_size = sizeof(long);
	pointer_size = sizeof(void*);
	size_t_size = sizeof(size_t);

	spin_lock(&facilities_lock);
	for (fac_id = 0; fac_id < LTT_MAX_NUM_FACILITIES; fac_id++) {
		if (atomic_read(&facilities[fac_id].ref) > 1) {
			facility = &facilities[fac_id];
			printk(KERN_DEBUG "Dumping facility %s\n",
					facility->name);
			trace_core_state_dump_facility_load(trace,
					facility->name,
					facility->checksum,
					fac_id,
					facility->int_size,
					facility->long_size,
					facility->pointer_size,
					facility->size_t_size,
					facility->alignment);
		}
	}
	spin_unlock(&facilities_lock);
}

EXPORT_SYMBOL(ltt_facility_kernel_register);
EXPORT_SYMBOL(ltt_facility_ref);
EXPORT_SYMBOL(ltt_facility_unregister);
EXPORT_SYMBOL(ltt_facility_free_unused);
EXPORT_SYMBOL(ltt_facility_state_dump);

static int __init ltt_facilities_init(void)
{
	int i;
	printk(KERN_INFO "LTT : ltt-facilities init\n");

	spin_lock_init(&facilities_lock);
	for (i = 0; i < LTT_MAX_NUM_FACILITIES; i++) {
		atomic_set(&facilities[i].ref, 0);
	}
	
	return 0;
}
__initcall(ltt_facilities_init);

