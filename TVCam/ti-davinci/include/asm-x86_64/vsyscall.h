#ifndef _ASM_X86_64_VSYSCALL_H_
#define _ASM_X86_64_VSYSCALL_H_

enum vsyscall_num {
	__NR_vgettimeofday,
	__NR_vtime,
};

#define VSYSCALL_START (-10UL << 20)
#define VSYSCALL_SIZE 1024
#define VSYSCALL_END (-2UL << 20)
#define VSYSCALL_ADDR(vsyscall_nr) (VSYSCALL_START+VSYSCALL_SIZE*(vsyscall_nr))

#ifdef __KERNEL__
#include <linux/seqlock.h>

/* Definitions for CONFIG_GENERIC_TIME definitions */
#define __section_vsyscall_gtod_data __attribute__ ((unused, __section__ (".vsyscall_gtod_data"),aligned(16)))
#define __vsyscall_fn __attribute__ ((unused,__section__(".vsyscall_fn")))


#define hpet_readl(a)           readl((const void __iomem *)fix_to_virt(FIX_HPET_BASE) + a)
#define hpet_writel(d,a)        writel(d, (void __iomem *)fix_to_virt(FIX_HPET_BASE) + a)

/* kernel space (writeable) */
extern struct timezone sys_tz;
extern raw_seqlock_t xtime_lock;
extern struct vsyscall_gtod_data_t vsyscall_gtod_data;

#endif /* __KERNEL__ */

#endif /* _ASM_X86_64_VSYSCALL_H_ */
