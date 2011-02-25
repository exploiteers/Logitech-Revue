
#include <ltt/ltt-facility-select-default.h>
#include <ltt/ltt-facility-custom-stack.h>
#include <linux/module.h>

void do_trace_process_stack_dump(struct pt_regs *regs)
{
	trace_process_stack_dump(regs);
}
EXPORT_SYMBOL(do_trace_process_stack_dump);

void do_trace_kernel_stack_dump(struct pt_regs *regs)
{
	trace_kernel_stack_dump(regs);
}
EXPORT_SYMBOL(do_trace_kernel_stack_dump);
