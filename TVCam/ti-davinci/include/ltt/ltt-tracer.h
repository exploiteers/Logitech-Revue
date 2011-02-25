/*
 * include/ltt/ltt-tracer.h
 *
 * Copyright (C) 2005,2006 Mathieu Desnoyers (mathieu.desnoyers@polymtl.ca)
 *
 * This contains the definitions for the Linux Trace Toolkit tracer.
 */

#ifndef _LTT_TRACER_H
#define _LTT_TRACER_H

#include <linux/config.h>
#include <linux/types.h>
#include <linux/limits.h>
#include <linux/list.h>
#include <linux/cache.h>
#include <linux/kernel.h>
#include <linux/timex.h>
#include <linux/wait.h>
#include <linux/relay.h>
#include <linux/ltt-facilities.h>
#include <linux/ltt-core.h>
#include <ltt/ltt-facility-id-core.h>
#include <ltt/ltt-facility-id-process.h>
#include <ltt/ltt-facility-id-network_ip_interface.h>
#include <ltt/ltt-facility-id-statedump.h>

#include <asm/semaphore.h>
#include <asm/atomic.h>
#include <asm/ltt.h>

/* Number of bytes to log with a read/write event */
#define LTT_LOG_RW_SIZE			32

#ifdef CONFIG_LTT_ALIGNMENT

/* Calculate the offset needed to align the type */
static inline unsigned int ltt_align(size_t align_drift,
		 size_t size_of_type)
{
	size_t alignment = min(sizeof(void*), size_of_type);
	return ((alignment - align_drift) & (alignment-1));
}
/* Default arch alignment */
#define LTT_ALIGN

#else
static inline unsigned int ltt_align(size_t align_drift,
		 size_t size_of_type)
{
	return 0;
}

#define LTT_ALIGN __attribute__((packed))

#endif //CONFIG_LTT_ALIGNMENT

#ifdef CONFIG_LTT


struct ltt_trace_struct;

/* LTTng lockless logging buffer info */
struct ltt_channel_buf_struct {
	/* Use the relay void *start as buffer start pointer */
	atomic_t offset;		/* Current offset in the buffer
					   atomic_up access. */
	atomic_t consumed;		/* Current offset in the buffer
					   standard atomic access (shared) */
	atomic_t active_readers;	/* Active readers count
					   standard atomic access (shared) */
	atomic_t wakeup_readers;	/* Boolean : wakeup readers waiting ? */
	atomic_t *commit_count;		/* Commit count per sub-buffer
					   atomic_up access. */
	spinlock_t full_lock;		/* buffer full condition spinlock, only
					 * for userspace tracing blocking mode
					 * synchronisation with reader. */
	atomic_t events_lost;		/* atomic_up access */
	atomic_t corrupted_subbuffers;	/* atomic_up access */
	struct timeval	current_subbuffer_start_time;
	wait_queue_head_t write_wait;	/* Wait queue for blocking user space
					 * writers */
	struct work_struct wake_writers;/* Writers wake-up work struct */
} ____cacheline_aligned;

struct ltt_channel_struct {
	char channel_name[PATH_MAX];
	struct ltt_trace_struct	*trace;
	struct ltt_channel_buf_struct buf[NR_CPUS];
	int overwrite;
	struct kref kref;

	void *trans_channel_data;

	/*
	 * buffer_begin - called on buffer-switch to a new sub-buffer
	 * @buf: the channel buffer containing the new sub-buffer
	 */
	void (*buffer_begin) (struct rchan_buf *buf,
			u64 tsc, unsigned int subbuf_idx);
	/*
	 * buffer_end - called on buffer-switch to a new sub-buffer
	 * @buf: the channel buffer containing the previous sub-buffer
	 */
	void (*buffer_end) (struct rchan_buf *buf,
			u64 tsc, unsigned int offset, unsigned int subbuf_idx);
};

struct user_dbg_data {
	unsigned long avail_size;
	unsigned long write;
	unsigned long read;
};

struct ltt_trace_ops {
	int (*create_dirs) (struct ltt_trace_struct *new_trace);
	void (*remove_dirs) (struct ltt_trace_struct *new_trace);
	int (*create_channel) (char *trace_name, struct ltt_trace_struct *trace,
				struct dentry *dir, char *channel_name,
				struct ltt_channel_struct **ltt_chan,
				unsigned int subbuf_size,
				unsigned int n_subbufs, int overwrite);
	void (*wakeup_channel) (struct ltt_channel_struct *ltt_channel);
	void (*finish_channel) (struct ltt_channel_struct *channel);
	void (*remove_channel) (struct ltt_channel_struct *channel);
	void *(*reserve_slot) (struct ltt_trace_struct *trace,
				struct ltt_channel_struct *channel,
				void **transport_data, size_t data_size,
				size_t *slot_size, u64 *tsc,
				size_t *before_hdr_pad, size_t *after_hdr_pad,
				size_t *header_size);
	void (*commit_slot) (struct ltt_channel_struct *channel,
				void **transport_data, void *reserved,
				size_t slot_size);
	void (*print_errors)(struct ltt_trace_struct *trace,
				struct ltt_channel_struct *ltt_chan, int cpu);
	int (*user_blocking) (struct ltt_trace_struct *trace,
				unsigned int index, size_t data_size,
				struct user_dbg_data *dbg);
	void (*user_errors) (struct ltt_trace_struct *trace,
				unsigned int index, size_t data_size,
				struct user_dbg_data *dbg);
};

struct ltt_transport {
	char *name;
	struct module *owner;
	struct list_head node;
	struct ltt_trace_ops ops;
};


enum trace_mode { LTT_TRACE_NORMAL, LTT_TRACE_FLIGHT, LTT_TRACE_HYBRID };

/* Per-trace information - each trace/flight recorder represented by one */
struct ltt_trace_struct {
	struct list_head list;
	int active;
	char trace_name[NAME_MAX];
	int paused;
	enum trace_mode mode;
	struct ltt_transport *transport;
	struct ltt_trace_ops *ops;
	struct kref ltt_transport_kref;
	u32 freq_scale;
	u64 start_freq;
	u64 start_tsc;
	unsigned long long start_monotonic;
	struct timeval		start_time;
	struct {
		struct dentry			*trace_root;
		struct dentry			*control_root;
	} dentry;
	struct {
		struct ltt_channel_struct	*facilities;
		struct ltt_channel_struct	*interrupts;
		struct ltt_channel_struct	*processes;
		struct ltt_channel_struct	*modules;
		struct ltt_channel_struct	*cpu;
		struct ltt_channel_struct	*network;
	} channel;
	struct rchan_callbacks callbacks;
	struct kref kref; /* Each channel has a kref of the trace struct */
} ____cacheline_aligned;

enum ltt_channels { LTT_CHANNEL_FACILITIES, LTT_CHANNEL_INTERRUPTS,
	LTT_CHANNEL_PROCESSES, LTT_CHANNEL_MODULES, LTT_CHANNEL_CPU,
	LTT_CHANNEL_NETWORK };

/* Hardcoded event headers */

/* event header for a trace with active heartbeat : 32 bits timestamps */

/* headers are 8 bytes aligned : that means members are aligned on memory
 * boundaries *if* structure starts on a 8 bytes boundary. In order to insure
 * such alignment, a dynamic per trace alignment value must be set.
 *
 * Remeber that the C compiler does align each member on the boundary equivalent
 * to their own size.
 *
 * As relay subbuffers are aligned on pages, we are sure that they are 8 bytes
 * aligned, so the buffer header and trace header are aligned.
 *
 * Event headers are aligned depending on the trace alignment option. */

struct ltt_event_header_hb {
	uint32_t timestamp;
	unsigned char facility_id;
	unsigned char event_id;
	uint16_t event_size;
} __attribute((packed));

struct ltt_event_header_nohb {
	uint64_t timestamp;
	unsigned char facility_id;
	unsigned char event_id;
	uint16_t event_size;
} __attribute((packed));

struct ltt_trace_header {
	uint32_t magic_number;
	uint32_t arch_type;
	uint32_t arch_variant;
	uint32_t float_word_order;	 /* Only useful for user space traces */
	uint8_t arch_size;
	uint8_t major_version;
	uint8_t minor_version;
	uint8_t flight_recorder;
	uint8_t has_heartbeat;
	uint8_t has_alignment;		/* Event header alignment */
	uint32_t freq_scale;
	uint64_t start_freq;
	uint64_t start_tsc;
	uint64_t start_monotonic;
	uint64_t start_time_sec;
	uint64_t start_time_usec;
} __attribute((packed));


/* We use asm/timex.h : cpu_khz/HZ variable in here : we might have to deal
 * specifically with CPU frequency scaling someday, so using an interpolation
 * between the start and end of buffer values is not flexible enough. Using an
 * immediate frequency value permits to calculate directly the times for parts
 * of a buffer that would be before a frequency change. */
struct ltt_block_start_header {
	struct { 
		uint64_t cycle_count;
		uint64_t freq; /* khz */
	} begin;
	struct { 
		uint64_t cycle_count;
		uint64_t freq; /* khz */
	} end;
	uint32_t lost_size;	/* Size unused at the end of the buffer */
	uint32_t buf_size;	/* The size of this sub-buffer */
	struct ltt_trace_header	trace;
} __attribute((packed));

/*
 * ltt_subbuf_header_len - called on buffer-switch to a new sub-buffer
 *
 * returns the client header size at the beginning of the buffer.
 */
static inline unsigned int ltt_subbuf_header_len(void)
{
	return sizeof(struct ltt_block_start_header);
}

/* Get the offset of the channel in the ltt_trace_struct */
#define GET_CHANNEL_INDEX(chan)	\
	(unsigned int)&((struct ltt_trace_struct*)NULL)->channel.chan

static inline struct ltt_channel_struct *ltt_get_channel_from_index(
		struct ltt_trace_struct *trace, unsigned int index)
{
	return *(struct ltt_channel_struct **)((void*)trace+index);
}


/*
 * ltt_get_header_size
 *
 * Calculate alignment offset for arch size void*. This is the
 * alignment offset of the event header.
 *
 * Important note :
 * The event header must be a size multiple of the void* size. This is necessary
 * to be able to calculate statically the alignment offset of the variable
 * length data fields that follows. The total offset calculated here :
 *
 *	 Alignment of header struct on arch size
 * + sizeof(header struct)
 * + padding added to end of struct to align on arch size.
 * */
static inline unsigned char ltt_get_header_size(struct ltt_trace_struct *trace,
		void *address,
		size_t *before_hdr_pad,
		size_t *after_hdr_pad,
		size_t *header_size)
{
	unsigned int padding;
	unsigned int header;
	
#ifdef CONFIG_LTT_HEARTBEAT_EVENT
	header = sizeof(struct ltt_event_header_hb);
#else
	header = sizeof(struct ltt_event_header_nohb);
#endif // CONFIG_LTT_HEARTBEAT_EVENT

	/* Padding before the header. Calculated dynamically */
	*before_hdr_pad = ltt_align((unsigned long)address, header);
	padding = *before_hdr_pad;

	/* Padding after header, considering header aligned on ltt_align.
	 * Calculated statically if header size if known. */
	*after_hdr_pad = ltt_align(header, sizeof(void*));
	padding += *after_hdr_pad;

	*header_size = header;

	return header+padding;
}


/* ltt_write_event_header
 *
 * Writes the event header to the pointer.
 *
 * @channel : pointer to the channel structure
 * @ptr : buffer pointer
 * @fID : facility ID
 * @eID : event ID
 * @event_size : size of the event, excluding the event header.
 * @offset : offset of the beginning of the header, for alignment.
 *           Calculated by ltt_get_event_header_size.
 * @tsc : time stamp counter.
 */
static inline void ltt_write_event_header(struct ltt_trace_struct *trace,
		struct ltt_channel_struct *channel,
		void *ptr, ltt_facility_t fID,
		uint32_t eID, size_t event_size,
		size_t offset, u64 tsc)
{
#ifdef CONFIG_LTT_HEARTBEAT_EVENT
	struct ltt_event_header_hb *hb;

	event_size = min(event_size, (size_t)0xFFFFU);
	hb = (struct ltt_event_header_hb *)(ptr+offset);
	hb->timestamp = (u32)tsc;
	hb->facility_id = fID;
	hb->event_id = eID;
	hb->event_size = (uint16_t)event_size;
#else
	struct ltt_event_header_nohb *nohb;
	
	event_size = min(event_size, (size_t)0xFFFFU);
	nohb = (struct ltt_event_header_nohb *)(ptr+offset);
	nohb->timestamp = (u64)tsc;
	nohb->facility_id = fID;
	nohb->event_id = eID;
	nohb->event_size = (uint16_t)event_size;
#endif //CONFIG_LTT_HEARTBEAT_EVENT
}

/* for flight recording. must be called after relay_commit.
 * This function does not protect from corruption resulting from writing non
 * sequentially in the buffer (and trying to read this buffer after a crash
 * which occured at the wrong moment).
 * That's why sequential writes are good!
 *
 * This function does nothing if trace is in normal mode. */
#if 0
static inline void ltt_write_commit_counter(struct rchan_buf *buf,
		void *reserved)
{
	struct ltt_channel_struct *channel = 
		(struct ltt_channel_struct*)buf->chan->client_data;
	struct ltt_block_start_header *header =
		(struct ltt_block_start_header*)buf->data;
	unsigned offset, subbuf_idx;
	
	offset = reserved - buf->start;
	subbuf_idx = offset / buf->chan->subbuf_size;

	if (channel->trace->mode == LTT_TRACE_FLIGHT)
		header->lost_size = buf->chan->subbuf_size - 
			buf->commit[subbuf_idx];

}
#endif //0

/* Lockless LTTng */

/* Buffer offset macros */

#define BUFFER_OFFSET(offset, chan) ((offset) & (chan->alloc_size-1))
#define SUBBUF_OFFSET(offset, chan) ((offset) & (chan->subbuf_size-1))
#define SUBBUF_ALIGN(offset, chan) \
	(((offset) + chan->subbuf_size) & (~(chan->subbuf_size-1)))
#define SUBBUF_TRUNC(offset, chan) \
	((offset) & (~(chan->subbuf_size-1)))
#define SUBBUF_INDEX(offset, chan) \
	(BUFFER_OFFSET((offset),chan)/chan->subbuf_size)

/* ltt_reserve_slot
 *
 * Atomic slot reservation in a LTTng buffer. It will take care of
 * sub-buffer switching.
 *
 * Parameters:
 *
 * @trace : the trace structure to log to.
 * @buf : the buffer to reserve space into.
 * @data_size : size of the variable length data to log.
 * @slot_size : pointer to total size of the slot (out)
 * @tsc : pointer to the tsc at the slot reservation (out)
 * @before_hdr_pad : dynamic padding before the event header.
 * @after_hdr_pad : dynamic padding after the event header.
 *
 * Return : NULL if not enough space, else returns the pointer
 * 					to the beginning of the reserved slot. */
static inline void *ltt_reserve_slot(
		struct ltt_trace_struct *trace,
		struct ltt_channel_struct *channel,
		void **transport_data,
		size_t data_size,
		size_t *slot_size,
		u64 *tsc,
		size_t *before_hdr_pad,
		size_t *after_hdr_pad,
		size_t *header_size)
{
	return trace->ops->reserve_slot(trace, channel, transport_data,
			data_size, slot_size, tsc, before_hdr_pad,
			after_hdr_pad, header_size);
}
	
	
/* ltt_commit_slot
 *
 * Atomic unordered slot commit. Increments the commit count in the
 * specified sub-buffer, and delivers it if necessary.
 *
 * Parameters:
 *
 * @buf : the buffer to commit to.
 * @reserved : address of the beginnig of the reserved slot.
 * @slot_size : size of the reserved slot.
 *
 */
static inline void ltt_commit_slot(
		struct ltt_channel_struct *channel,
		void **transport_data,
		void *reserved,
		size_t slot_size)
{
	struct ltt_trace_struct *trace = channel->trace;

	trace->ops->commit_slot(channel, transport_data, reserved, slot_size);
}

#endif //CONFIG_LTT

/* Is kernel tracer enabled */
#if defined(CONFIG_LTT_TRACER) || defined(CONFIG_LTT_TRACER_MODULE)

/* 4 control channels :
 * ltt/control/facilities
 * ltt/control/interrupts
 * ltt/control/processes
 * ltt/control/network
 *
 * 1 cpu channel :
 * ltt/cpu
 */
#define LTT_RELAY_ROOT		"ltt"
#define LTT_CONTROL_ROOT	"control"
#define LTT_FACILITIES_CHANNEL	"facilities_"
#define LTT_INTERRUPTS_CHANNEL	"interrupts_"
#define LTT_PROCESSES_CHANNEL	"processes_"
#define LTT_MODULES_CHANNEL	"modules_"
#define LTT_NETWORK_CHANNEL	"network_"
#define LTT_CPU_CHANNEL		"cpu_"
#define LTT_FLIGHT_PREFIX	"flight-"

/* System types */
#define LTT_SYS_TYPE_VANILLA_LINUX	1

/* Architecture types */
#define LTT_ARCH_TYPE_I386		1
#define LTT_ARCH_TYPE_PPC		2
#define LTT_ARCH_TYPE_SH		3
#define LTT_ARCH_TYPE_S390		4
#define LTT_ARCH_TYPE_MIPS		5
#define LTT_ARCH_TYPE_ARM		6
#define LTT_ARCH_TYPE_PPC64		7
#define LTT_ARCH_TYPE_X86_64		8
#define LTT_ARCH_TYPE_C2		9
#define LTT_ARCH_TYPE_POWERPC		10

/* Standard definitions for variants */
#define LTT_ARCH_VARIANT_NONE		0

/* Tracer properties */
#define LTT_DEFAULT_SUBBUF_SIZE_LOW	65536
#define LTT_DEFAULT_N_SUBBUFS_LOW	2
#define LTT_DEFAULT_SUBBUF_SIZE_MED	262144
#define LTT_DEFAULT_N_SUBBUFS_MED	2
#define LTT_DEFAULT_SUBBUF_SIZE_HIGH	1048576
#define LTT_DEFAULT_N_SUBBUFS_HIGH	2
#define LTT_TRACER_MAGIC_NUMBER		0x00D6B7ED
#define LTT_TRACER_VERSION_MAJOR	0
#define LTT_TRACER_VERSION_MINOR	7

/* Size reserved for high priority events (interrupts, NMI, BH) at the end of a
 * nearly full buffer. User space won't use this last amount of space when in
 * blocking mode. This space also includes the event header that would be
 * written by this user space event. */
#define LTT_RESERVE_CRITICAL		4096

/* Register and unregister function pointers */

enum ltt_module_function {
	LTT_FUNCTION_RUN_FILTER,
	LTT_FUNCTION_FILTER_CONTROL,
	LTT_FUNCTION_STATEDUMP
};

extern int ltt_module_register(enum ltt_module_function name, void *function,
		struct module *owner);
extern void ltt_module_unregister(enum ltt_module_function name);

void ltt_transport_register(struct ltt_transport *transport);
void ltt_transport_unregister(struct ltt_transport *transport);

/* Exported control function */

enum ltt_heartbeat_functor_msg { LTT_HEARTBEAT_START, LTT_HEARTBEAT_STOP };

enum ltt_control_msg {
	LTT_CONTROL_START,
	LTT_CONTROL_STOP,
	LTT_CONTROL_CREATE_TRACE,
	LTT_CONTROL_DESTROY_TRACE
};

union ltt_control_args {
	struct {
		enum trace_mode mode;
		unsigned subbuf_size_low;
		unsigned n_subbufs_low;
		unsigned subbuf_size_med;
		unsigned n_subbufs_med;
		unsigned subbuf_size_high;
		unsigned n_subbufs_high;
	} new_trace;
};

extern int ltt_control(enum ltt_control_msg msg, char *trace_name,
		char *trace_type, union ltt_control_args args);

enum ltt_filter_control_msg { 
	LTT_FILTER_DEFAULT_ACCEPT,
	LTT_FILTER_DEFAULT_REJECT };

extern int ltt_filter_control(enum ltt_filter_control_msg msg,
		char *trace_name);

void ltt_write_trace_header(struct ltt_trace_struct *trace,
		struct ltt_trace_header *header);
extern void ltt_buffer_destroy(struct ltt_channel_struct *ltt_chan);
extern void ltt_wakeup_writers(void *private);

void ltt_core_register(int (*function)(u8, void*));

void ltt_core_unregister(void);

#ifdef CONFIG_LTT_HEARTBEAT
int ltt_heartbeat_trigger(enum ltt_heartbeat_functor_msg msg);
#endif //CONFIG_LTT_HEARTBEAT

/* Relay IOCTL */

/* Get the next sub buffer that can be read. */
#define RELAY_GET_SUBBUF		_IOR(0xF4, 0x00,__u32)
/* Release the oldest reserved (by "get") sub buffer. */
#define RELAY_PUT_SUBBUF		_IOW(0xF4, 0x01,__u32)
/* returns the number of sub buffers in the per cpu channel. */
#define RELAY_GET_N_SUBBUFS		_IOR(0xF4, 0x02,__u32)
/* returns the size of the sub buffers. */
#define RELAY_GET_SUBBUF_SIZE		_IOR(0xF4, 0x03,__u32)

#endif /* defined(CONFIG_LTT_TRACER) || defined(CONFIG_LTT_TRACER_MODULE) */
#endif /* _LTT_TRACER_H */
