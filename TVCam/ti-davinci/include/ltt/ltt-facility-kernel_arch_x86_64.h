#ifndef _LTT_FACILITY_KERNEL_ARCH_H_
#define _LTT_FACILITY_KERNEL_ARCH_H_

#include <linux/types.h>
#include <ltt/ltt-facility-id-kernel_arch_x86_64.h>
#include <ltt/ltt-tracer.h>

/* Named types */

enum lttng_syscall_name {
	LTTNG_read = 0,
	LTTNG_write = 1,
	LTTNG_open = 2,
	LTTNG_close = 3,
	LTTNG_stat = 4,
	LTTNG_fstat = 5,
	LTTNG_lstat = 6,
	LTTNG_poll = 7,
	LTTNG_lseek = 8,
	LTTNG_mmap = 9,
	LTTNG_mprotect = 10,
	LTTNG_munmap = 11,
	LTTNG_brk = 12,
	LTTNG_rt_sigaction = 13,
	LTTNG_rt_sigprocmask = 14,
	LTTNG_rt_sigreturn = 15,
	LTTNG_ioctl = 16,
	LTTNG_pread64 = 17,
	LTTNG_pwrite64 = 18,
	LTTNG_readv = 19,
	LTTNG_writev = 20,
	LTTNG_access = 21,
	LTTNG_pipe = 22,
	LTTNG_select = 23,
	LTTNG_sched_yield = 24,
	LTTNG_mremap = 25,
	LTTNG_msync = 26,
	LTTNG_mincore = 27,
	LTTNG_madvise = 28,
	LTTNG_shmget = 29,
	LTTNG_shmat = 30,
	LTTNG_shmctl = 31,
	LTTNG_dup = 32,
	LTTNG_dup2 = 33,
	LTTNG_pause = 34,
	LTTNG_nanosleep = 35,
	LTTNG_getitimer = 36,
	LTTNG_alarm = 37,
	LTTNG_setitimer = 38,
	LTTNG_getpid = 39,
	LTTNG_sendfile = 40,
	LTTNG_socket = 41,
	LTTNG_connect = 42,
	LTTNG_accept = 43,
	LTTNG_sendto = 44,
	LTTNG_recvfrom = 45,
	LTTNG_sendmsg = 46,
	LTTNG_recvmsg = 47,
	LTTNG_shutdown = 48,
	LTTNG_bind = 49,
	LTTNG_listen = 50,
	LTTNG_getsockname = 51,
	LTTNG_getpeername = 52,
	LTTNG_socketpair = 53,
	LTTNG_setsockopt = 54,
	LTTNG_getsockopt = 55,
	LTTNG_clone = 56,
	LTTNG_fork = 57,
	LTTNG_vfork = 58,
	LTTNG_execve = 59,
	LTTNG_exit = 60,
	LTTNG_wait4 = 61,
	LTTNG_kill = 62,
	LTTNG_uname = 63,
	LTTNG_semget = 64,
	LTTNG_semop = 65,
	LTTNG_semctl = 66,
	LTTNG_shmdt = 67,
	LTTNG_msgget = 68,
	LTTNG_msgsnd = 69,
	LTTNG_msgrcv = 70,
	LTTNG_msgctl = 71,
	LTTNG_fcntl = 72,
	LTTNG_flock = 73,
	LTTNG_fsync = 74,
	LTTNG_fdatasync = 75,
	LTTNG_truncate = 76,
	LTTNG_ftruncate = 77,
	LTTNG_getdents = 78,
	LTTNG_getcwd = 79,
	LTTNG_chdir = 80,
	LTTNG_fchdir = 81,
	LTTNG_rename = 82,
	LTTNG_mkdir = 83,
	LTTNG_rmdir = 84,
	LTTNG_creat = 85,
	LTTNG_link = 86,
	LTTNG_unlink = 87,
	LTTNG_symlink = 88,
	LTTNG_readlink = 89,
	LTTNG_chmod = 90,
	LTTNG_fchmod = 91,
	LTTNG_chown = 92,
	LTTNG_fchown = 93,
	LTTNG_lchown = 94,
	LTTNG_umask = 95,
	LTTNG_gettimeofday = 96,
	LTTNG_getrlimit = 97,
	LTTNG_getrusage = 98,
	LTTNG_sysinfo = 99,
	LTTNG_times = 100,
	LTTNG_ptrace = 101,
	LTTNG_getuid = 102,
	LTTNG_syslog = 103,
	LTTNG_getgid = 104,
	LTTNG_setuid = 105,
	LTTNG_setgid = 106,
	LTTNG_geteuid = 107,
	LTTNG_getegid = 108,
	LTTNG_setpgid = 109,
	LTTNG_getppid = 110,
	LTTNG_getpgrp = 111,
	LTTNG_setsid = 112,
	LTTNG_setreuid = 113,
	LTTNG_setregid = 114,
	LTTNG_getgroups = 115,
	LTTNG_setgroups = 116,
	LTTNG_setresuid = 117,
	LTTNG_getresuid = 118,
	LTTNG_setresgid = 119,
	LTTNG_getresgid = 120,
	LTTNG_getpgid = 121,
	LTTNG_setfsuid = 122,
	LTTNG_setfsgid = 123,
	LTTNG_getsid = 124,
	LTTNG_capget = 125,
	LTTNG_capset = 126,
	LTTNG_rt_sigpending = 127,
	LTTNG_rt_sigtimedwait = 128,
	LTTNG_rt_sigqueueinfo = 129,
	LTTNG_rt_sigsuspend = 130,
	LTTNG_sigaltstack = 131,
	LTTNG_utime = 132,
	LTTNG_mknod = 133,
	LTTNG_uselib_NOT_IMPLEMENTED = 134,
	LTTNG_personality = 135,
	LTTNG_ustat = 136,
	LTTNG_statfs = 137,
	LTTNG_fstatfs = 138,
	LTTNG_sysfs = 139,
	LTTNG_getpriority = 140,
	LTTNG_setpriority = 141,
	LTTNG_sched_setparam = 142,
	LTTNG_sched_getparam = 143,
	LTTNG_sched_setscheduler = 144,
	LTTNG_sched_getscheduler = 145,
	LTTNG_sched_get_priority_max = 146,
	LTTNG_sched_get_priority_min = 147,
	LTTNG_sched_rr_get_interval = 148,
	LTTNG_mlock = 149,
	LTTNG_munlock = 150,
	LTTNG_mlockall = 151,
	LTTNG_munlockall = 152,
	LTTNG_vhangup = 153,
	LTTNG_modify_ldt = 154,
	LTTNG_pivot_root = 155,
	LTTNG__sysctl = 156,
	LTTNG_prctl = 157,
	LTTNG_arch_prctl = 158,
	LTTNG_adjtimex = 159,
	LTTNG_setrlimit = 160,
	LTTNG_chroot = 161,
	LTTNG_sync = 162,
	LTTNG_acct = 163,
	LTTNG_settimeofday = 164,
	LTTNG_mount = 165,
	LTTNG_umount2 = 166,
	LTTNG_swapon = 167,
	LTTNG_swapoff = 168,
	LTTNG_reboot = 169,
	LTTNG_sethostname = 170,
	LTTNG_setdomainname = 171,
	LTTNG_iopl = 172,
	LTTNG_ioperm = 173,
	LTTNG_create_module_NOT_IMPLEMENTED = 174,
	LTTNG_init_module = 175,
	LTTNG_delete_module = 176,
	LTTNG_get_kernel_syms_NOT_IMPLEMENTED = 177,
	LTTNG_query_module_NOT_IMPLEMENTED = 178,
	LTTNG_quotactl = 179,
	LTTNG_nfsservctl = 180,
	LTTNG_getpmsg_NOT_IMPLEMENTED = 181,
	LTTNG_putpmsg_NOT_IMPLEMENTED = 182,
	LTTNG_afs_syscall_NOT_IMPLEMENTED = 183,
	LTTNG_tuxcall_NOT_IMPLEMENTED = 184,
	LTTNG_security_NOT_IMPLEMENTED = 185,
	LTTNG_gettid = 186,
	LTTNG_readahead = 187,
	LTTNG_setxattr = 188,
	LTTNG_lsetxattr = 189,
	LTTNG_fsetxattr = 190,
	LTTNG_getxattr = 191,
	LTTNG_lgetxattr = 192,
	LTTNG_fgetxattr = 193,
	LTTNG_listxattr = 194,
	LTTNG_llistxattr = 195,
	LTTNG_flistxattr = 196,
	LTTNG_removexattr = 197,
	LTTNG_lremovexattr = 198,
	LTTNG_fremovexattr = 199,
	LTTNG_tkill = 200,
	LTTNG_time = 201,
	LTTNG_futex = 202,
	LTTNG_sched_setaffinity = 203,
	LTTNG_sched_getaffinity = 204,
	LTTNG_set_thread_area_NOT_IMPLEMENTED = 205,
	LTTNG_io_setup = 206,
	LTTNG_io_destroy = 207,
	LTTNG_io_getevents = 208,
	LTTNG_io_submit = 209,
	LTTNG_io_cancel = 210,
	LTTNG_get_thread_area_NOT_IMPLEMENTED = 211,
	LTTNG_lookup_dcookie = 212,
	LTTNG_epoll_create = 213,
	LTTNG_epoll_ctl_old_NOT_IMPLEMENTED = 214,
	LTTNG_epoll_wait_old_NOT_IMPLEMENTED = 215,
	LTTNG_remap_file_pages = 216,
	LTTNG_getdents64 = 217,
	LTTNG_set_tid_address = 218,
	LTTNG_restart_syscall = 219,
	LTTNG_semtimedop = 220,
	LTTNG_fadvise64 = 221,
	LTTNG_timer_create = 222,
	LTTNG_timer_settime = 223,
	LTTNG_timer_gettime = 224,
	LTTNG_timer_getoverrun = 225,
	LTTNG_timer_delete = 226,
	LTTNG_clock_settime = 227,
	LTTNG_clock_gettime = 228,
	LTTNG_clock_getres = 229,
	LTTNG_clock_nanosleep = 230,
	LTTNG_exit_group = 231,
	LTTNG_epoll_wait = 232,
	LTTNG_epoll_ctl = 233,
	LTTNG_tgkill = 234,
	LTTNG_utimes = 235,
	LTTNG_vserver_NOT_IMPLEMENTED = 236,
	LTTNG_mbind = 237,
	LTTNG_set_mempolicy = 238,
	LTTNG_get_mempolicy = 239,
	LTTNG_mq_open = 240,
	LTTNG_mq_unlink = 241,
	LTTNG_mq_timedsend = 242,
	LTTNG_mq_timedreceive = 243,
	LTTNG_mq_notify = 244,
	LTTNG_mq_getsetattr = 245,
	LTTNG_kexec_load = 246,
	LTTNG_waitid = 247,
	LTTNG_add_key = 248,
	LTTNG_request_key = 249,
	LTTNG_keyctl = 250,
	LTTNG_ioprio_set = 251,
	LTTNG_ioprio_get = 252,
	LTTNG_inotify_init = 253,
	LTTNG_inotify_add_watch = 254,
	LTTNG_inotify_rm_watch = 255,
	LTTNG_migrate_pages = 256,
	LTTNG_openat = 257,
	LTTNG_mkdirat = 258,
	LTTNG_mknodat = 259,
	LTTNG_fchownat = 260,
	LTTNG_futimesat = 261,
	LTTNG_newfstatat = 262,
	LTTNG_unlinkat = 263,
	LTTNG_renameat = 264,
	LTTNG_linkat = 265,
	LTTNG_symlinkat = 266,
	LTTNG_readlinkat = 267,
	LTTNG_fchmodat = 268,
	LTTNG_faccessat = 269,
	LTTNG_pselect6_NOT_IMPLEMENTED = 270,
	LTTNG_ppoll_NOT_IMPLEMENTED = 271,
	LTTNG_unshare = 272,
	LTTNG_set_robust_list = 273,
	LTTNG_get_robust_list = 274,
	LTTNG_splice = 275,
	LTTNG_tee = 276,
	LTTNG_sync_file_range = 277,
	LTTNG_vmsplice = 278,
	LTTNG_move_pages = 279,
	LTTNG_ltt_trace_generic = 280,
	LTTNG_ltt_register_generic = 281,
	LTTNG_ia32_restart_syscall = 5000,
	LTTNG_ia32_exit = 5001,
	LTTNG_ia32_fork = 5002,
	LTTNG_ia32_read = 5003,
	LTTNG_ia32_write = 5004,
	LTTNG_ia32_open = 5005,
	LTTNG_ia32_close = 5006,
	LTTNG_ia32_waitpid = 5007,
	LTTNG_ia32_creat = 5008,
	LTTNG_ia32_link = 5009,
	LTTNG_ia32_unlink = 5010,
	LTTNG_ia32_execve = 5011,
	LTTNG_ia32_chdir = 5012,
	LTTNG_ia32_time = 5013,
	LTTNG_ia32_mknod = 5014,
	LTTNG_ia32_chmod = 5015,
	LTTNG_ia32_lchown = 5016,
	LTTNG_ia32_break_NOT_IMPLEMENTED = 5017,
	LTTNG_ia32_oldstat = 5018,
	LTTNG_ia32_lseek = 5019,
	LTTNG_ia32_getpid = 5020,
	LTTNG_ia32_mount = 5021,
	LTTNG_ia32_umount = 5022,
	LTTNG_ia32_setuid = 5023,
	LTTNG_ia32_getuid = 5024,
	LTTNG_ia32_stime = 5025,
	LTTNG_ia32_ptrace = 5026,
	LTTNG_ia32_alarm = 5027,
	LTTNG_ia32_oldfstat = 5028,
	LTTNG_ia32_pause = 5029,
	LTTNG_ia32_utime = 5030,
	LTTNG_ia32_stty_NOT_IMPLEMENTED = 5031,
	LTTNG_ia32_gtty_NOT_IMPLEMENTED = 5032,
	LTTNG_ia32_access = 5033,
	LTTNG_ia32_nice = 5034,
	LTTNG_ia32_ftime_NOT_IMPLEMENTED = 5035,
	LTTNG_ia32_sync = 5036,
	LTTNG_ia32_kill = 5037,
	LTTNG_ia32_rename = 5038,
	LTTNG_ia32_mkdir = 5039,
	LTTNG_ia32_rmdir = 5040,
	LTTNG_ia32_dup = 5041,
	LTTNG_ia32_pipe = 5042,
	LTTNG_ia32_times = 5043,
	LTTNG_ia32_prof_NOT_IMPLEMENTED = 5044,
	LTTNG_ia32_brk = 5045,
	LTTNG_ia32_setgid = 5046,
	LTTNG_ia32_getgid = 5047,
	LTTNG_ia32_signal = 5048,
	LTTNG_ia32_geteuid = 5049,
	LTTNG_ia32_getegid = 5050,
	LTTNG_ia32_acct = 5051,
	LTTNG_ia32_umount2 = 5052,
	LTTNG_ia32_lock_NOT_IMPLEMENTED = 5053,
	LTTNG_ia32_ioctl = 5054,
	LTTNG_ia32_fcntl = 5055,
	LTTNG_ia32_mpx_NOT_IMPLEMENTED = 5056,
	LTTNG_ia32_setpgid = 5057,
	LTTNG_ia32_ulimit_NOT_IMPLEMENTED = 5058,
	LTTNG_ia32_oldolduname = 5059,
	LTTNG_ia32_umask = 5060,
	LTTNG_ia32_chroot = 5061,
	LTTNG_ia32_ustat = 5062,
	LTTNG_ia32_dup2 = 5063,
	LTTNG_ia32_getppid = 5064,
	LTTNG_ia32_getpgrp = 5065,
	LTTNG_ia32_setsid = 5066,
	LTTNG_ia32_sigaction = 5067,
	LTTNG_ia32_sgetmask = 5068,
	LTTNG_ia32_ssetmask = 5069,
	LTTNG_ia32_setreuid = 5070,
	LTTNG_ia32_setregid = 5071,
	LTTNG_ia32_sigsuspend = 5072,
	LTTNG_ia32_sigpending = 5073,
	LTTNG_ia32_sethostname = 5074,
	LTTNG_ia32_setrlimit = 5075,
	LTTNG_ia32_getrlimit = 5076,
	LTTNG_ia32_getrusage = 5077,
	LTTNG_ia32_gettimeofday = 5078,
	LTTNG_ia32_settimeofday = 5079,
	LTTNG_ia32_getgroups = 5080,
	LTTNG_ia32_setgroups = 5081,
	LTTNG_ia32_select = 5082,
	LTTNG_ia32_symlink = 5083,
	LTTNG_ia32_oldlstat = 5084,
	LTTNG_ia32_readlink = 5085,
	LTTNG_ia32_uselib = 5086,
	LTTNG_ia32_swapon = 5087,
	LTTNG_ia32_reboot = 5088,
	LTTNG_ia32_readdir = 5089,
	LTTNG_ia32_mmap = 5090,
	LTTNG_ia32_munmap = 5091,
	LTTNG_ia32_truncate = 5092,
	LTTNG_ia32_ftruncate = 5093,
	LTTNG_ia32_fchmod = 5094,
	LTTNG_ia32_fchown = 5095,
	LTTNG_ia32_getpriority = 5096,
	LTTNG_ia32_setpriority = 5097,
	LTTNG_ia32_profil_NOT_IMPLEMENTED = 5098,
	LTTNG_ia32_statfs = 5099,
	LTTNG_ia32_fstatfs = 5100,
	LTTNG_ia32_ioperm = 5101,
	LTTNG_ia32_socketcall = 5102,
	LTTNG_ia32_syslog = 5103,
	LTTNG_ia32_setitimer = 5104,
	LTTNG_ia32_getitimer = 5105,
	LTTNG_ia32_stat = 5106,
	LTTNG_ia32_lstat = 5107,
	LTTNG_ia32_fstat = 5108,
	LTTNG_ia32_olduname = 5109,
	LTTNG_ia32_iopl = 5110,
	LTTNG_ia32_vhangup = 5111,
	LTTNG_ia32_idle_NOT_IMPLEMENTED = 5112,
	LTTNG_ia32_vm86old = 5113,
	LTTNG_ia32_wait4 = 5114,
	LTTNG_ia32_swapoff = 5115,
	LTTNG_ia32_sysinfo = 5116,
	LTTNG_ia32_ipc = 5117,
	LTTNG_ia32_fsync = 5118,
	LTTNG_ia32_sigreturn = 5119,
	LTTNG_ia32_clone = 5120,
	LTTNG_ia32_setdomainname = 5121,
	LTTNG_ia32_uname = 5122,
	LTTNG_ia32_modify_ldt = 5123,
	LTTNG_ia32_adjtimex = 5124,
	LTTNG_ia32_mprotect = 5125,
	LTTNG_ia32_sigprocmask = 5126,
	LTTNG_ia32_create_module_NOT_IMPLEMENTED = 5127,
	LTTNG_ia32_init_module = 5128,
	LTTNG_ia32_delete_module = 5129,
	LTTNG_ia32_get_kernel_syms_NOT_IMPLEMENTED = 5130,
	LTTNG_ia32_quotactl = 5131,
	LTTNG_ia32_getpgid = 5132,
	LTTNG_ia32_fchdir = 5133,
	LTTNG_ia32_bdflush_NOT_IMPLEMENTED = 5134,
	LTTNG_ia32_sysfs = 5135,
	LTTNG_ia32_personality = 5136,
	LTTNG_ia32_afs_syscall_NOT_IMPLEMENTED = 5137,
	LTTNG_ia32_setfsuid = 5138,
	LTTNG_ia32_setfsgid = 5139,
	LTTNG_ia32__llseek = 5140,
	LTTNG_ia32_getdents = 5141,
	LTTNG_ia32__newselect = 5142,
	LTTNG_ia32_flock = 5143,
	LTTNG_ia32_msync = 5144,
	LTTNG_ia32_readv = 5145,
	LTTNG_ia32_writev = 5146,
	LTTNG_ia32_getsid = 5147,
	LTTNG_ia32_fdatasync = 5148,
	LTTNG_ia32__sysctl = 5149,
	LTTNG_ia32_mlock = 5150,
	LTTNG_ia32_munlock = 5151,
	LTTNG_ia32_mlockall = 5152,
	LTTNG_ia32_munlockall = 5153,
	LTTNG_ia32_sched_setparam = 5154,
	LTTNG_ia32_sched_getparam = 5155,
	LTTNG_ia32_sched_setscheduler = 5156,
	LTTNG_ia32_sched_getscheduler = 5157,
	LTTNG_ia32_sched_yield = 5158,
	LTTNG_ia32_sched_get_priority_max = 5159,
	LTTNG_ia32_sched_get_priority_min = 5160,
	LTTNG_ia32_sched_rr_get_interval = 5161,
	LTTNG_ia32_nanosleep = 5162,
	LTTNG_ia32_mremap = 5163,
	LTTNG_ia32_setresuid = 5164,
	LTTNG_ia32_getresuid = 5165,
	LTTNG_ia32_vm86 = 5166,
	LTTNG_ia32_query_module_NOT_IMPLEMENTED = 5167,
	LTTNG_ia32_poll = 5168,
	LTTNG_ia32_nfsservctl = 5169,
	LTTNG_ia32_setresgid = 5170,
	LTTNG_ia32_getresgid = 5171,
	LTTNG_ia32_prctl = 5172,
	LTTNG_ia32_rt_sigreturn = 5173,
	LTTNG_ia32_rt_sigaction = 5174,
	LTTNG_ia32_rt_sigprocmask = 5175,
	LTTNG_ia32_rt_sigpending = 5176,
	LTTNG_ia32_rt_sigtimedwait = 5177,
	LTTNG_ia32_rt_sigqueueinfo = 5178,
	LTTNG_ia32_rt_sigsuspend = 5179,
	LTTNG_ia32_pread = 5180,
	LTTNG_ia32_pwrite = 5181,
	LTTNG_ia32_chown = 5182,
	LTTNG_ia32_getcwd = 5183,
	LTTNG_ia32_capget = 5184,
	LTTNG_ia32_capset = 5185,
	LTTNG_ia32_sigaltstack = 5186,
	LTTNG_ia32_sendfile = 5187,
	LTTNG_ia32_getpmsg_NOT_IMPLEMENTED = 5188,
	LTTNG_ia32_putpmsg_NOT_IMPLEMENTED = 5189,
	LTTNG_ia32_vfork = 5190,
	LTTNG_ia32_ugetrlimit = 5191,
	LTTNG_ia32_mmap2 = 5192,
	LTTNG_ia32_truncate64 = 5193,
	LTTNG_ia32_ftruncate64 = 5194,
	LTTNG_ia32_stat64 = 5195,
	LTTNG_ia32_lstat64 = 5196,
	LTTNG_ia32_fstat64 = 5197,
	LTTNG_ia32_lchown32 = 5198,
	LTTNG_ia32_getuid32 = 5199,
	LTTNG_ia32_getgid32 = 5200,
	LTTNG_ia32_geteuid32 = 5201,
	LTTNG_ia32_getegid32 = 5202,
	LTTNG_ia32_setreuid32 = 5203,
	LTTNG_ia32_setregid32 = 5204,
	LTTNG_ia32_getgroups32 = 5205,
	LTTNG_ia32_setgroups32 = 5206,
	LTTNG_ia32_fchown32 = 5207,
	LTTNG_ia32_setresuid32 = 5208,
	LTTNG_ia32_getresuid32 = 5209,
	LTTNG_ia32_setresgid32 = 5210,
	LTTNG_ia32_getresgid32 = 5211,
	LTTNG_ia32_chown32 = 5212,
	LTTNG_ia32_setuid32 = 5213,
	LTTNG_ia32_setgid32 = 5214,
	LTTNG_ia32_setfsuid32 = 5215,
	LTTNG_ia32_setfsgid32 = 5216,
	LTTNG_ia32_pivot_root = 5217,
	LTTNG_ia32_mincore = 5218,
	LTTNG_ia32_madvise = 5219,
	LTTNG_ia32_madvise1 = 5219,
	LTTNG_ia32_getdents64 = 5220,
	LTTNG_ia32_fcntl64 = 5221,
	LTTNG_ia32_tuxcall_NOT_IMPLEMENTED = 5222,
	LTTNG_ia32_security_NOT_IMPLEMENTED = 5223,
	LTTNG_ia32_gettid = 5224,
	LTTNG_ia32_readahead = 5225,
	LTTNG_ia32_setxattr = 5226,
	LTTNG_ia32_lsetxattr = 5227,
	LTTNG_ia32_fsetxattr = 5228,
	LTTNG_ia32_getxattr = 5229,
	LTTNG_ia32_lgetxattr = 5230,
	LTTNG_ia32_fgetxattr = 5231,
	LTTNG_ia32_listxattr = 5232,
	LTTNG_ia32_llistxattr = 5233,
	LTTNG_ia32_flistxattr = 5234,
	LTTNG_ia32_removexattr = 5235,
	LTTNG_ia32_lremovexattr = 5236,
	LTTNG_ia32_fremovexattr = 5237,
	LTTNG_ia32_tkill = 5238,
	LTTNG_ia32_sendfile64 = 5239,
	LTTNG_ia32_futex = 5240,
	LTTNG_ia32_sched_setaffinity = 5241,
	LTTNG_ia32_sched_getaffinity = 5242,
	LTTNG_ia32_set_thread_area = 5243,
	LTTNG_ia32_get_thread_area = 5244,
	LTTNG_ia32_io_setup = 5245,
	LTTNG_ia32_io_destroy = 5246,
	LTTNG_ia32_io_getevents = 5247,
	LTTNG_ia32_io_submit = 5248,
	LTTNG_ia32_io_cancel = 5249,
	LTTNG_ia32_fadvise64 = 5250,
	LTTNG_ia32_set_zone_reclaim_NOT_IMPLEMENTED = 5251,
	LTTNG_ia32_exit_group = 5252,
	LTTNG_ia32_lookup_dcookie = 5253,
	LTTNG_ia32_sys_epoll_create = 5254,
	LTTNG_ia32_sys_epoll_ctl = 5255,
	LTTNG_ia32_sys_epoll_wait = 5256,
	LTTNG_ia32_remap_file_pages = 5257,
	LTTNG_ia32_set_tid_address = 5258,
	LTTNG_ia32_timer_create = 5259,
	LTTNG_ia32_timer_settime = 5260,
	LTTNG_ia32_timer_gettime = 5261,
	LTTNG_ia32_timer_getoverrun = 5262,
	LTTNG_ia32_timer_delete = 5263,
	LTTNG_ia32_clock_settime = 5264,
	LTTNG_ia32_clock_gettime = 5265,
	LTTNG_ia32_clock_getres = 5266,
	LTTNG_ia32_clock_nanosleep = 5267,
	LTTNG_ia32_statfs64 = 5268,
	LTTNG_ia32_fstatfs64 = 5269,
	LTTNG_ia32_tgkill = 5270,
	LTTNG_ia32_utimes = 5271,
	LTTNG_ia32_fadvise64_64 = 5272,
	LTTNG_ia32_vserver_NOT_IMPLEMENTED = 5273,
	LTTNG_ia32_mbind = 5274,
	LTTNG_ia32_get_mempolicy = 5275,
	LTTNG_ia32_set_mempolicy = 5276,
	LTTNG_ia32_mq_open  = 5277,
	LTTNG_ia32_mq_unlink = 5278,
	LTTNG_ia32_mq_timedsend = 5279,
	LTTNG_ia32_mq_timedreceive = 5280,
	LTTNG_ia32_mq_notify = 5281,
	LTTNG_ia32_mq_getsetattr = 5282,
	LTTNG_ia32_kexec = 5283,
	LTTNG_ia32_waitid = 5284,
	LTTNG_ia32_sys_setaltroot_NOT_IMPLEMENTED = 5285,
	LTTNG_ia32_add_key = 5286,
	LTTNG_ia32_request_key = 5287,
	LTTNG_ia32_keyctl = 5288,
	LTTNG_ia32_ioprio_set = 5289,
	LTTNG_ia32_ioprio_get = 5290,
	LTTNG_ia32_inotify_init = 5291,
	LTTNG_ia32_inotify_add_watch = 5292,
	LTTNG_ia32_inotify_rm_watch = 5293,
	LTTNG_ia32_migrate_pages = 5294,
	LTTNG_ia32_openat = 5295,
	LTTNG_ia32_mkdirat = 5296,
	LTTNG_ia32_mknodat = 5297,
	LTTNG_ia32_fchownat = 5298,
	LTTNG_ia32_futimesat = 5299,
	LTTNG_ia32_fstatat64 = 5300,
	LTTNG_ia32_unlinkat = 5301,
	LTTNG_ia32_renameat = 5302,
	LTTNG_ia32_linkat = 5303,
	LTTNG_ia32_symlinkat = 5304,
	LTTNG_ia32_readlinkat = 5305,
	LTTNG_ia32_fchmodat = 5306,
	LTTNG_ia32_faccessat = 5307,
	LTTNG_ia32_pselect6_NOT_IMPLEMENTED = 5308,
	LTTNG_ia32_ppoll_NOT_IMPLEMENTED = 5309,
	LTTNG_ia32_unshare = 5310,
	LTTNG_ia32_set_robust_list = 5311,
	LTTNG_ia32_get_robust_list = 5312,
	LTTNG_ia32_splice = 5313,
	LTTNG_ia32_sync_file_range = 5314,
	LTTNG_ia32_tee = 5315,
	LTTNG_ia32_vmsplice = 5316,
	LTTNG_ia32_ltt_trace_generic = 5317,
	LTTNG_ia32_ltt_register_generic = 5318,
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
			ltt_facility_kernel_arch_842889DA, event_kernel_arch_syscall_entry,
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
			ltt_facility_kernel_arch_842889DA, event_kernel_arch_syscall_exit,
			reserve_size, before_hdr_pad, tsc);
		*to_base += before_hdr_pad + after_hdr_pad + header_size;

		ltt_commit_slot(channel, &transport_data, buffer, slot_size);

	}

	ltt_nesting[smp_processor_id()]--;
	preempt_enable_no_resched();
}

#endif //_LTT_FACILITY_KERNEL_ARCH_H_
