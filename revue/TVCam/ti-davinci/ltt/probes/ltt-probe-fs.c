/* ltt-probe-fs.c
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
#include <ltt/ltt-tracer.h>
#include <ltt/ltt-facility-select-default.h>
#include <ltt/ltt-facility-fs.h>
#include <ltt/ltt-facility-custom-fs_data.h>
#include <ltt/ltt-facility-fs_data.h>

#define FS_CLOSE_FORMAT "%u"
void probe_fs_close(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned int fd;

	/* Assign args */
	va_start(ap, format);
	fd = va_arg(ap, typeof(fd));

	/* Call tracer */
	trace_fs_close(fd);
	
	va_end(ap);
}

#define FS_OPEN_FORMAT "%d %s"
void probe_fs_open(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned int fd;
	lttng_sequence_fs_open_filename filename;

	/* Assign args */
	va_start(ap, format);
	fd = va_arg(ap, typeof(fd));
	filename.array = va_arg(ap, typeof(filename.array));
	filename.len = strlen(filename.array);

	/* Call tracer */
	trace_fs_open(&filename, fd);
	
	va_end(ap);
}

#define FS_WRITEV_FORMAT "%lu %lu"
void probe_fs_writev(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned long fd;
	unsigned long vlen;

	/* Assign args */
	va_start(ap, format);
	fd = va_arg(ap, typeof(fd));
	vlen = va_arg(ap, typeof(vlen));

	/* Call tracer */
	trace_fs_writev(fd, vlen);
	
	va_end(ap);
}

#define FS_READV_FORMAT "%lu %lu"
void probe_fs_readv(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned long fd;
	unsigned long vlen;

	/* Assign args */
	va_start(ap, format);
	fd = va_arg(ap, typeof(fd));
	vlen = va_arg(ap, typeof(vlen));

	/* Call tracer */
	trace_fs_readv(fd, vlen);
	
	va_end(ap);
}

#define FS_LSEEK_FORMAT "%u off_t %ld %u"
void probe_fs_lseek(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned int fd;
	off_t offset;
	unsigned int origin;

	/* Assign args */
	va_start(ap, format);
	fd = va_arg(ap, typeof(fd));
	offset = va_arg(ap, typeof(offset));
	origin = va_arg(ap, typeof(origin));

	/* Call tracer */
	trace_fs_lseek(fd, offset, origin);
	
	va_end(ap);
}

#define FS_LLSEEK_FORMAT "%u loff_t %lld %u"
void probe_fs_llseek(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned int fd;
	loff_t offset;
	unsigned int origin;

	/* Assign args */
	va_start(ap, format);
	fd = va_arg(ap, typeof(fd));
	offset = va_arg(ap, typeof(offset));
	origin = va_arg(ap, typeof(origin));

	/* Call tracer */
	trace_fs_lseek(fd, offset, origin);
	
	va_end(ap);
}

#define FS_READ_FORMAT "%u %zu"
void probe_fs_read(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned int fd;
	size_t count;

	/* Assign args */
	va_start(ap, format);
	fd = va_arg(ap, typeof(fd));
	count = va_arg(ap, typeof(count));

	/* Call tracer */
	trace_fs_read(fd, count);
	
	va_end(ap);
}

#define FS_READ_RET_FORMAT "%u %zd __user %p"
void probe_fs_read_ret(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned int fd;
	size_t count;
	char __user * buf;

	/* Assign args */
	va_start(ap, format);
	fd = va_arg(ap, typeof(fd));
	count = va_arg(ap, typeof(count));
	buf = va_arg(ap, typeof(buf));

	/* Call tracer */
	if (count > 0) {
		lttng_sequence_fs_data_read_data lttng_data;
		lttng_data.len = min((size_t)LTT_LOG_RW_SIZE, count);
		lttng_data.array = buf;
		trace_fs_data_read(fd, count, &lttng_data);
	}
	
	va_end(ap);
}

#define FS_WRITE_FORMAT "%u %zu"
void probe_fs_write(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned int fd;
	size_t count;

	/* Assign args */
	va_start(ap, format);
	fd = va_arg(ap, typeof(fd));
	count = va_arg(ap, typeof(count));

	/* Call tracer */
	trace_fs_write(fd, count);
	
	va_end(ap);
}

#define FS_WRITE_RET_FORMAT "%u %zd __user %p"
void probe_fs_write_ret(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned int fd;
	size_t count;
	char __user * buf;

	/* Assign args */
	va_start(ap, format);
	fd = va_arg(ap, typeof(fd));
	count = va_arg(ap, typeof(count));
	buf = va_arg(ap, typeof(buf));

	/* Call tracer */
	if (count > 0) {
		lttng_sequence_fs_data_write_data lttng_data;
		lttng_data.len = min((size_t)LTT_LOG_RW_SIZE, count);
		lttng_data.array = buf;
		trace_fs_data_write(fd, count, &lttng_data);
	}
	
	va_end(ap);
}

#define FS_PREAD64_FORMAT "%u %zu loff_t %lld"
void probe_fs_pread64(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned int fd;
	size_t count;
	loff_t pos;

	/* Assign args */
	va_start(ap, format);
	fd = va_arg(ap, typeof(fd));
	count = va_arg(ap, typeof(count));
	pos = va_arg(ap, typeof(pos));

	/* Call tracer */
	trace_fs_pread64(fd, count, pos);
	
	va_end(ap);
}

#define FS_PWRITE64_FORMAT "%u %zu loff_t %lld"
void probe_fs_pwrite64(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned int fd;
	size_t count;
	loff_t pos;

	/* Assign args */
	va_start(ap, format);
	fd = va_arg(ap, typeof(fd));
	count = va_arg(ap, typeof(count));
	pos = va_arg(ap, typeof(pos));

	/* Call tracer */
	trace_fs_pwrite64(fd, count, pos);
	
	va_end(ap);
}


#define FS_BUFFER_WAIT_START_FORMAT "%p"
void probe_fs_buffer_wait_start(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	const void *address;

	/* Assign args */
	va_start(ap, format);
	address = va_arg(ap, typeof(address));

	/* Call tracer */
	trace_fs_buf_wait_start(address);
	
	va_end(ap);
}

#define FS_BUFFER_WAIT_END_FORMAT "%p"
void probe_fs_buffer_wait_end(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	const void *address;

	/* Assign args */
	va_start(ap, format);
	address = va_arg(ap, typeof(address));

	/* Call tracer */
	trace_fs_buf_wait_end(address);
	
	va_end(ap);
}

#define FS_EXEC_FORMAT "%s"
void probe_fs_exec(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	const char *name;

	lttng_sequence_fs_exec_filename filename;

	/* Assign args */
	va_start(ap, format);
	name = va_arg(ap, typeof(name));
	filename.array = name;
	filename.len = strlen(name);

	/* Call tracer */
	trace_fs_exec(&filename);
	
	va_end(ap);
}

#define FS_IOCTL_FORMAT "%u %u %lu"
void probe_fs_ioctl(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned int fd;
	unsigned int cmd;
	unsigned long arg;

	/* Assign args */
	va_start(ap, format);
	fd = va_arg(ap, typeof(fd));
	cmd = va_arg(ap, typeof(cmd));
	arg = va_arg(ap, typeof(arg));

	/* Call tracer */
	trace_fs_ioctl(fd, cmd, arg);
	
	va_end(ap);
}

#define FS_POLLFD_FORMAT "%d"
void probe_fs_pollfd(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	int fd;

	/* Assign args */
	va_start(ap, format);
	fd = va_arg(ap, typeof(fd));

	/* Call tracer */
	trace_fs_poll(fd);
	
	va_end(ap);
}

#define FS_SELECT_FORMAT "%d %lld"
void probe_fs_select(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	int fd;
	long long timeout;

	/* Assign args */
	va_start(ap, format);
	fd = va_arg(ap, typeof(fd));
	timeout = va_arg(ap, typeof(timeout));

	/* Call tracer */
	trace_fs_select(fd, timeout);
	
	va_end(ap);
}

static int __init probe_init(void)
{
	int result;
	result = marker_set_probe("fs_close",
			FS_CLOSE_FORMAT,
			probe_fs_close);
	if (!result)
		goto cleanup;
	result = marker_set_probe("fs_open",
			FS_OPEN_FORMAT,
			probe_fs_open);
	if (!result)
		goto cleanup;
	result = marker_set_probe("fs_writev",
			FS_WRITEV_FORMAT,
			probe_fs_writev);
	if (!result)
		goto cleanup;
	result = marker_set_probe("fs_readv",
			FS_READV_FORMAT,
			probe_fs_readv);
	if (!result)
		goto cleanup;
	result = marker_set_probe("fs_lseek",
			FS_LSEEK_FORMAT,
			probe_fs_lseek);
	if (!result)
		goto cleanup;
	result = marker_set_probe("fs_llseek",
			FS_LLSEEK_FORMAT,
			probe_fs_llseek);
	if (!result)
		goto cleanup;
	result = marker_set_probe("fs_read",
			FS_READ_FORMAT,
			probe_fs_read);
	if (!result)
		goto cleanup;
	result = marker_set_probe("fs_read_ret",
			FS_READ_RET_FORMAT,
			probe_fs_read_ret);
	if (!result)
		goto cleanup;
	result = marker_set_probe("fs_write",
			FS_WRITE_FORMAT,
			probe_fs_write);
	if (!result)
		goto cleanup;
	result = marker_set_probe("fs_write_ret",
			FS_WRITE_RET_FORMAT,
			probe_fs_write_ret);
	if (!result)
		goto cleanup;
	result = marker_set_probe("fs_pread64",
			FS_PREAD64_FORMAT,
			probe_fs_pread64);
	if (!result)
		goto cleanup;
	result = marker_set_probe("fs_pwrite64",
			FS_PWRITE64_FORMAT,
			probe_fs_pwrite64);
	if (!result)
		goto cleanup;
	result = marker_set_probe("fs_buffer_wait_start",
			FS_BUFFER_WAIT_START_FORMAT,
			probe_fs_buffer_wait_start);
	if (!result)
		goto cleanup;
	result = marker_set_probe("fs_buffer_wait_end",
			FS_BUFFER_WAIT_END_FORMAT,
			probe_fs_buffer_wait_end);
	if (!result)
		goto cleanup;
	result = marker_set_probe("fs_exec",
			FS_EXEC_FORMAT,
			probe_fs_exec);
	if (!result)
		goto cleanup;
	result = marker_set_probe("fs_ioctl",
			FS_IOCTL_FORMAT,
			probe_fs_ioctl);
	if (!result)
		goto cleanup;
	result = marker_set_probe("fs_pollfd",
			FS_POLLFD_FORMAT,
			probe_fs_pollfd);
	if (!result)
		goto cleanup;
	result = marker_set_probe("fs_select",
			FS_SELECT_FORMAT,
			probe_fs_select);
	if (!result)
		goto cleanup;

	return 0;

cleanup:
	marker_remove_probe(probe_fs_close);
	marker_remove_probe(probe_fs_open);
	marker_remove_probe(probe_fs_writev);
	marker_remove_probe(probe_fs_readv);
	marker_remove_probe(probe_fs_lseek);
	marker_remove_probe(probe_fs_llseek);
	marker_remove_probe(probe_fs_read);
	marker_remove_probe(probe_fs_read_ret);
	marker_remove_probe(probe_fs_write);
	marker_remove_probe(probe_fs_write_ret);
	marker_remove_probe(probe_fs_pread64);
	marker_remove_probe(probe_fs_pwrite64);
	marker_remove_probe(probe_fs_buffer_wait_start);
	marker_remove_probe(probe_fs_buffer_wait_end);
	marker_remove_probe(probe_fs_exec);
	marker_remove_probe(probe_fs_ioctl);
	marker_remove_probe(probe_fs_pollfd);
	marker_remove_probe(probe_fs_select);
	return -EPERM;
}

static void __exit probe_fini(void)
{	
	marker_remove_probe(probe_fs_close);
	marker_remove_probe(probe_fs_open);
	marker_remove_probe(probe_fs_writev);
	marker_remove_probe(probe_fs_readv);
	marker_remove_probe(probe_fs_lseek);
	marker_remove_probe(probe_fs_llseek);
	marker_remove_probe(probe_fs_read);
	marker_remove_probe(probe_fs_read_ret);
	marker_remove_probe(probe_fs_write);
	marker_remove_probe(probe_fs_write_ret);
	marker_remove_probe(probe_fs_pread64);
	marker_remove_probe(probe_fs_pwrite64);
	marker_remove_probe(probe_fs_buffer_wait_start);
	marker_remove_probe(probe_fs_buffer_wait_end);
	marker_remove_probe(probe_fs_exec);
	marker_remove_probe(probe_fs_ioctl);
	marker_remove_probe(probe_fs_pollfd);
	marker_remove_probe(probe_fs_select);
}


module_init(probe_init);
module_exit(probe_fini);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("FS Probe");

