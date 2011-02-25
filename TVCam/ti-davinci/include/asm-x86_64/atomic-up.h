#ifndef __ARCH_X86_64_ATOMIC_UP__
#define __ARCH_X86_64_ATOMIC_UP__

#include <asm/alternative.h>
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
		:"=m" (v->counter)
		:"ir" (i), "m" (v->counter));
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
		:"=m" (v->counter)
		:"ir" (i), "m" (v->counter));
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
		:"=m" (v->counter), "=qm" (c)
		:"ir" (i), "m" (v->counter) : "memory");
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
		:"=m" (v->counter)
		:"m" (v->counter));
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
		:"=m" (v->counter)
		:"m" (v->counter));
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
		:"=m" (v->counter), "=qm" (c)
		:"m" (v->counter) : "memory");
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
		:"=m" (v->counter), "=qm" (c)
		:"m" (v->counter) : "memory");
	return c != 0;
}

/**
 * atomic_up_add_negative - add and test if negative
 * @i: integer value to add
 * @v: pointer of type atomic_t
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
		:"=m" (v->counter), "=qm" (c)
		:"ir" (i), "m" (v->counter) : "memory");
	return c;
}

/**
 * atomic_up_add_return - add and return
 * @i: integer value to add
 * @v: pointer of type atomic_t
 *
 * Atomically adds @i to @v and returns @i + @v
 */
static __inline__ int atomic_up_add_return(int i, atomic_t *v)
{
	int __i = i;
	__asm__ __volatile__(
		"xaddl %0, %1;"
		:"=r"(i)
		:"m"(v->counter), "0"(i));
	return i + __i;
}

static __inline__ int atomic_up_sub_return(int i, atomic_t *v)
{
	return atomic_up_add_return(-i,v);
}

#define atomic_up_inc_return(v)  (atomic_up_add_return(1,v))
#define atomic_up_dec_return(v)  (atomic_up_sub_return(1,v))

/**
 * atomic64_up_add - add integer to atomic64 variable
 * @i: integer value to add
 * @v: pointer to type atomic64_t
 *
 * Atomically adds @i to @v.
 */
static __inline__ void atomic64_up_add(long i, atomic64_t *v)
{
	__asm__ __volatile__(
		"addq %1,%0"
		:"=m" (v->counter)
		:"ir" (i), "m" (v->counter));
}

/**
 * atomic64_up_sub - subtract the atomic64 variable
 * @i: integer value to subtract
 * @v: pointer to type atomic64_t
 *
 * Atomically subtracts @i from @v.
 */
static __inline__ void atomic64_up_sub(long i, atomic64_t *v)
{
	__asm__ __volatile__(
		"subq %1,%0"
		:"=m" (v->counter)
		:"ir" (i), "m" (v->counter));
}

/**
 * atomic64_up_sub_and_test - subtract value from variable and test result
 * @i: integer value to subtract
 * @v: pointer to type atomic64_t
 *
 * Atomically subtracts @i from @v and returns
 * true if the result is zero, or false for all
 * other cases.
 */
static __inline__ int atomic64_up_sub_and_test(long i, atomic64_t *v)
{
	unsigned char c;

	__asm__ __volatile__(
		"subq %2,%0; sete %1"
		:"=m" (v->counter), "=qm" (c)
		:"ir" (i), "m" (v->counter) : "memory");
	return c;
}

/**
 * atomic64_up_inc - increment atomic64 variable
 * @v: pointer to type atomic64_t
 *
 * Atomically increments @v by 1.
 */
static __inline__ void atomic64_up_inc(atomic64_t *v)
{
	__asm__ __volatile__(
		"incq %0"
		:"=m" (v->counter)
		:"m" (v->counter));
}

/**
 * atomic64_up_dec - decrement atomic64 variable
 * @v: pointer to type atomic64_t
 *
 * Atomically decrements @v by 1.
 */
static __inline__ void atomic64_up_dec(atomic64_t *v)
{
	__asm__ __volatile__(
		"decq %0"
		:"=m" (v->counter)
		:"m" (v->counter));
}

/**
 * atomic64_up_dec_and_test - decrement and test
 * @v: pointer to type atomic64_t
 *
 * Atomically decrements @v by 1 and
 * returns true if the result is 0, or false for all other
 * cases.
 */
static __inline__ int atomic64_up_dec_and_test(atomic64_t *v)
{
	unsigned char c;

	__asm__ __volatile__(
		"decq %0; sete %1"
		:"=m" (v->counter), "=qm" (c)
		:"m" (v->counter) : "memory");
	return c != 0;
}

/**
 * atomic64_up_inc_and_test - increment and test
 * @v: pointer to type atomic64_t
 *
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.
 */
static __inline__ int atomic64_up_inc_and_test(atomic64_t *v)
{
	unsigned char c;

	__asm__ __volatile__(
		"incq %0; sete %1"
		:"=m" (v->counter), "=qm" (c)
		:"m" (v->counter) : "memory");
	return c != 0;
}

/**
 * atomic64_up_add_negative - add and test if negative
 * @i: integer value to add
 * @v: pointer to type atomic64_t
 *
 * Atomically adds @i to @v and returns true
 * if the result is negative, or false when
 * result is greater than or equal to zero.
 */
static __inline__ int atomic64_up_add_negative(long i, atomic64_t *v)
{
	unsigned char c;

	__asm__ __volatile__(
		"addq %2,%0; sets %1"
		:"=m" (v->counter), "=qm" (c)
		:"ir" (i), "m" (v->counter) : "memory");
	return c;
}

/**
 * atomic64_up_add_return - add and return
 * @i: integer value to add
 * @v: pointer to type atomic64_t
 *
 * Atomically adds @i to @v and returns @i + @v
 */
static __inline__ long atomic64_up_add_return(long i, atomic64_t *v)
{
	long __i = i;
	__asm__ __volatile__(
		"xaddq %0, %1;"
		:"=r"(i)
		:"m"(v->counter), "0"(i));
	return i + __i;
}

static __inline__ long atomic64_up_sub_return(long i, atomic64_t *v)
{
	return atomic64_up_add_return(-i,v);
}

#define atomic64_up_inc_return(v)  (atomic64_up_add_return(1,v))
#define atomic64_up_dec_return(v)  (atomic64_up_sub_return(1,v))

#define atomic_up_cmpxchg(v, old, new) ((int)cmpxchg_up(&((v)->counter), \
	old, new))
/* Always has a lock prefix anyway */
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
		old = atomic_up_cmpxchg((v), c, c + (a));		\
		if (likely(old == c))				\
			break;					\
		c = old;					\
	}							\
	c != (u);						\
})
#define atomic_up_inc_not_zero(v) atomic_up_add_unless((v), 1, 0)

/* These are x86-specific, used by some header files */
#define atomic_up_clear_mask(mask, addr) \
__asm__ __volatile__("andl %0,%1" \
: : "r" (~(mask)),"m" (*addr) : "memory")

#define atomic_up_set_mask(mask, addr) \
__asm__ __volatile__("orl %0,%1" \
: : "r" ((unsigned)mask),"m" (*(addr)) : "memory")

#endif
