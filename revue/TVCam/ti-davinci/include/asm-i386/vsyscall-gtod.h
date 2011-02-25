#ifndef _ASM_i386_VSYSCALL_GTOD_H_
#define _ASM_i386_VSYSCALL_GTOD_H_

#ifdef CONFIG_GENERIC_TIME_VSYSCALL

/* VSYSCALL_GTOD_START must be the same as
 * __fix_to_virt(FIX_VSYSCALL_GTOD FIRST_PAGE)
 * and must also be same as addr in vmlinux.lds.S */
#define VSYSCALL_GTOD_START 0xffffd000
#define VSYSCALL_GTOD_SIZE 1024
#define VSYSCALL_GTOD_END (VSYSCALL_GTOD_START + PAGE_SIZE)
#define VSYSCALL_GTOD_NUMPAGES \
	((VSYSCALL_GTOD_END-VSYSCALL_GTOD_START) >> PAGE_SHIFT)
#define VSYSCALL_ADDR(vsyscall_nr) \
	(VSYSCALL_GTOD_START+VSYSCALL_GTOD_SIZE*(vsyscall_nr))

#ifdef __KERNEL__
#ifndef __ASSEMBLY__
#include <linux/seqlock.h>
#define __vsyscall(nr) __attribute__ ((unused,__section__(".vsyscall_" #nr)))

#define __vsyscall_fn __attribute__ ((unused,__section__(".vsyscall_fn")))
#define __vsyscall_data __attribute__ ((unused,__section__(".vsyscall_data")))

/* ReadOnly generic time value attributes*/
#define __section_vsyscall_gtod_data __attribute__ ((unused, __section__ (".vsyscall_gtod_data")))

#define __section_vsyscall_gtod_lock __attribute__ ((unused, __section__ (".vsyscall_gtod_lock")))


enum vsyscall_num {
	__NR_vgettimeofday,
	__NR_vtime,
};

int vsyscall_init(void);
extern char __vsyscall_0;
#endif /* __ASSEMBLY__ */
#endif /* __KERNEL__ */
#else /* CONFIG_GENERIC_TIME_VSYSCALL */

#define __vsyscall(nr)
#define __vsyscall_fn
#define __vsyscall_data

#define vsyscall_init()
#define vsyscall_set_timesource(x)
#endif /* CONFIG_GENERIC_TIME_VSYSCALL */
#endif /* _ASM_i386_VSYSCALL_GTOD_H_ */
