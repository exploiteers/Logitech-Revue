/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994 - 2000, 2001, 2003 Ralf Baechle
 * Copyright (C) 1999, 2000 Silicon Graphics, Inc.
 * Copyright (C) 2001 MIPS Technologies, Inc.
 */

#include <asm/asm.h>
#include <asm/asmmacro.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/stackframe.h>
#include <asm/isadep.h>
#include <asm/thread_info.h>
#include <asm/war.h>
#ifdef CONFIG_MIPS_MT_SMTC
#include <asm/mipsmtregs.h>
#endif

#ifdef CONFIG_PREEMPT
	.macro	preempt_stop
	.endm
#else
	.macro	preempt_stop
	raw_local_irq_disable
	.endm
#define resume_kernel	restore_all
#endif

	.text
	.align	5
FEXPORT(ret_from_exception)
	preempt_stop
FEXPORT(ret_from_irq)
	LONG_L	t0, PT_STATUS(sp)		# returning to kernel mode?
	andi	t0, t0, KU_USER
	beqz	t0, resume_kernel

resume_userspace:
	raw_local_irq_disable	# make sure we dont miss an
					# interrupt setting need_resched
					# between sampling and return
	LONG_L	a2, TI_FLAGS($28)	# current->work
	andi	t0, a2, _TIF_WORK_MASK	# (ignoring syscall_trace)
	bnez	t0, work_pending
	j	restore_all

#ifdef CONFIG_LATENCY_TRACE

#ifdef CONFIG_32BIT

#define PTR_ADDU_STRING add
#define PTR_SUBU_STRING	sub

#else

#define PTR_ADDU_STRING daddu
#define PTR_SUBU_STRING dsubu

#endif

FEXPORT(mcount)
	.set noreorder
	.set noat
	lw t0, mcount_enabled
	beqz t0, out
	PTR_SUBU_STRING $29,$29,96
	LONG_S $4,24($29)
        LONG_S $5,32($29)
        LONG_S $6,40($29)
        LONG_S $7,48($29)
        LONG_S $8,56($29)
        LONG_S $9,64($29)
        LONG_S $10,72($29)
        LONG_S $11,80($29)
        LONG_S $2,16($29)
        LONG_S $1,0($29)
        LONG_S $31,8($29)
        move $5,$31
        move $4,$1
	jal __trace
	nop;
        LONG_L $4,24($29)
        LONG_L $5,32($29)
        LONG_L $6,40($29)
        LONG_L $7,48($29)
        LONG_L $8,56($29)
        LONG_L $9,64($29)
        LONG_L $10,72($29)
        LONG_L $11,80($29)
        LONG_L $2,16($29)
        LONG_L $31,8($29)
        LONG_L $1,0($29)
	PTR_ADDU_STRING $29,$29,96
	j $31
	move $31,$1;
out:
	.set reorder;
	.set at
#endif
	

#ifdef CONFIG_PREEMPT
resume_kernel:
	raw_local_irq_disable
	lw	t0, kernel_preemption
	beqz	t0, restore_all
	lw	t0, TI_PRE_COUNT($28)
	bnez	t0, restore_all
need_resched:
	LONG_L	t0, TI_FLAGS($28)
	andi	t1, t0, _TIF_NEED_RESCHED
	beqz	t1, restore_all
	LONG_L	t0, PT_STATUS(sp)		# Interrupts off?
	andi	t0, 1
	beqz	t0, restore_all
	raw_local_irq_disable
	jal	preempt_schedule_irq
	sw      zero, TI_PRE_COUNT($28)
	b	need_resched
#endif

FEXPORT(ret_from_fork)
	jal	schedule_tail		# a0 = struct task_struct *prev

FEXPORT(syscall_exit)
	raw_local_irq_disable	# make sure need_resched and
					# signals dont change between
					# sampling and return
	LONG_L	a2, TI_FLAGS($28)	# current->work
	li	t0, _TIF_ALLWORK_MASK
	and	t0, a2, t0
	bnez	t0, syscall_exit_work

FEXPORT(restore_all)			# restore full frame
#ifdef CONFIG_MIPS_MT_SMTC
/* Detect and execute deferred IPI "interrupts" */
	move	a0,sp
	jal	deferred_smtc_ipi
/* Re-arm any temporarily masked interrupts not explicitly "acked" */
	mfc0	v0, CP0_TCSTATUS
	ori	v1, v0, TCSTATUS_IXMT
	mtc0	v1, CP0_TCSTATUS
	andi	v0, TCSTATUS_IXMT
	_ehb
	mfc0	t0, CP0_TCCONTEXT
	DMT	9				# dmt t1
	jal	mips_ihb
	mfc0	t2, CP0_STATUS
	andi	t3, t0, 0xff00
	or	t2, t2, t3
	mtc0	t2, CP0_STATUS
	_ehb
	andi	t1, t1, VPECONTROL_TE
	beqz	t1, 1f
	EMT
1:
	mfc0	v1, CP0_TCSTATUS
	/* We set IXMT above, XOR should clear it here */
	xori	v1, v1, TCSTATUS_IXMT
	or	v1, v0, v1
	mtc0	v1, CP0_TCSTATUS
	_ehb
	xor	t0, t0, t3
	mtc0	t0, CP0_TCCONTEXT
#endif /* CONFIG_MIPS_MT_SMTC */
	.set	noat
	RESTORE_TEMP
	RESTORE_AT
	RESTORE_STATIC
FEXPORT(restore_partial)		# restore partial frame
#ifdef CONFIG_TRACE_IRQFLAGS
	SAVE_STATIC
	SAVE_AT
	SAVE_TEMP
	LONG_L	v0, PT_STATUS(sp)
	and	v0, 1
	beqz	v0, 1f
	jal	trace_hardirqs_on
	b	2f
1:	jal	trace_hardirqs_off
2:
	RESTORE_TEMP
	RESTORE_AT
	RESTORE_STATIC
#endif
	RESTORE_SOME
	RESTORE_SP_AND_RET
	.set	at

work_pending:
					# a2 is preloaded with TI_FLAGS
	andi	t0, a2, (_TIF_NEED_RESCHED|_TIF_NEED_RESCHED_DELAYED)
	beqz	t0, work_notifysig
work_resched:
	raw_local_irq_enable  t0
	jal	schedule

	raw_local_irq_disable	# make sure need_resched and
					# signals dont change between
					# sampling and return
	LONG_L	a2, TI_FLAGS($28)
	andi	t0, a2, _TIF_WORK_MASK	# is there any work to be done
					# other than syscall tracing?
	beqz	t0, restore_all
	andi	t0, a2, (_TIF_NEED_RESCHED|_TIF_NEED_RESCHED_DELAYED)
	bnez	t0, work_resched

work_notifysig:				# deal with pending signals and
					# notify-resume requests
	move	a0, sp
	li	a1, 0
	jal	do_notify_resume	# a2 already loaded
	j	resume_userspace

FEXPORT(syscall_exit_work_partial)
	SAVE_STATIC
syscall_exit_work:
	li	t0, _TIF_SYSCALL_TRACE | _TIF_SYSCALL_AUDIT
	and	t0, a2			# a2 is preloaded with TI_FLAGS
	beqz	t0, work_pending	# trace bit set?
	raw_local_irq_enable	# could let do_syscall_trace()
					# call schedule() instead
	move	a0, sp
	li	a1, 1
	jal	do_syscall_trace
	b	resume_userspace

#if defined(CONFIG_CPU_MIPSR2) || defined(CONFIG_MIPS_MT)

/*
 * MIPS32R2 Instruction Hazard Barrier - must be called
 *
 * For C code use the inline version named instruction_hazard().
 */
LEAF(mips_ihb)
	.set	mips32r2
	jr.hb	ra
	nop
	END(mips_ihb)

#endif /* CONFIG_CPU_MIPSR2 or CONFIG_MIPS_MT */
