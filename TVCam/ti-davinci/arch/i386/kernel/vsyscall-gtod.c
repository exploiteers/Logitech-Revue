#include <linux/time.h>
#include <linux/timer.h>
#include <linux/clocksource.h>
#include <linux/sched.h>
#include <asm/vsyscall-gtod.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/fixmap.h>
#include <asm/msr.h>
#include <asm/timer.h>
#include <asm/system.h>
#include <asm/unistd.h>
#include <asm/errno.h>

struct vsyscall_gtod_data_t {
	struct timeval wall_time_tv;
	struct timezone sys_tz;
	struct clocksource clock;
};

struct vsyscall_gtod_data_t vsyscall_gtod_data;
struct vsyscall_gtod_data_t __vsyscall_gtod_data __section_vsyscall_gtod_data;

seqlock_t vsyscall_gtod_lock = SEQLOCK_UNLOCKED;
seqlock_t __vsyscall_gtod_lock __section_vsyscall_gtod_lock = SEQLOCK_UNLOCKED;

int errno;
static inline _syscall2(int,gettimeofday,struct timeval *,tv,struct timezone *,tz);

static int vsyscall_mapped = 0; /* flag variable for remap_vsyscall() */
extern struct timezone sys_tz;

static inline void do_vgettimeofday(struct timeval* tv)
{
	cycle_t now, cycle_delta;
	uint64_t nsec_delta;

	if (!__vsyscall_gtod_data.clock.vread) {
		gettimeofday(tv, NULL);
		return;
	}

	/* read the clocksource and calc cycle_delta */
	now = __vsyscall_gtod_data.clock.vread();
	cycle_delta = (now - __vsyscall_gtod_data.clock.cycle_last)
				& __vsyscall_gtod_data.clock.mask;

	/* convert cycles to nsecs */
	nsec_delta = cycle_delta * __vsyscall_gtod_data.clock.mult;
	nsec_delta = nsec_delta >> __vsyscall_gtod_data.clock.shift;

	/* add nsec offset to wall_time_tv */
	*tv = __vsyscall_gtod_data.wall_time_tv;
	while (nsec_delta >= NSEC_PER_SEC) {
		tv->tv_sec += 1;
		nsec_delta -= NSEC_PER_SEC;
	}
	tv->tv_usec += ((unsigned long)nsec_delta) / NSEC_PER_USEC;
}

static inline void do_get_tz(struct timezone *tz)
{
	*tz = __vsyscall_gtod_data.sys_tz;
}

static int  __vsyscall(0) asmlinkage vgettimeofday(struct timeval *tv, struct timezone *tz)
{
	unsigned long seq;
	do {
		seq = read_seqbegin(&__vsyscall_gtod_lock);

		if (tv)
			do_vgettimeofday(tv);
		if (tz)
			do_get_tz(tz);

	} while (read_seqretry(&__vsyscall_gtod_lock, seq));

	return 0;
}

static time_t __vsyscall(1) asmlinkage vtime(time_t * t)
{
	struct timeval tv;
	vgettimeofday(&tv,NULL);
	if (t)
		*t = tv.tv_sec;
	return tv.tv_sec;
}

struct clocksource* curr_clock;

void update_vsyscall(struct timespec *wall_time,
				struct clocksource* clock)
{
	unsigned long flags;

	write_seqlock_irqsave(&vsyscall_gtod_lock, flags);

	/* XXX - hackitty hack hack. this is terrible! */
	if (curr_clock != clock) {
		curr_clock = clock;
	}

	/* save off wall time as timeval */
	vsyscall_gtod_data.wall_time_tv.tv_sec = wall_time->tv_sec;
	vsyscall_gtod_data.wall_time_tv.tv_usec = wall_time->tv_nsec/1000;

	/* copy current clocksource */
	vsyscall_gtod_data.clock = *clock;

	/* save off current timezone */
	vsyscall_gtod_data.sys_tz = sys_tz;

	write_sequnlock_irqrestore(&vsyscall_gtod_lock, flags);

}
extern char __vsyscall_0;

static void __init map_vsyscall(void)
{
	unsigned long physaddr_page0 = (unsigned long) &__vsyscall_0 - PAGE_OFFSET;

	/* Initially we map the VSYSCALL page w/ PAGE_KERNEL permissions to
	 * keep the alternate_instruction code from bombing out when it
	 * changes the seq_lock memory barriers in vgettimeofday()
	 */
	__set_fixmap(FIX_VSYSCALL_GTOD_FIRST_PAGE, physaddr_page0, PAGE_KERNEL);
}

static int __init remap_vsyscall(void)
{
	unsigned long physaddr_page0 = (unsigned long) &__vsyscall_0 - PAGE_OFFSET;

	if (!vsyscall_mapped)
		return 0;

	/* Remap the VSYSCALL page w/ PAGE_KERNEL_VSYSCALL permissions
	 * after the alternate_instruction code has run
	 */
	clear_fixmap(FIX_VSYSCALL_GTOD_FIRST_PAGE);
	__set_fixmap(FIX_VSYSCALL_GTOD_FIRST_PAGE, physaddr_page0, PAGE_KERNEL_VSYSCALL);

	return 0;
}

int __init vsyscall_init(void)
{
	printk("VSYSCALL: consistency checks...");
	if ((unsigned long) &vgettimeofday != VSYSCALL_ADDR(__NR_vgettimeofday)) {
		printk("vgettimeofday link addr broken\n");
		printk("VSYSCALL: vsyscall_init failed!\n");
		return -EFAULT;
	}
	if ((unsigned long) &vtime != VSYSCALL_ADDR(__NR_vtime)) {
		printk("vtime link addr broken\n");
		printk("VSYSCALL: vsyscall_init failed!\n");
		return -EFAULT;
	}
	if (VSYSCALL_ADDR(0) != __fix_to_virt(FIX_VSYSCALL_GTOD_FIRST_PAGE)) {
		printk("fixmap first vsyscall 0x%lx should be 0x%x\n",
			__fix_to_virt(FIX_VSYSCALL_GTOD_FIRST_PAGE),
			VSYSCALL_ADDR(0));
		printk("VSYSCALL: vsyscall_init failed!\n");
		return -EFAULT;
	}


	printk("passed...mapping...");
	map_vsyscall();
	printk("done.\n");
	vsyscall_mapped = 1;
	printk("VSYSCALL: fixmap virt addr: 0x%lx\n",
		__fix_to_virt(FIX_VSYSCALL_GTOD_FIRST_PAGE));

	return 0;
}
__initcall(remap_vsyscall);
