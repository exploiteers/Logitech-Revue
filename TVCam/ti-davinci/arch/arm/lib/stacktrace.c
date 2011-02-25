#include <linux/sched.h>
#include <linux/stacktrace.h>

struct stackframe {
	unsigned long fp;
	unsigned long sp;
	unsigned long lr;
	unsigned long pc;
};

int walk_stackframe(unsigned long fp, unsigned long low, unsigned long high,
		    int (*fn)(struct stackframe *, void *), void *data)
{
	struct stackframe *frame;

	do {
		/*
		 * Check current frame pointer is within bounds
		 */
		if ((fp - 12) < low || fp + 4 >= high)
			break;

		frame = (struct stackframe *)(fp - 12);

		if (fn(frame, data))
			break;

		/*
		 * Update the low bound - the next frame must always
		 * be at a higher address than the current frame.
		 */
		low = fp + 4;
		fp = frame->fp;
	} while (fp);

	return 0;
}

struct stack_trace_data {
	struct stack_trace *trace;
	unsigned int skip;
};

static int save_trace(struct stackframe *frame, void *d)
{
	struct stack_trace_data *data = d;
	struct stack_trace *trace = data->trace;

	if (data->skip) {
		data->skip--;
		return 0;
	}

	trace->entries[trace->nr_entries++] = frame->lr;

	return trace->nr_entries >= trace->max_entries;
}

void save_stack_trace(struct stack_trace *trace, struct task_struct *task,
		      int all_contexts, unsigned int skip)
{
	struct stack_trace_data data;
	unsigned long fp, base;

	data.trace = trace;
	data.skip = skip;

	if (task) {
		base = (unsigned long)task_stack_page(task);
		fp = 0;
	} else {
		base = (unsigned long)task_stack_page(current);
		asm("mov %0, fp" : "=r" (fp));
	}

	walk_stackframe(fp, base, base + THREAD_SIZE, save_trace, &data);
}
