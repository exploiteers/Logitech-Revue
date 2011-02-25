#ifndef _LTT_FACILITY_KERNEL_ARCH_H_
#define _LTT_FACILITY_KERNEL_ARCH_H_

#include <linux/types.h>
#include <ltt/ltt-facility-id-kernel_arch_ppc.h>
#include <ltt/ltt-tracer.h>

/* Named types */

enum lttng_syscall_name {
	LTTNG_restart_syscall = 0,
	LTTNG_exit = 1,
	LTTNG_fork = 2,
	LTTNG_read = 3,
	LTTNG_write = 4,
	LTTNG_open = 5,
	LTTNG_close = 6,
	LTTNG_waitpid = 7,
	LTTNG_creat = 8,
	LTTNG_link = 9,
	LTTNG_unlink = 10,
	LTTNG_execve = 11,
	LTTNG_chdir = 12,
	LTTNG_time = 13,
	LTTNG_mknod = 14,
	LTTNG_chmod = 15,
	LTTNG_lchown = 16,
	LTTNG_break = 17,
	LTTNG_oldstat = 18,
	LTTNG_lseek = 19,
	LTTNG_getpid = 20,
	LTTNG_mount = 21,
	LTTNG_umount = 22,
	LTTNG_setuid = 23,
	LTTNG_getuid = 24,
	LTTNG_stime = 25,
	LTTNG_ptrace = 26,
	LTTNG_alarm = 27,
	LTTNG_oldfstat = 28,
	LTTNG_pause = 29,
	LTTNG_utime = 30,
	LTTNG_stty = 31,
	LTTNG_gtty = 32,
	LTTNG_access = 33,
	LTTNG_nice = 34,
	LTTNG_ftime = 35,
	LTTNG_sync = 36,
	LTTNG_kill = 37,
	LTTNG_rename = 38,
	LTTNG_mkdir = 39,
	LTTNG_rmdir = 40,
	LTTNG_dup = 41,
	LTTNG_pipe = 42,
	LTTNG_times = 43,
	LTTNG_prof = 44,
	LTTNG_brk = 45,
	LTTNG_setgid = 46,
	LTTNG_getgid = 47,
	LTTNG_signal = 48,
	LTTNG_geteuid = 49,
	LTTNG_getegid = 50,
	LTTNG_acct = 51,
	LTTNG_umount2 = 52,
	LTTNG_lock = 53,
	LTTNG_ioctl = 54,
	LTTNG_fcntl = 55,
	LTTNG_mpx = 56,
	LTTNG_setpgid = 57,
	LTTNG_ulimit = 58,
	LTTNG_oldolduname = 59,
	LTTNG_umask = 60,
	LTTNG_chroot = 61,
	LTTNG_ustat = 62,
	LTTNG_dup2 = 63,
	LTTNG_getppid = 64,
	LTTNG_getpgrp = 65,
	LTTNG_setsid = 66,
	LTTNG_sigaction = 67,
	LTTNG_sgetmask = 68,
	LTTNG_ssetmask = 69,
	LTTNG_setreuid = 70,
	LTTNG_setregid = 71,
	LTTNG_sigsuspend = 72,
	LTTNG_sigpending = 73,
	LTTNG_sethostname = 74,
	LTTNG_setrlimit = 75,
	LTTNG_getrlimit = 76,
	LTTNG_getrusage = 77,
	LTTNG_gettimeofday = 78,
	LTTNG_settimeofday = 79,
	LTTNG_getgroups = 80,
	LTTNG_setgroups = 81,
	LTTNG_select = 82,
	LTTNG_symlink = 83,
	LTTNG_oldlstat = 84,
	LTTNG_readlink = 85,
	LTTNG_uselib = 86,
	LTTNG_swapon = 87,
	LTTNG_reboot = 88,
	LTTNG_readdir = 89,
	LTTNG_mmap = 90,
	LTTNG_munmap = 91,
	LTTNG_truncate = 92,
	LTTNG_ftruncate = 93,
	LTTNG_fchmod = 94,
	LTTNG_fchown = 95,
	LTTNG_getpriority = 96,
	LTTNG_setpriority = 97,
	LTTNG_profil = 98,
	LTTNG_statfs = 99,
	LTTNG_fstatfs = 100,
	LTTNG_ioperm = 101,
	LTTNG_socketcall = 102,
	LTTNG_syslog = 103,
	LTTNG_setitimer = 104,
	LTTNG_getitimer = 105,
	LTTNG_stat = 106,
	LTTNG_lstat = 107,
	LTTNG_fstat = 108,
	LTTNG_olduname = 109,
	LTTNG_iopl = 110,
	LTTNG_vhangup = 111,
	LTTNG_idle = 112,
	LTTNG_vm86 = 113,
	LTTNG_wait4 = 114,
	LTTNG_swapoff = 115,
	LTTNG_sysinfo = 116,
	LTTNG_ipc = 117,
	LTTNG_fsync = 118,
	LTTNG_sigreturn = 119,
	LTTNG_clone = 120,
	LTTNG_setdomainname = 121,
	LTTNG_uname = 122,
	LTTNG_modify_ldt = 123,
	LTTNG_adjtimex = 124,
	LTTNG_mprotect = 125,
	LTTNG_sigprocmask = 126,
	LTTNG_create_module = 127,
	LTTNG_init_module = 128,
	LTTNG_delete_module = 129,
	LTTNG_get_kernel_syms = 130,
	LTTNG_quotactl = 131,
	LTTNG_getpgid = 132,
	LTTNG_fchdir = 133,
	LTTNG_bdflush = 134,
	LTTNG_sysfs = 135,
	LTTNG_personality = 136,
	LTTNG_afs_syscall = 137,
	LTTNG_setfsuid = 138,
	LTTNG_setfsgid = 139,
	LTTNG__llseek = 140,
	LTTNG_getdents = 141,
	LTTNG__newselect = 142,
	LTTNG_flock = 143,
	LTTNG_msync = 144,
	LTTNG_readv = 145,
	LTTNG_writev = 146,
	LTTNG_getsid = 147,
	LTTNG_fdatasync = 148,
	LTTNG__sysctl = 149,
	LTTNG_mlock = 150,
	LTTNG_munlock = 151,
	LTTNG_mlockall = 152,
	LTTNG_munlockall = 153,
	LTTNG_sched_setparam = 154,
	LTTNG_sched_getparam = 155,
	LTTNG_sched_setscheduler = 156,
	LTTNG_sched_getscheduler = 157,
	LTTNG_sched_yield = 158,
	LTTNG_sched_get_priority_max = 159,
	LTTNG_sched_get_priority_min = 160,
	LTTNG_sched_rr_get_interval = 161,
	LTTNG_nanosleep = 162,
	LTTNG_mremap = 163,
	LTTNG_setresuid = 164,
	LTTNG_getresuid = 165,
	LTTNG_query_module = 166,
	LTTNG_poll = 167,
	LTTNG_nfsservctl = 168,
	LTTNG_setresgid = 169,
	LTTNG_getresgid = 170,
	LTTNG_prctl = 171,
	LTTNG_rt_sigreturn = 172,
	LTTNG_rt_sigaction = 173,
	LTTNG_rt_sigprocmask = 174,
	LTTNG_rt_sigpending = 175,
	LTTNG_rt_sigtimedwait = 176,
	LTTNG_rt_sigqueueinfo = 177,
	LTTNG_rt_sigsuspend = 178,
	LTTNG_pread64 = 179,
	LTTNG_pwrite64 = 180,
	LTTNG_chown = 181,
	LTTNG_getcwd = 182,
	LTTNG_capget = 183,
	LTTNG_capset = 184,
	LTTNG_sigaltstack = 185,
	LTTNG_sendfile = 186,
	LTTNG_getpmsg = 187,
	LTTNG_putpmsg = 188,
	LTTNG_vfork = 189,
	LTTNG_ugetrlimit = 190,
	LTTNG_readahead = 191,
	LTTNG_mmap2 = 192,
	LTTNG_truncate64 = 193,
	LTTNG_ftruncate64 = 194,
	LTTNG_stat64 = 195,
	LTTNG_lstat64 = 196,
	LTTNG_fstat64 = 197,
	LTTNG_pciconfig_read = 198,
	LTTNG_pciconfig_write = 199,
	LTTNG_pciconfig_iobase = 200,
	LTTNG_multiplexer = 201,
	LTTNG_getdents64 = 202,
	LTTNG_pivot_root = 203,
	LTTNG_fcntl64 = 204,
	LTTNG_madvise = 205,
	LTTNG_mincore = 206,
	LTTNG_gettid = 207,
	LTTNG_tkill = 208,
	LTTNG_setxattr = 209,
	LTTNG_lsetxattr = 210,
	LTTNG_fsetxattr = 211,
	LTTNG_getxattr = 212,
	LTTNG_lgetxattr = 213,
	LTTNG_fgetxattr = 214,
	LTTNG_listxattr = 215,
	LTTNG_llistxattr = 216,
	LTTNG_flistxattr = 217,
	LTTNG_removexattr = 218,
	LTTNG_lremovexattr = 219,
	LTTNG_fremovexattr = 220,
	LTTNG_futex = 221,
	LTTNG_sched_setaffinity = 222,
	LTTNG_sched_getaffinity = 223,
	LTTNG_tuxcall = 225,
	LTTNG_sendfile64 = 226,
	LTTNG_io_setup = 227,
	LTTNG_io_destroy = 228,
	LTTNG_io_getevents = 229,
	LTTNG_io_submit = 230,
	LTTNG_io_cancel = 231,
	LTTNG_set_tid_address = 232,
	LTTNG_fadvise64 = 233,
	LTTNG_exit_group = 234,
	LTTNG_lookup_dcookie = 235,
	LTTNG_epoll_create = 236,
	LTTNG_epoll_ctl = 237,
	LTTNG_epoll_wait = 238,
	LTTNG_remap_file_pages = 239,
	LTTNG_timer_create = 240,
	LTTNG_timer_settime = 241,
	LTTNG_timer_gettime = 242,
	LTTNG_timer_getoverrun = 243,
	LTTNG_timer_delete = 244,
	LTTNG_clock_settime = 245,
	LTTNG_clock_gettime = 246,
	LTTNG_clock_getres = 247,
	LTTNG_clock_nanosleep = 248,
	LTTNG_swapcontext = 249,
	LTTNG_tgkill = 250,
	LTTNG_utimes = 251,
	LTTNG_statfs64 = 252,
	LTTNG_fstatfs64 = 253,
	LTTNG_fadvise64_64 = 254,
	LTTNG_rtas = 255,
	LTTNG_sys_debug_setcontext = 256,
	LTTNG_mbind = 259,
	LTTNG_get_mempolicy = 260,
	LTTNG_set_mempolicy = 261,
	LTTNG_mq_open = 262,
	LTTNG_mq_unlink = 263,
	LTTNG_mq_timedsend = 264,
	LTTNG_mq_timedreceive = 265,
	LTTNG_mq_notify = 266,
	LTTNG_mq_getsetattr = 267,
	LTTNG_kexec_load = 268,
	LTTNG_add_key = 269,
	LTTNG_request_key = 270,
	LTTNG_keyctl = 271,
	LTTNG_waitid = 272,
	LTTNG_ioprio_set = 273,
	LTTNG_ioprio_get = 274,
	LTTNG_inotify_init = 275,
	LTTNG_inotify_add_watch = 276,
	LTTNG_inotify_rm_watch = 277,
	LTTNG_spu_run = 278,
	LTTNG_spu_create = 279,
	LTTNG_pselect6 = 280,
	LTTNG_ppoll = 281,
	LTTNG_unshare = 282,
	LTTNG_splice = 283,
	LTTNG_tee = 284,
	LTTNG_vmsplice = 285,
	LTTNG_openat = 286,
	LTTNG_mkdirat = 287,
	LTTNG_mknodat = 288,
	LTTNG_fchownat = 289,
	LTTNG_futimesat = 290,
	LTTNG_fstatat64 = 291,
	LTTNG_unlinkat = 292,
	LTTNG_renameat = 293,
	LTTNG_linkat = 294,
	LTTNG_symlinkat = 295,
	LTTNG_readlinkat = 296,
	LTTNG_fchmodat = 297,
	LTTNG_faccessat = 298,
	LTTNG_get_robust_list = 299,
	LTTNG_set_robust_list = 300,
	LTTNG_ltt_trace_generic = 301,
	LTTNG_ltt_register_generic = 302,
};

/* Event syscall_entry structures */

/* Event syscall_entry logging function */
static inline void trace_kernel_arch_syscall_entry(
		enum lttng_syscall_name lttng_param_syscall_id,
		const void * lttng_param_address)
{
	unsigned int index;
	struct ltt_channel_struct *channel;
	struct ltt_trace_struct *trace;
	void *transport_data;
	char *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	size_t align;
	const char *real_from;
	const char **from = &real_from;
	u64 tsc;
	size_t before_hdr_pad, after_hdr_pad, header_size;

	if (ltt_traces.num_active_traces == 0)
		return;

	/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = (const char*)&lttng_param_syscall_id;
	align = sizeof(enum lttng_syscall_name);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(enum lttng_syscall_name);

	*from = (const char*)&lttng_param_address;
	align = sizeof(const void *);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(const void *);

	reserve_size = *to_base + *to + *len;
	preempt_disable();
	ltt_nesting[smp_processor_id()]++;
	index = ltt_get_index_from_facility_kernel_arch(						event_kernel_arch_syscall_entry);

	list_for_each_entry_rcu(trace, &ltt_traces.head, list) {
		if (!trace->active)
			continue;

		channel = ltt_get_channel_from_index(trace, index);

		slot_size = 0;
		buffer = ltt_reserve_slot(trace, channel, &transport_data,
			reserve_size, &slot_size, &tsc,
			&before_hdr_pad, &after_hdr_pad, &header_size);
		if (!buffer)
			continue; /* buffer full */

		*to_base = *to = *len = 0;

		ltt_write_event_header(trace, channel, buffer,
			ltt_facility_kernel_arch_E944278C, event_kernel_arch_syscall_entry,
			reserve_size, before_hdr_pad, tsc);
		*to_base += before_hdr_pad + after_hdr_pad + header_size;

		*from = (const char*)&lttng_param_syscall_id;
		align = sizeof(enum lttng_syscall_name);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(enum lttng_syscall_name);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_address;
		align = sizeof(const void *);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(const void *);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		ltt_commit_slot(channel, &transport_data, buffer, slot_size);

	}

	ltt_nesting[smp_processor_id()]--;
	preempt_enable_no_resched();
}

/* Event syscall_exit structures */

/* Event syscall_exit logging function */
static inline void trace_kernel_arch_syscall_exit(
		void)
{
	unsigned int index;
	struct ltt_channel_struct *channel;
	struct ltt_trace_struct *trace;
	void *transport_data;
	char *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	u64 tsc;
	size_t before_hdr_pad, after_hdr_pad, header_size;

	if (ltt_traces.num_active_traces == 0)
		return;

	/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	reserve_size = *to_base + *to + *len;
	preempt_disable();
	ltt_nesting[smp_processor_id()]++;
	index = ltt_get_index_from_facility_kernel_arch(						event_kernel_arch_syscall_exit);

	list_for_each_entry_rcu(trace, &ltt_traces.head, list) {
		if (!trace->active)
			continue;

		channel = ltt_get_channel_from_index(trace, index);

		slot_size = 0;
		buffer = ltt_reserve_slot(trace, channel, &transport_data,
			reserve_size, &slot_size, &tsc,
			&before_hdr_pad, &after_hdr_pad, &header_size);
		if (!buffer)
			continue; /* buffer full */

		*to_base = *to = *len = 0;

		ltt_write_event_header(trace, channel, buffer,
			ltt_facility_kernel_arch_E944278C, event_kernel_arch_syscall_exit,
			reserve_size, before_hdr_pad, tsc);
		*to_base += before_hdr_pad + after_hdr_pad + header_size;

		ltt_commit_slot(channel, &transport_data, buffer, slot_size);

	}

	ltt_nesting[smp_processor_id()]--;
	preempt_enable_no_resched();
}

#endif //_LTT_FACILITY_KERNEL_ARCH_H_
