#ifndef _ASM_GENERIC_ATOMIC_UP_H
#define _ASM_GENERIC_ATOMIC_UP_H

/* Uniprocessor atomic operations.
 *
 * The generic version of up-only atomic ops falls back on atomic.h.
 */

#include <asm/atomic.h>

#define atomic_up_add atomic_add
#define atomic_up_sub atomic_sub
#define atomic_up_add_return atomic_add_return
#define atomic_up_sub_return atomic_sub_return
#define atomic_up_sub_if_positive atomic_sub_if_positive
#define atomic_up_cmpxchg atomic_cmpxchg
#define atomic_up_xchg atomic_xchg
#define atomic_up_add_unless atomic_add_unless
#define atomic_up_inc_not_zero atomic_inc_not_zero
#define atomic_up_dec_return atomic_dec_return
#define atomic_up_inc_return atomic_inc_return
#define atomic_up_sub_and_test atomic_sub_and_test
#define atomic_up_inc_and_test atomic_inc_and_test
#define atomic_up_dec_and_test atomic_dec_and_test
#define atomic_up_dec_if_positive atomic_dec_if_positive
#define atomic_up_inc atomic_inc
#define atomic_up_dec atomic_dec
#define atomic_up_add_negative atomic_add_negative

#ifdef CONFIG_64BIT

#define atomic64_up_add atomic64_add
#define atomic64_up_sub atomic64_sub
#define atomic64_up_add_return atomic64_add_return
#define atomic64_up_sub_return atomic64_sub_return
#define atomic64_up_sub_if_positive atomic64_sub_if_positive
#define atomic64_up_dec_return atomic64_dec_return
#define atomic64_up_inc_return atomic64_inc_return
#define atomic64_up_sub_and_test atomic64_sub_and_test
#define atomic64_up_inc_and_test atomic64_inc_and_test
#define atomic64_up_dec_and_test atomic64_dec_and_test
#define atomic64_up_dec_if_positive atomic64_dec_if_positive
#define atomic64_up_inc atomic64_inc
#define atomic64_up_dec atomic64_dec
#define atomic64_up_add_negative atomic64_add_negative

#endif /* CONFIG_64BIT */

#endif /* _ASM_GENERIC_ATOMIC_UP_H */
