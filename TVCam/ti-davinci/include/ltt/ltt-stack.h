#ifndef _LTT_LTT_STACK_H
#define _LTT_LTT_STACK_H
void do_trace_process_stack_dump(struct pt_regs *regs);

void do_trace_kernel_stack_dump(struct pt_regs *regs);
#endif //_LTT_LTT_STACK_H
