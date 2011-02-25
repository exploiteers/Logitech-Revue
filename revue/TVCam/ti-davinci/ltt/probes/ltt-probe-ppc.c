/* ltt-probe-ppc.c
 *
 * Loads a function at a marker call site.
 *
 * (C) Copyright 2006 Mathieu Desnoyers <mathieu.desnoyers@polymtl.ca>
 *
 * This file is released under the GPLv2.
 * See the file COPYING for more details.
 */

#include <linux/marker.h>
#include <linux/module.h>
#include <linux/ptrace.h>
#include <linux/interrupt.h>
#include <ltt/ltt-facility-select-default.h>
#include <ltt/ltt-facility-select-kernel.h>
#include <ltt/ltt-facility-kernel.h>
#include <ltt/ltt-facility-ipc.h>
#include <ltt/ltt-facility-memory.h>
#include <ltt/ltt-facility-kernel_arch_ppc.h>

#define KERNEL_TRAP_ENTRY_FORMAT "%ld struct pt_regs %p"
void probe_kernel_trap_entry(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	long trap_id;
	struct pt_regs *regs;
	long address;

	/* Assign args */
	va_start(ap, format);
	trap_id = va_arg(ap, typeof(trap_id));
	regs = va_arg(ap, typeof(regs));

	/* Extra args */
	if (regs)
		address = instruction_pointer(regs);
	else
		address = 0;

	/* Call tracer */
	trace_kernel_trap_entry(trap_id, (void*)address);
	
	/* TODO : Call other instrumentation for regs */

	va_end(ap);
}

#define KERNEL_TRAP_EXIT_FORMAT MARK_NOARGS
void probe_kernel_trap_exit(const char *format, ...)
{
	/* Call tracer */
	trace_kernel_trap_exit();
}

#define KERNEL_SYSCALL_ENTRY_FORMAT "%ld struct pt_regs %p"
void probe_kernel_syscall_entry(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	long syscall_id;
	long address;
	struct pt_regs *regs;

	/* Assign args */
	va_start(ap, format);
	syscall_id = va_arg(ap, typeof(syscall_id));
	regs = va_arg(ap, typeof(regs));

	/* Extra args */
	if (regs)
		address = instruction_pointer(regs);
	else
		address = 0;

	/* Call tracer */
	trace_kernel_arch_syscall_entry(syscall_id, (void*)address);
	
	/* TODO : Call other instrumentation for regs */

	va_end(ap);
}

#define KERNEL_SYSCALL_EXIT_FORMAT MARK_NOARGS
void probe_kernel_syscall_exit(const char *format, ...)
{
	/* Call tracer */
	trace_kernel_arch_syscall_exit();
}

#define IPC_CALL_FORMAT "%u %d"
void probe_ipc_call(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned int call_number;
	int first;

	/* Assign args */
	va_start(ap, format);
	call_number = va_arg(ap, typeof(call_number));
	first = va_arg(ap, typeof(first));

	/* Call tracer */
	trace_ipc_call(call_number, first);
	
	va_end(ap);
}

#define MM_HANDLE_FAULT_ENTRY_FORMAT "%lu %ld"
void probe_mm_handle_fault_entry(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned long address;
	long eip;

	/* Extra info */.
	int trap_id = ((current)->thread.regs? (current)->thread.regs->trap: 0);

	/* Assign args */
	va_start(ap, format);
	address = va_arg(ap, typeof(address));
	eip = va_arg(ap, typeof(eip));

	/* Call tracer */
	trace_memory_page_fault(address, eip);
	
	va_end(ap);
}

#define MM_HANDLE_FAULT_EXIT_FORMAT MARK_NOARGS
void probe_mm_handle_fault_exit(const char *format, ...)
{
}




int __init probe_init(void)
{
	int result;
	result = marker_set_probe("kernel_trap_entry",
			KERNEL_TRAP_ENTRY_FORMAT,
			probe_kernel_trap_entry);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_trap_exit",
			KERNEL_TRAP_EXIT_FORMAT,
			probe_kernel_trap_exit);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_syscall_entry",
			KERNEL_SYSCALL_ENTRY_FORMAT,
			probe_kernel_syscall_entry);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_syscall_exit",
			KERNEL_SYSCALL_EXIT_FORMAT,
			probe_kernel_syscall_exit);
	if (!result)
		goto cleanup;
	result = marker_set_probe("ipc_call",
			IPC_CALL_FORMAT,
			probe_ipc_call);
	if (!result)
		goto cleanup;
	result = marker_set_probe("mm_handle_fault_entry",
			MM_HANDLE_FAULT_ENTRY_FORMAT,
			probe_mm_handle_fault_entry);
	if (!result)
		goto cleanup;
	result = marker_set_probe("mm_handle_fault_exit",
			MM_HANDLE_FAULT_EXIT_FORMAT,
			probe_mm_handle_fault_exit);
	if (!result)
		goto cleanup;

	return 0;

cleanup:
	marker_remove_probe(probe_kernel_trap_entry);
	marker_remove_probe(probe_kernel_trap_exit);
	marker_remove_probe(probe_kernel_syscall_entry);
	marker_remove_probe(probe_kernel_syscall_exit);
	marker_remove_probe(probe_ipc_call);
	marker_remove_probe(probe_mm_handle_fault_entry);
	marker_remove_probe(probe_mm_handle_fault_exit);
	return -EPERM;
}

void __exit probe_fini(void)
{
	marker_remove_probe(probe_kernel_trap_entry);
	marker_remove_probe(probe_kernel_trap_exit);
	marker_remove_probe(probe_kernel_syscall_entry);
	marker_remove_probe(probe_kernel_syscall_exit);
	marker_remove_probe(probe_ipc_call);
	marker_remove_probe(probe_mm_handle_fault_entry);
	marker_remove_probe(probe_mm_handle_fault_exit);
}

module_init(probe_init);
module_exit(probe_fini);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("ppc Probe");

