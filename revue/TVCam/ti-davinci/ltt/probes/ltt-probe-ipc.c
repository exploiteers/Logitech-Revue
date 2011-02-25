/* ltt-probe-ipc.c
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
#include <ltt/ltt-facility-select-default.h>
#include <ltt/ltt-facility-ipc.h>

#define IPC_MSG_CREATE_FORMAT "%d %d"
void probe_ipc_msg_create(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	int id;
	int flags;

	/* Assign args */
	va_start(ap, format);
	id = va_arg(ap, typeof(id));
	flags = va_arg(ap, typeof(flags));

	/* Call tracer */
	trace_ipc_msg_create(id, flags);
	
	va_end(ap);
}

#define IPC_SEM_CREATE_FORMAT "%d %d"
void probe_ipc_sem_create(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	int id;
	int flags;

	/* Assign args */
	va_start(ap, format);
	id = va_arg(ap, typeof(id));
	flags = va_arg(ap, typeof(flags));

	/* Call tracer */
	trace_ipc_sem_create(id, flags);
	
	va_end(ap);
}

#define IPC_SHM_CREATE_FORMAT "%d %d"
void probe_ipc_shm_create(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	int id;
	int flags;

	/* Assign args */
	va_start(ap, format);
	id = va_arg(ap, typeof(id));
	flags = va_arg(ap, typeof(flags));

	/* Call tracer */
	trace_ipc_shm_create(id, flags);
	
	va_end(ap);
}


static int __init probe_init(void)
{
	int result;
	result = marker_set_probe("ipc_msg_create",
			IPC_MSG_CREATE_FORMAT,
			probe_ipc_msg_create);
	if (!result)
		goto cleanup;
	result = marker_set_probe("ipc_sem_create",
			IPC_SEM_CREATE_FORMAT,
			probe_ipc_sem_create);
	if (!result)
		goto cleanup;
	result = marker_set_probe("ipc_shm_create",
			IPC_SHM_CREATE_FORMAT,
			probe_ipc_shm_create);
	if (!result)
		goto cleanup;

	return 0;

cleanup:
	marker_remove_probe(probe_ipc_msg_create);
	marker_remove_probe(probe_ipc_sem_create);
	marker_remove_probe(probe_ipc_shm_create);
	return -EPERM;
}

static void __exit probe_fini(void)
{
	marker_remove_probe(probe_ipc_msg_create);
	marker_remove_probe(probe_ipc_sem_create);
	marker_remove_probe(probe_ipc_shm_create);
}


module_init(probe_init);
module_exit(probe_fini);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("IPC Probe");

