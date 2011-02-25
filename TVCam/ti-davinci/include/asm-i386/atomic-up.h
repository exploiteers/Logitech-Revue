#ifndef __ARCH_I386_ATOMIC_UP__
#define __ARCH_I386_ATOMIC_UP__

#include <linux/compiler.h>
#include <asm/processor.h>
#include <asm/atomic.h>

/* 
 * atomic_up variants insure operation atomicity only if the variable is not
 * shared between cpus. This is useful to have per-cpu atomic operations to
 * protect from contexts like non-maskable interrupts without the LOCK prefix
 * performance cost.
 */

/**
 * atomic_up_add - add integer to atomic variable
 * @i: integer value to add
 * @v: pointer of type atomic_t
 * 
 * Atomically adds @i to @v.
 */
static __inline__ void atomic_up_add(int i, atomic_t *v)
{
	__asm__ __volatile__(
		"addl %1,%0"
		:"+m" (v->counter)
		:"ir" (i));
}

/**
 * atomic_up_sub - subtract the atomic variable
 * @i: integer value to subtract
 * @v: pointer of type atomic_t
 * 
 * Atomically subtracts @i from @v.
 */
static __inline__ void atomic_up_sub(int i, atomic_t *v)
{
	__asm__ __volatile__(
		"subl %1,%0"
		:"+m" (v->counter)
		:"ir" (i));
}

/**
 * atomic_up_sub_and_test - subtract value from variable and test result
 * @i: integer value to subtract
 * @v: pointer of type atomic_t
 * 
 * Atomically subtracts @i from @v and returns
 * true if the result is zero, or false for all
 * other cases.
 */
static __inline__ int atomic_up_sub_and_test(int i, atomic_t *v)
{
	unsigned char c;

	__asm__ __volatile__(
		"subl %2,%0; sete %1"
		:"+m" (v->counter), "=qm" (c)
		:"ir" (i) : "memory");
	return c;
}

/**
 * atomic_up_inc - increment atomic variable
 * @v: pointer of type atomic_t
 * 
 * Atomically increments @v by 1.
 */ 
static __inline__ void atomic_up_inc(atomic_t *v)
{
	__asm__ __volatile__(
		"incl %0"
		:"+m" (v->counter));
}

/**
 * atomic_up_dec - decrement atomic variable
 * @v: pointer of type atomic_t
 * 
 * Atomically decrements @v by 1.
 */ 
static __inline__ void atomic_up_dec(atomic_t *v)
{
	__asm__ __volatile__(
		"decl %0"
		:"+m" (v->counter));
}

/**
 * atomic_up_dec_and_test - decrement and test
 * @v: pointer of type atomic_t
 * 
 * Atomically decrements @v by 1 and
 * returns true if the result is 0, or false for all other
 * cases.
 */ 
static __inline__ int atomic_up_dec_and_test(atomic_t *v)
{
	unsigned char c;

	__asm__ __volatile__(
		"decl %0; sete %1"
		:"+m" (v->counter), "=qm" (c)
		: : "memory");
	return c != 0;
}

/**
 * atomic_up_inc_and_test - increment and test 
 * @v: pointer of type atomic_t
 * 
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.
 */ 
static __inline__ int atomic_up_inc_and_test(atomic_t *v)
{
	unsigned char c;

	__asm__ __volatile__(
		"incl %0; sete %1"
		:"+m" (v->counter), "=qm" (c)
		: : "memory");
	return c != 0;
}

/**
 * atomic_up_add_negative - add and test if negative
 * @v: pointer of type atomic_t
 * @i: integer value to add
 * 
 * Atomically adds @i to @v and returns true
 * if the result is negative, or false when
 * result is greater than or equal to zero.
 */ 
static __inline__ int atomic_up_add_negative(int i, atomic_t *v)
{
	unsigned char c;

	__asm__ __volatile__(
		"addl %2,%0; sets %1"
		:"+m" (v->counter), "=qm" (c)
		:"ir" (i) : "memory");
	return c;
}

/**
 * atomic_up_add_return - add and return
 * @v: pointer of type atomic_t
 * @i: integer value to add
 *
 * Atomically adds @i to @v and returns @i + @v
 */
static __inline__ int atomic_up_add_return(int i, atomic_t *v)
{
	int __i;
#ifdef CONFIG_M386
	unsigned long flags;
	if(unlikely(boot_cpu_data.x86==3))
		goto no_xadd;
#endif
	/* Modern 486+ processor */
	__i = i;
	__asm__ __volatile__(
		"xaddl %0, %1;"
		:"=r"(i)
		:"m"(v->counter), "0"(i));
	return i + __i;

#ifdef CONFIG_M386
no_xadd: /* Legacy 386 processor */
	local_irq_save(flags);
	__i = atomic_up_read(v);
	atomic_up_set(v, i + __i);
	local_irq_restore(flags);
	return i + __i;
#endif
}

static __inline__ int atomic_up_sub_return(int i, atomic_t *v)
{
	return atomic_up_add_return(-i,v);
}

#define atomic_up_cmpxchg(v, old, new) ((int)cmpxchg_up(&((v)->counter), \
	old, new))
/* xchg always has a LOCK prefix */
#define atomic_up_xchg(v, new) (xchg(&((v)->counter), new))

/**
 * atomic_up_add_unless - add unless the number is a given value
 * @v: pointer of type atomic_t
 * @a: the amount to add to v...
 * @u: ...unless v is equal to u.
 *
 * Atomically adds @a to @v, so long as it was not @u.
 * Returns non-zero if @v was not @u, and zero otherwise.
 */
#define atomic_up_add_unless(v, a, u)				\
({								\
	int c, old;						\
	c = atomic_read(v);					\
	for (;;) {						\
		if (unlikely(c == (u)))				\
			break;					\
		old = atomic_up_cmpxchg((v), c, c + (a));	\
		if (likely(old == c))				\
			break;					\
		c = old;					\
	}							\
	c != (u);						\
})
#define atomic_up_inc_not_zero(v) atomic_up_add_unless((v), 1, 0)

#define atomic_up_inc_return(v)  (atomic_up_add_return(1,v))
#define atomic_up_dec_return(v)  (atomic_up_sub_return(1,v))

/* These are x86-specific, used by some header files */
#define atomic_up_clear_mask(mask, addr) \
__asm__ __volatile__("andl %0,%1" \
: : "r" (~(mask)),"m" (*addr) : "memory")

#define atomic_up_set_mask(mask, addr) \
__asm__ __volatile__("orl %0,%1" \
: : "r" (mask),"m" (*(addr)) : "memory")

#endif
