/*
 * ltt-relay.c
 *
 * (C) Copyright 2005-2006 - Mathieu Desnoyers (mathieu.desnoyers@polymtl.ca)
 *
 * Contains the kernel code for the Linux Trace Toolkit.
 *
 * Author:
 *	Mathieu Desnoyers (mathieu.desnoyers@polymtl.ca)
 *
 * Inspired from LTT :
 *	Karim Yaghmour (karim@opersys.com)
 *	Tom Zanussi (zanussi@us.ibm.com)
 *	Bob Wisniewski (bob@watson.ibm.com)
 * And from K42 :
 *  Bob Wisniewski (bob@watson.ibm.com)
 *
 * Changelog:
 *  19/10/05, Complete lockless mechanism. (Mathieu Desnoyers)
 *	27/05/05, Modular redesign and rewrite. (Mathieu Desnoyers)

 * Comments :
 * num_active_traces protects the functors. Changing the pointer is an atomic
 * operation, but the functions can only be called when in tracing. It is then
 * safe to unload a module in which sits a functor when no tracing is active.
 *
 * filter_control functor is protected by incrementing its module refcount.
 *
 *
 */

#include <linux/config.h>
#include <linux/time.h>
#include <ltt/ltt-tracer.h>
#include <linux/relay.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/ltt-facilities.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/bitops.h>
#include <linux/fs.h>
#include <linux/smp_lock.h>
#include <linux/debugfs.h>
#include <linux/stat.h>
#include <asm/atomic.h>
#include <asm/atomic-up.h>

static struct dentry *ltt_root_dentry;
static struct file_operations ltt_file_operations;

/* How a force_switch must be done ?
 *
 * Is it done during tracing or as a final flush after tracing
 * (so it won't write in the new sub-buffer).
 */
enum force_switch_mode { FORCE_ACTIVE, FORCE_FLUSH };


/* Trace callbacks */

static void ltt_buffer_begin_callback(struct rchan_buf *buf,
			u64 tsc, unsigned int subbuf_idx)
{
	struct ltt_channel_struct *channel =
		(struct ltt_channel_struct*)buf->chan->private_data;
	struct ltt_block_start_header *header =
		(struct ltt_block_start_header*)
			(buf->start + (subbuf_idx*buf->chan->subbuf_size));

	header->begin.cycle_count = tsc;
	header->begin.freq = ltt_frequency();
	header->lost_size = 0xFFFFFFFF; // for debugging...
	header->buf_size = buf->chan->subbuf_size;
	ltt_write_trace_header(channel->trace, &header->trace);
}

static void ltt_buffer_end_callback(struct rchan_buf *buf,
			u64 tsc, unsigned int offset, unsigned int subbuf_idx)
{
	struct ltt_block_start_header *header =
		(struct ltt_block_start_header*)
			(buf->start + (subbuf_idx*buf->chan->subbuf_size));

	/* offset is assumed to never be 0 here : never deliver a completely
	 * empty subbuffer.
	 * The lost size is between 0 and subbuf_size-1 */
	header->lost_size = SUBBUF_OFFSET((buf->chan->subbuf_size - offset),
				buf->chan);
	header->end.cycle_count = tsc;
	header->end.freq = ltt_frequency();
}

static int ltt_subbuf_start_callback(struct rchan_buf *buf,
				void *subbuf,
				void *prev_subbuf,
				size_t prev_padding)
{
	return 0;
}



static void ltt_deliver(struct rchan_buf *buf,
		unsigned subbuf_idx,
		void *subbuf)
{
	struct ltt_channel_struct *channel =
		(struct ltt_channel_struct*)buf->chan->private_data;
	struct ltt_channel_buf_struct *ltt_buf = &channel->buf[buf->cpu];

	atomic_set(&ltt_buf->wakeup_readers, 1);
}

static void ltt_buf_mapped_callback(struct rchan_buf *buf,
		struct file *filp)
{
}

static void ltt_buf_unmapped_callback(struct rchan_buf *buf,
		struct file *filp)
{
}

static struct dentry *ltt_create_buf_file_callback(const char *filename,
				  struct dentry *parent,
				  int mode,
				  struct rchan_buf *buf,
				  int *is_global)
{
	return debugfs_create_file(filename, mode, parent, buf,
			&ltt_file_operations);
}

static void ltt_release_transport(struct kref *kref)
{
	struct ltt_trace_struct *trace = container_of(kref,
			struct ltt_trace_struct, ltt_transport_kref);
	debugfs_remove(trace->dentry.control_root);
	//FIXME : trace_root can be left there if there is somewhere a PWD
	//being the control_root.
	debugfs_remove(trace->dentry.trace_root);
}

static int ltt_remove_buf_file_callback(struct dentry *dentry)
{
	struct rchan_buf *buf = dentry->d_inode->u.generic_ip;
	struct ltt_channel_struct *ltt_chan = buf->chan->private_data;

	debugfs_remove(dentry);
	kref_put(&ltt_chan->trace->ltt_transport_kref, ltt_release_transport);
	ltt_buffer_destroy(ltt_chan);

	return 0;
}

/* This function should not be called from NMI interrupt context */
static void ltt_buf_unfull(struct rchan_buf *buf,
		unsigned subbuf_idx,
		void *subbuf)
{
	struct ltt_channel_struct *ltt_channel =
		(struct ltt_channel_struct*)buf->chan->private_data;
	struct ltt_channel_buf_struct *ltt_buf = &ltt_channel->buf[buf->cpu];
	if (waitqueue_active(&ltt_buf->write_wait))
		schedule_work(&ltt_buf->wake_writers);
}


/**
 *	ltt_poll - poll file op for ltt files
 *	@filp: the file
 *	@wait: poll table
 *
 *	Poll implemention.
 */
static unsigned int ltt_poll(struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;
	struct inode *inode = filp->f_dentry->d_inode;
	struct rchan_buf *buf = inode->u.generic_ip;
	struct ltt_channel_struct *ltt_channel =
		(struct ltt_channel_struct*)buf->chan->private_data;
	struct ltt_channel_buf_struct *ltt_buf = &ltt_channel->buf[buf->cpu];

	//printk(KERN_DEBUG "DEBUG : in LTT poll %p\n", filp);
	if (filp->f_mode & FMODE_READ) {
		poll_wait(filp, &buf->read_wait, wait);

		if (atomic_read(&ltt_buf->active_readers) != 0) {
			return 0;
		} else {
			if (SUBBUF_TRUNC(
				atomic_read(&ltt_buf->offset), buf->chan)
			- SUBBUF_TRUNC(
				atomic_read(&ltt_buf->consumed), buf->chan)
			== 0) {
				if (buf->finalized) return POLLHUP;
				else return 0;
			} else {
				struct rchan *rchan =
					ltt_channel->trans_channel_data;
				if (SUBBUF_TRUNC(atomic_read(&ltt_buf->offset),
						buf->chan)
				- SUBBUF_TRUNC(atomic_read(&ltt_buf->consumed),
							buf->chan)
				>= rchan->alloc_size)
					return POLLPRI | POLLRDBAND;
				else
					return POLLIN | POLLRDNORM;
			}
		}
	}
	return mask;
}


/**
 *	ltt_ioctl - ioctl control on the debugfs file
 *
 *	@inode: the inode
 *	@filp: the file
 *	@cmd: the command
 *	@arg: command arg
 *
 *	This ioctl implements three commands necessary for a minimal
 *	producer/consumer implementation :
 *	RELAY_GET_SUBBUF
 *		Get the next sub buffer that can be read. It never blocks.
 *	RELAY_PUT_SUBBUF
 *		Release the currently read sub-buffer. Parameter is the last
 *		put subbuffer (returned by GET_SUBBUF).
 *	RELAY_GET_N_BUBBUFS
 *		returns the number of sub buffers in the per cpu channel.
 *	RELAY_GET_SUBBUF_SIZE
 *		returns the size of the sub buffers.
 *
 */
static int ltt_ioctl(struct inode *inode, struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	struct rchan_buf *buf = inode->u.generic_ip;
	struct ltt_channel_struct *ltt_channel =
		(struct ltt_channel_struct*)buf->chan->private_data;
	struct ltt_channel_buf_struct *ltt_buf = &ltt_channel->buf[buf->cpu];
	u32 __user *argp = (u32 __user *)arg;

	switch (cmd) {
		case RELAY_GET_SUBBUF:
		{
			unsigned int consumed_old, consumed_idx;
			atomic_inc(&ltt_buf->active_readers);
			consumed_old = atomic_read(&ltt_buf->consumed);
			consumed_idx = SUBBUF_INDEX(consumed_old, buf->chan);
			if (SUBBUF_OFFSET(
				atomic_read(
					&ltt_buf->commit_count[consumed_idx]),
				buf->chan) != 0) {
				atomic_dec(&ltt_buf->active_readers);
				return -EAGAIN;
			}
			if ((SUBBUF_TRUNC(
				atomic_read(&ltt_buf->offset), buf->chan)
			- SUBBUF_TRUNC(consumed_old, buf->chan))
			== 0) {
				atomic_dec(&ltt_buf->active_readers);
				return -EAGAIN;
			}
			//printk(KERN_DEBUG "LTT ioctl get subbuf %d\n",
			//		consumed_old);
			return put_user((u32)consumed_old, argp);
			break;
		}
		case RELAY_PUT_SUBBUF:
		{
			u32 consumed_old;
			int ret;
			unsigned int consumed_new;

			ret = get_user(consumed_old, argp);
			if (ret)
				return ret; /* will return -EFAULT */

			//printk(KERN_DEBUG "LTT ioctl put subbuf %d\n",
			//		consumed_old);
			consumed_new = SUBBUF_ALIGN(consumed_old, buf->chan);
			spin_lock(&ltt_buf->full_lock);
			if (atomic_cmpxchg(
				&ltt_buf->consumed, consumed_old, consumed_new)
					!= consumed_old) {
				/* We have been pushed by the writer : the last
				 * buffer read _is_ corrupted! It can also
				 * happen if this is a buffer we never got. */
				atomic_dec(&ltt_buf->active_readers);
				spin_unlock(&ltt_buf->full_lock);
				return -EIO;
			} else {
				/* tell the client that buffer is now unfull */
				int index;
				void *data;
				index = SUBBUF_INDEX(consumed_old, buf->chan);
				data = buf->start +
					BUFFER_OFFSET(consumed_old, buf->chan);
				ltt_buf_unfull(buf, index, data);
				atomic_dec(&ltt_buf->active_readers);
				spin_unlock(&ltt_buf->full_lock);
			}
			break;
		}
		case RELAY_GET_N_SUBBUFS:
			//printk(KERN_DEBUG "LTT ioctl get n subbufs\n");
			return put_user((u32)buf->chan->n_subbufs, argp);
			break;
		case RELAY_GET_SUBBUF_SIZE:
			//printk(KERN_DEBUG "LTT ioctl get subbuf size\n");
			return put_user((u32)buf->chan->subbuf_size, argp);
			break;
		default:
			return -ENOIOCTLCMD;
	}
	return 0;
}

#ifdef CONFIG_COMPAT

static long ltt_compat_ioctl(struct file *file, unsigned cmd, unsigned long arg)
{
	long ret = -ENOIOCTLCMD;

	lock_kernel();
	ret = ltt_ioctl(file->f_dentry->d_inode, file, cmd, arg);
	unlock_kernel();

	return ret;
}

#endif //CONFIG_COMPAT

/* Create channel.
 */
static int ltt_relay_create_channel(char *trace_name,
		struct ltt_trace_struct *trace,
		struct dentry *dir,
		char *channel_name,
		struct ltt_channel_struct **ltt_chan,
		unsigned int subbuf_size, unsigned int n_subbufs,
		int overwrite)
{
	struct rchan *rchan;
	unsigned int i, j;
	char *tmpname;
	int err = 0;

	tmpname = kmalloc(PATH_MAX, GFP_KERNEL);
	if (!tmpname)
		return EPERM;
	if (overwrite) {
		strncpy(tmpname, LTT_FLIGHT_PREFIX, PATH_MAX-1);
		strncat(tmpname, channel_name,
			PATH_MAX-1-sizeof(LTT_FLIGHT_PREFIX));
	} else {
		strncpy(tmpname, channel_name, PATH_MAX-1);
	}

	*ltt_chan = kzalloc(sizeof(struct ltt_channel_struct), GFP_KERNEL);
	if (!(*ltt_chan))
		goto ltt_chan_alloc_error;
	kref_init(&(*ltt_chan)->kref);

	(*ltt_chan)->trace = trace;
	(*ltt_chan)->trans_channel_data = relay_open(tmpname,
			dir,
			subbuf_size,
			n_subbufs,
			&trace->callbacks);
	
	if ((*ltt_chan)->trans_channel_data == NULL) {
		printk(KERN_ERR "LTT : Can't open %s channel for trace %s\n",
				tmpname, trace_name);
		goto relay_open_error;
	}

	(*ltt_chan)->buffer_begin = ltt_buffer_begin_callback;
	(*ltt_chan)->buffer_end = ltt_buffer_end_callback;
	(*ltt_chan)->overwrite = overwrite;
	rchan = (*ltt_chan)->trans_channel_data;

	rchan->private_data = (*ltt_chan);

	strncpy((*ltt_chan)->channel_name, tmpname, PATH_MAX-1);


	for (i = 0; i < NR_CPUS; i++) {
		(*ltt_chan)->buf[i].commit_count =
			kmalloc(sizeof(atomic_t) * n_subbufs, GFP_KERNEL);
		if (!(*ltt_chan)->buf[i].commit_count)
			goto relay_open_error;
	}

	for (i = 0; i < NR_CPUS; i++) {
		kref_get(&trace->kref);
		kref_get(&trace->ltt_transport_kref);
		kref_get(&(*ltt_chan)->kref);
		atomic_set(&(*ltt_chan)->buf[i].offset,
			ltt_subbuf_header_len());
		atomic_set(&(*ltt_chan)->buf[i].consumed, 0);
		atomic_set(&(*ltt_chan)->buf[i].active_readers, 0);
		for (j = 0; j < n_subbufs; j++)
			atomic_set(&(*ltt_chan)->buf[i].commit_count[j], 0);
		init_waitqueue_head(&(*ltt_chan)->buf[i].write_wait);
		atomic_set(&(*ltt_chan)->buf[i].wakeup_readers, 0);
		INIT_WORK(&(*ltt_chan)->buf[i].wake_writers,
				ltt_wakeup_writers, &(*ltt_chan)->buf[i]);
		spin_lock_init(&(*ltt_chan)->buf[i].full_lock);
		/* Tracing not started yet :
		 * we don't deal with concurrency (no delivery) */
		if (!rchan->buf[i])
			continue;

		ltt_buffer_begin_callback(rchan->buf[i], trace->start_tsc, 0);
		/* atomic_add made on atomic_up variable on data that belongs to
		 * various CPUs : ok because tracing not started. */
		atomic_add(ltt_subbuf_header_len(),
			&(*ltt_chan)->buf[i].commit_count[0]);
	}
	err = 0;
	goto end;

relay_open_error:
	for (i = 0; i < NR_CPUS; i++) {
		kfree((*ltt_chan)->buf[i].commit_count);
	}
	kfree(*ltt_chan);
	*ltt_chan = NULL;
ltt_chan_alloc_error:
	err = EPERM;
end:
	kfree(tmpname);
	return err;
}

static int ltt_relay_create_dirs(struct ltt_trace_struct *new_trace)
{
	new_trace->dentry.trace_root = debugfs_create_dir(new_trace->trace_name,
			ltt_root_dentry);
	if (new_trace->dentry.trace_root == NULL) {
		printk(KERN_ERR "LTT : Trace directory name %s already taken\n",
				new_trace->trace_name);
		return EEXIST;
	}

	new_trace->dentry.control_root = debugfs_create_dir(LTT_CONTROL_ROOT,
			new_trace->dentry.trace_root);
	if (new_trace->dentry.control_root == NULL) {
		printk(KERN_ERR "LTT : Trace control subdirectory name "\
				"%s/%s already taken\n",
				new_trace->trace_name, LTT_CONTROL_ROOT);
		debugfs_remove(new_trace->dentry.trace_root);
		return EEXIST;
	}

	new_trace->callbacks.subbuf_start = ltt_subbuf_start_callback;
	new_trace->callbacks.buf_mapped = ltt_buf_mapped_callback;
	new_trace->callbacks.buf_unmapped = ltt_buf_unmapped_callback;
	new_trace->callbacks.create_buf_file = ltt_create_buf_file_callback;
	new_trace->callbacks.remove_buf_file = ltt_remove_buf_file_callback;

	return 0;
}

static void ltt_relay_remove_dirs(struct ltt_trace_struct *trace)
{
	debugfs_remove(trace->dentry.control_root);
	debugfs_remove(trace->dentry.trace_root);
}

/* Force a sub-buffer switch for a per-cpu buffer. This operation is
 * completely reentrant : can be called while tracing is active with
 * absolutely no lock held.
 */
static void ltt_force_switch(struct rchan_buf *buf, enum force_switch_mode mode)
{
	struct ltt_channel_struct *ltt_channel =
			(struct ltt_channel_struct*)buf->chan->private_data;
	struct ltt_channel_buf_struct *ltt_buf = &ltt_channel->buf[buf->cpu];
	struct rchan *rchan = ltt_channel->trans_channel_data;

	u64 tsc;
	int offset_begin, offset_end, offset_old;
	int reserve_commit_diff;
	int consumed_old, consumed_new;
	int commit_count;
	int end_switch_old;

	do {
		offset_old = atomic_read(&ltt_buf->offset);
		offset_begin = offset_old;
		end_switch_old = 0;

		if (SUBBUF_OFFSET(offset_begin, buf->chan) != 0) {
			offset_begin = SUBBUF_ALIGN(offset_begin, buf->chan);
			end_switch_old = 1;
		} else {
			/* we do not have to switch : buffer is empty */
			return;
		}
		if (mode == FORCE_ACTIVE)
			offset_begin += ltt_subbuf_header_len();
		/* Always begin_switch in FORCE_ACTIVE mode */
		/* Test new buffer integrity */
		reserve_commit_diff = SUBBUF_OFFSET(
			buf->chan->subbuf_size
			- atomic_read(
			&ltt_buf->commit_count[SUBBUF_INDEX(offset_begin,
						buf->chan)]), buf->chan);
		if (reserve_commit_diff == 0) {
			/* Next buffer not corrupted. */
			if (mode == FORCE_ACTIVE && !ltt_channel->overwrite &&
				(offset_begin - atomic_read(&ltt_buf->consumed))
				>= rchan->alloc_size) {
	  			/* We do not overwrite non consumed buffers
				 * and we are full :
				 * ignore switch while tracing is active. */
				return;
			}
		} else {
			/* Next subbuffer corrupted. Force pushing reader even
			 * in normal mode */
		}
		offset_end = offset_begin;

		tsc = ltt_get_timestamp64();
		if (tsc == 0) {
			/* Error in getting the timestamp : should not happen :
			 * it would mean we are called from an NMI during a
			 * write seqlock on xtime. */
			return;
		}
	} while (atomic_up_cmpxchg(&ltt_buf->offset, offset_old, offset_end)
							!= offset_old);

	if (mode == FORCE_ACTIVE) {
		/* Push the reader if necessary */
		do {
			consumed_old = atomic_read(&ltt_buf->consumed);
			/* If buffer is in overwrite mode, push the reader
			 * consumed count if the write position has reached it
			 * and we are not at the first iteration (don't push
			 * the reader farther than the writer). This operation
			 * can be done concurrently by many writers in the same
			 * buffer, the writer being at the fartest write
			 * position sub-buffer index in the buffer being the
			 * one which will win this loop.
			 * If the buffer is not in overwrite mode, pushing the
			 * reader only happen if a sub-buffer is corrupted */
			if ((SUBBUF_TRUNC(offset_end-1, buf->chan)
					- SUBBUF_TRUNC(consumed_old,
						buf->chan))
					>= rchan->alloc_size)
				consumed_new =
					SUBBUF_ALIGN(consumed_old, buf->chan);
			else {
				consumed_new = consumed_old;
				break;
			}
		} while (atomic_cmpxchg(&ltt_buf->consumed, consumed_old,
					consumed_new) != consumed_old);

		if (consumed_old != consumed_new) {
			/* Reader pushed : we are the winner of the push, we
			 * can therefore reequilibrate reserve and commit.
			 * Atomic increment of the commit count permits other
			 * writers to play around with this variable before us.
			 * We keep track of corrupted_subbuffers even in
			 * overwrite mode :
			 * we never want to write over a non completely
			 * committed sub-buffer : possible causes : the buffer
			 * size is too low compared to the unordered data input,
			 * or there is a writer who died between the reserve
			 * and the commit. */
			if (reserve_commit_diff) {
				/* We have to alter the sub-buffer commit
				 * count : a sub-buffer is corrupted */
				atomic_up_add(reserve_commit_diff,
					&ltt_buf->commit_count[SUBBUF_INDEX(
						offset_begin, buf->chan)]);
				atomic_up_inc(&ltt_buf->corrupted_subbuffers);
			}
		}
	}

	/* Always switch */
	if (end_switch_old) {
		/* old subbuffer */
		/* Concurrency safe because we are the last and only thread to
		 * alter this sub-buffer. As long as it is not delivered and
		 * read, no other thread can alter the offset, alter the
		 * reserve_count or call the client_buffer_end_callback on this
		 * sub-buffer. The only remaining threads could be the ones
		 * with pending commits. They will have to do the deliver
		 * themself.
		 * Not concurrency safe in overwrite mode.
		 * We detect corrupted subbuffers with commit and reserve
		 * counts. We keep a corrupted sub-buffers count and push the
		 * readers across these sub-buffers. Not concurrency safe if a
		 * writer is stalled in a subbuffer and another writer switches
		 * in, finding out it's corrupted. The result will be than the
		 * old (uncommited) subbuffer will be declared corrupted, and
		 * that the new subbuffer will be declared corrupted too because
		 * of the commit count adjustment.
		 * Offset old should never be 0. */
		ltt_channel->buffer_end(buf, tsc, offset_old,
				SUBBUF_INDEX((offset_old-1), buf->chan));
		commit_count =
			atomic_up_add_return(buf->chan->subbuf_size
				- (SUBBUF_OFFSET(offset_old-1, buf->chan) + 1),
				&ltt_buf->commit_count[SUBBUF_INDEX(
						offset_old-1, buf->chan)]);
		if (SUBBUF_OFFSET(commit_count, buf->chan) == 0) {
			ltt_deliver(buf,
				SUBBUF_INDEX((offset_old-1), buf->chan), NULL);
		}
	}

	if (mode == FORCE_ACTIVE) {
		/* New sub-buffer */
		/* This code can be executed unordered : writers may already
		 * have written to the sub-buffer before this code gets
		 * executed, caution. */
		/* The commit makes sure that this code is executed before the
		 * deliver of this sub-buffer */
		ltt_channel->buffer_begin(buf, tsc,
				SUBBUF_INDEX(offset_begin, buf->chan));
		commit_count =
			atomic_up_add_return(ltt_subbuf_header_len(),
			 &ltt_buf->commit_count[SUBBUF_INDEX(offset_begin,
				 buf->chan)]);
		/* Check if the written buffer has to be delivered */
		if (SUBBUF_OFFSET(commit_count, buf->chan) == 0) {
			ltt_deliver(buf,
				SUBBUF_INDEX(offset_begin, buf->chan), NULL);
		}
	}
}

/* LTTng channel flush function.
 *
 * Must be called when no tracing is active in the channel, because of
 * accesses across CPUs. */
static void ltt_relay_buffer_flush(struct rchan *chan)
{
	unsigned int i;
	for (i = 0; i < NR_CPUS; i++) {
		if (!chan->buf[i])
			continue;
		chan->buf[i]->finalized = 1;
		ltt_force_switch(chan->buf[i], FORCE_FLUSH);
	}
}

static void ltt_relay_async_wakeup_chan(struct ltt_channel_struct *ltt_chan)
{
	struct rchan *rchan = ltt_chan->trans_channel_data;
	unsigned int i;

	for (i = 0; i < NR_CPUS; i++) {
		if (!rchan->buf[i])
			continue;
		if (atomic_read(&ltt_chan->buf[i].wakeup_readers) == 1) {
			atomic_set(&ltt_chan->buf[i].wakeup_readers, 0);
			wake_up_interruptible(&rchan->buf[i]->read_wait);
		}
	}
}

/* Wake writers :
 *
 * This must be done after the trace is removed from the RCU list so that there
 * are no stalled writers. */
static void ltt_relay_wake_writers(struct rchan *chan)
{
	struct ltt_channel_struct *ltt_channel =
		(struct ltt_channel_struct*)chan->private_data;
	struct ltt_channel_buf_struct *ltt_buf;
	unsigned int i;

	for (i = 0; i < NR_CPUS; i++) {
		if (!chan->buf[i])
			continue;
		ltt_buf = &ltt_channel->buf[i];
		if (waitqueue_active(&ltt_buf->write_wait))
			schedule_work(&ltt_buf->wake_writers);
	}
}

static void ltt_relay_finish_channel(struct ltt_channel_struct *channel)
{
	struct rchan *rchan = channel->trans_channel_data;
#if 0
	relay_flush(rchan);
#endif //0
	ltt_relay_buffer_flush(rchan);
	ltt_relay_wake_writers(rchan);
}

static void ltt_relay_remove_channel(struct ltt_channel_struct *channel)
{
	struct rchan *rchan = channel->trans_channel_data;

	relay_close(rchan);
#if 0
	ltt_channel_destroy(channel);
#endif //0
}

/* ltt_relay_reserve_slot
 *
 * Atomic slot reservation in a LTTng buffer. It will take care of
 * sub-buffer switching.
 *
 * Parameters:
 *
 * @trace : the trace structure to log to.
 * @buf : the buffer to reserve space into.
 * @data_size : size of the variable length data to log.
 * @slot_size : pointer to total size of the slot (out)
 * @tsc : pointer to the tsc at the slot reservation (out)
 * @before_hdr_pad : dynamic padding before the event header.
 * @after_hdr_pad : dynamic padding after the event header.
 *
 * Return : NULL if not enough space, else returns the pointer
 * 					to the beginning of the reserved slot. */
static void *ltt_relay_reserve_slot(
		struct ltt_trace_struct *trace,
		struct ltt_channel_struct *ltt_channel,
		void **transport_data,
		size_t data_size,
		size_t *slot_size,
		u64 *tsc,
		size_t *before_hdr_pad,
		size_t *after_hdr_pad,
		size_t *header_size)
{
	struct rchan *rchan = ltt_channel->trans_channel_data;
	struct rchan_buf *buf = *transport_data = rchan->buf[smp_processor_id()];
	struct ltt_channel_buf_struct *ltt_buf = &ltt_channel->buf[buf->cpu];
	int offset_begin, offset_end, offset_old;
	int begin_switch, end_switch_current, end_switch_old;
	int reserve_commit_diff = 0;
	size_t size;
	int consumed_old, consumed_new;
	int commit_count;

	if (ltt_nesting[smp_processor_id()] > 4) {
		atomic_up_inc(&ltt_buf->events_lost);
		return NULL;
	}

	do {
		offset_old = atomic_read(&ltt_buf->offset);
		offset_begin = offset_old;
		begin_switch = 0;
		end_switch_current = 0;
		end_switch_old = 0;

		if (SUBBUF_OFFSET(offset_begin, buf->chan) == 0) {
			begin_switch = 1; /* For offset_begin */
		} else {
			size = ltt_get_header_size(trace,
					buf->start + offset_begin,
					before_hdr_pad, after_hdr_pad,
					header_size) + data_size;
			if ((SUBBUF_OFFSET(offset_begin, buf->chan)+size)
					> buf->chan->subbuf_size) {
				end_switch_old = 1;	/* For offset_old */
				begin_switch = 1;	/* For offset_begin */
			}
		}
		if (begin_switch) {
			if (end_switch_old) {
				offset_begin =
					SUBBUF_ALIGN(offset_begin, buf->chan);
			}
			offset_begin = offset_begin +
				ltt_subbuf_header_len();
			/* Test new buffer integrity */
			reserve_commit_diff = SUBBUF_OFFSET(
				buf->chan->subbuf_size - atomic_read(
				&ltt_buf->commit_count[
						SUBBUF_INDEX(offset_begin,
						buf->chan)]), buf->chan);
			if (reserve_commit_diff == 0) {
				/* Next buffer not corrupted. */
				if (!ltt_channel->overwrite &&
					(SUBBUF_TRUNC(offset_begin, buf->chan)
					- SUBBUF_TRUNC(
						atomic_read(&ltt_buf->consumed),
						buf->chan))
					>= rchan->alloc_size) {
					/* We do not overwrite non consumed
					 * buffers and we are full : event
					 * is lost. */
					atomic_up_inc(&ltt_buf->events_lost);
					return NULL;
				} else {
					/* next buffer not corrupted, we are
					 * either in overwrite mode or the
					 * buffer is not full. It's safe to
					 * write in this new subbuffer.*/
				}
			} else {
				/* Next subbuffer corrupted. Force pushing
				 * reader even in normal mode. It's safe to
				 * write in this new subbuffer. */
			}
			size = ltt_get_header_size(trace,
					buf->start + offset_begin,
					before_hdr_pad, after_hdr_pad,
					header_size) + data_size;
			if ((SUBBUF_OFFSET(offset_begin,buf->chan) + size)
					> buf->chan->subbuf_size) {
				/* Event too big for subbuffers, report error,
				 * don't complete the sub-buffer switch. */
				atomic_up_inc(&ltt_buf->events_lost);
				return NULL;
			} else {
				/* We just made a successful buffer switch and
				 * the event fits in the new subbuffer. Let's
				 * write. */
			}
		} else {
			/* Event fits in the current buffer and we are not on a
			 * switch boundary. It's safe to write */
		}
		offset_end = offset_begin + size;

		if ((SUBBUF_OFFSET(offset_end, buf->chan)) == 0) {
			/* The offset_end will fall at the very beginning of
			 * the next subbuffer. */
			end_switch_current = 1;	/* For offset_begin */
		}
#ifdef CONFIG_LTT_HEARTBEAT_EVENT
		if (begin_switch || end_switch_old || end_switch_current)
			*tsc = ltt_get_timestamp64();
		else
			*tsc = ltt_get_timestamp32();
#else
		*tsc = ltt_get_timestamp64();
#endif //CONFIG_LTT_HEARTBEAT_EVENT
		if (*tsc == 0) {
			/* Error in getting the timestamp, event lost */
			atomic_up_inc(&ltt_buf->events_lost);
			return NULL;
		}

	} while (atomic_up_cmpxchg(&ltt_buf->offset, offset_old, offset_end)
							!= offset_old);


	/* Push the reader if necessary */
	do {
		consumed_old = atomic_read(&ltt_buf->consumed);
		/* If buffer is in overwrite mode, push the reader consumed
		 * count if the write position has reached it and we are not
		 * at the first iteration (don't push the reader farther than
		 * the writer). This operation can be done concurrently by many
		 * writers in the same buffer, the writer being at the fartest
		 * write position sub-buffer index in the buffer being the one
		 * which will win this loop. */
		/* If the buffer is not in overwrite mode, pushing the reader
		 * only happen if a sub-buffer is corrupted */
		if ((SUBBUF_TRUNC(offset_end-1, buf->chan)
					- SUBBUF_TRUNC(consumed_old, buf->chan))
					>= rchan->alloc_size)
			consumed_new = SUBBUF_ALIGN(consumed_old, buf->chan);
		else {
			consumed_new = consumed_old;
			break;
		}
	} while (atomic_cmpxchg(&ltt_buf->consumed, consumed_old, consumed_new)
						!= consumed_old);

	if (consumed_old != consumed_new) {
		/* Reader pushed : we are the winner of the push, we can
		 * therefore reequilibrate reserve and commit. Atomic increment
		 * of the commit count permits other writers to play around
		 * with this variable before us. We keep track of
		 * corrupted_subbuffers even in overwrite mode :
		 * we never want to write over a non completely committed
		 * sub-buffer : possible causes : the buffer size is too low
		 * compared to the unordered data input, or there is a writer
		 * who died between the reserve and the commit. */
		if (reserve_commit_diff) {
			/* We have to alter the sub-buffer commit count : a
			 * sub-buffer is corrupted. We do not deliver it. */
			atomic_up_add(
				reserve_commit_diff,
				&ltt_buf->commit_count[
					SUBBUF_INDEX(offset_begin, buf->chan)]);
			atomic_up_inc(&ltt_buf->corrupted_subbuffers);
		}
	}

	if (end_switch_old) {
		/* old subbuffer */
		/* Concurrency safe because we are the last and only thread to
		 * alter this sub-buffer. As long as it is not delivered and
		 * read, no other thread can alter the offset, alter the
		 * reserve_count or call the client_buffer_end_callback on
		 * this sub-buffer.
		 * The only remaining threads could be the ones with pending
		 * commits. They will have to do the deliver themself.
		 * Not concurrency safe in overwrite mode. We detect corrupted
		 * subbuffers with commit and reserve counts. We keep a
		 * corrupted sub-buffers count and push the readers across
		 * these sub-buffers.
		 * Not concurrency safe if a writer is stalled in a subbuffer
		 * and another writer switches in, finding out it's corrupted.
		 * The result will be than the old (uncommited) subbuffer will
		 * be declared corrupted, and that the new subbuffer will be
		 * declared corrupted too because of the commit count
		 * adjustment.
		 * Note : offset_old should never be 0 here.*/
		ltt_channel->buffer_end(buf, *tsc, offset_old,
			SUBBUF_INDEX((offset_old-1), buf->chan));
		commit_count =
			atomic_up_add_return(buf->chan->subbuf_size
				- (SUBBUF_OFFSET(offset_old-1, buf->chan)+1),
				&ltt_buf->commit_count[SUBBUF_INDEX(
						offset_old-1, buf->chan)]);
		if (SUBBUF_OFFSET(commit_count, buf->chan) == 0) {
			ltt_deliver(buf, SUBBUF_INDEX((offset_old-1),
						buf->chan), NULL);
		}
	}

	if (begin_switch) {
		/* New sub-buffer */
		/* This code can be executed unordered : writers may already
		 * have written to the sub-buffer before this code gets
		 * executed, caution. */
		/* The commit makes sure that this code is executed before the
		 * deliver of this sub-buffer */
		ltt_channel->buffer_begin(buf, *tsc, SUBBUF_INDEX(offset_begin,
					buf->chan));
		commit_count = atomic_up_add_return(
				ltt_subbuf_header_len(),
				&ltt_buf->commit_count[
					SUBBUF_INDEX(offset_begin, buf->chan)]);
		/* Check if the written buffer has to be delivered */
		if (SUBBUF_OFFSET(commit_count, buf->chan) == 0) {
			ltt_deliver(buf,
				SUBBUF_INDEX(offset_begin, buf->chan), NULL);
		}
	}

	if (end_switch_current) {
		/* current subbuffer */
		/* Concurrency safe because we are the last and only thread to
		 * alter this sub-buffer. As long as it is not delivered and
		 * read, no other thread can alter the offset, alter the
		 * reserve_count or call the client_buffer_end_callback on this
		 * sub-buffer.
		 * The only remaining threads could be the ones with pending
		 * commits. They will have to do the deliver themself.
		 * Not concurrency safe in overwrite mode. We detect corrupted
		 * subbuffers with commit and reserve counts. We keep a
		 * corrupted sub-buffers count and push the readers across
		 * these sub-buffers.
		 * Not concurrency safe if a writer is stalled in a subbuffer
		 * and another writer switches in, finding out it's corrupted.
		 * The result will be than the old (uncommited) subbuffer will
		 * be declared corrupted, and that the new subbuffer will be
		 * declared corrupted too because of the commit count
		 * adjustment. */
		ltt_channel->buffer_end(buf, *tsc, offset_end,
			SUBBUF_INDEX((offset_end-1), buf->chan));
		commit_count =
			atomic_up_add_return(buf->chan->subbuf_size
				- (SUBBUF_OFFSET(offset_end-1, buf->chan)+1),
				&ltt_buf->commit_count[SUBBUF_INDEX(
						offset_end-1, buf->chan)]);
		if (SUBBUF_OFFSET(commit_count, buf->chan) == 0) {
			ltt_deliver(buf,
				SUBBUF_INDEX((offset_end-1), buf->chan), NULL);
		}
	}

	*slot_size = size;

	//BUG_ON(*slot_size != (data_size + *before_hdr_pad + *after_hdr_pad + *header_size));
	//BUG_ON(*slot_size != (offset_end - offset_begin));

	return buf->start + BUFFER_OFFSET(offset_begin, buf->chan);
}


/* ltt_relay_commit_slot
 *
 * Atomic unordered slot commit. Increments the commit count in the
 * specified sub-buffer, and delivers it if necessary.
 *
 * Parameters:
 *
 * @buf : the buffer to commit to.
 * @reserved : address of the beginnig of the reserved slot.
 * @slot_size : size of the reserved slot.
 *
 */
static void ltt_relay_commit_slot(
		struct ltt_channel_struct *ltt_channel,
		void **transport_data,
		void *reserved,
		size_t slot_size)
{
	struct rchan_buf *buf = *transport_data;
	struct ltt_channel_buf_struct *ltt_buf = &ltt_channel->buf[buf->cpu];
	unsigned int offset_begin = reserved - buf->start;
	int commit_count;

	commit_count = atomic_up_add_return(slot_size,
		&ltt_buf->commit_count[SUBBUF_INDEX(offset_begin, buf->chan)]);
	/* Check if all commits have been done */
	if (SUBBUF_OFFSET(commit_count, buf->chan) == 0)
		ltt_deliver(buf, SUBBUF_INDEX(offset_begin, buf->chan), NULL);
}

static void ltt_relay_print_subbuffer_errors(struct ltt_channel_struct *ltt_chan,
		int cons_off, unsigned int i)
{
	struct rchan *rchan = ltt_chan->trans_channel_data;
	int cons_idx;

	printk(KERN_WARNING
		"LTT : unread channel %s offset is %d "
		"and cons_off : %d (cpu %u)\n",
		ltt_chan->channel_name,
		atomic_read(&ltt_chan->buf[i].offset), cons_off, i);
	/* Check each sub-buffer for non zero commit count */
	cons_idx = SUBBUF_INDEX(cons_off, rchan);
	if (SUBBUF_OFFSET(atomic_read(&ltt_chan->buf[i].commit_count[cons_idx]),
				rchan))
		printk(KERN_ALERT
			"LTT : %s : subbuffer %u has non zero "
			"commit count.\n",
			ltt_chan->channel_name, cons_idx);
	printk(KERN_ALERT "LTT : %s : commit count : %u, subbuf size %zd\n",
			ltt_chan->channel_name,
			atomic_read(&ltt_chan->buf[i].commit_count[cons_idx]),
			rchan->subbuf_size);
}

static void ltt_relay_print_errors(struct ltt_trace_struct *trace,
		struct ltt_channel_struct *ltt_chan, int cpu)
{
	struct rchan *rchan = ltt_chan->trans_channel_data;
	int cons_off;

	for (cons_off = atomic_read(&ltt_chan->buf[cpu].consumed);
		(SUBBUF_TRUNC(atomic_read(&ltt_chan->buf[cpu].offset),
				rchan)
			- cons_off) > 0;
		cons_off = SUBBUF_ALIGN(cons_off, rchan)) {
		ltt_relay_print_subbuffer_errors(ltt_chan, cons_off, cpu);
	}
}

/* This is called with preemption disabled when user space has requested
 * blocking mode.  If one of the active traces has free space below a
 * specific threshold value, we reenable preemption and block.
 */
static int ltt_relay_user_blocking(struct ltt_trace_struct *trace,
		unsigned int index, size_t data_size, struct user_dbg_data *dbg)
{
	struct rchan *rchan;
	struct ltt_channel_buf_struct *ltt_buf;
	struct ltt_channel_struct *channel;
	struct rchan_buf *relay_buf;
	DECLARE_WAITQUEUE(wait, current);

	channel = ltt_get_channel_from_index(trace, index);
	rchan = channel->trans_channel_data;
	relay_buf = rchan->buf[smp_processor_id()];
	ltt_buf = &channel->buf[smp_processor_id()];
	/* Check if data is too big for the channel : do not
	 * block for it */
	if (LTT_RESERVE_CRITICAL + data_size > relay_buf->chan->subbuf_size)
		return 0;

	/* If free space too low, we block. We restart from the
	 * beginning after we resume (cpu id may have changed
	 * while preemption is active).
	 */
	spin_lock(&ltt_buf->full_lock);
	if (!channel->overwrite &&
		(dbg->avail_size = (dbg->write = atomic_read(
			&channel->buf[relay_buf->cpu].offset))
		+ LTT_RESERVE_CRITICAL + data_size
		 - SUBBUF_TRUNC((dbg->read = atomic_read(
		&channel->buf[relay_buf->cpu].consumed)),
			 		relay_buf->chan))
			>= rchan->alloc_size) {
		__set_current_state(TASK_INTERRUPTIBLE);
		add_wait_queue(&ltt_buf->write_wait, &wait);
		spin_unlock(&ltt_buf->full_lock);
		preempt_enable();
		schedule();
		__set_current_state(TASK_RUNNING);
		remove_wait_queue(&ltt_buf->write_wait, &wait);
		if (signal_pending(current))
			return -ERESTARTSYS;
		preempt_disable();
		return 1;
	}
	spin_unlock(&ltt_buf->full_lock);
	return 0;
}

static void ltt_relay_print_user_errors(struct ltt_trace_struct *trace,
		unsigned int index, size_t data_size, struct user_dbg_data *dbg)
{
	struct rchan *rchan;
	struct ltt_channel_buf_struct *ltt_buf;
	struct ltt_channel_struct *channel;
	struct rchan_buf *relay_buf;

	channel = ltt_get_channel_from_index(trace, index);
	rchan = channel->trans_channel_data;
	relay_buf = rchan->buf[smp_processor_id()];
	ltt_buf = &channel->buf[smp_processor_id()];
	printk(KERN_ERR "Error in LTT usertrace : "
	"buffer full : event lost in blocking "
	"mode. Increase LTT_RESERVE_CRITICAL.\n");
	printk(KERN_ERR "LTT nesting level is %u.\n",
		ltt_nesting[smp_processor_id()]);
	printk(KERN_ERR "LTT avail size %lu.\n",
		dbg->avail_size);
	printk(KERN_ERR "avai write : %lu, read : %lu\n",
			dbg->write, dbg->read);
	printk(KERN_ERR "LTT cur size %lu.\n",
		(dbg->write = atomic_read(
		&channel->buf[relay_buf->cpu].offset))
	+ LTT_RESERVE_CRITICAL + data_size
	 - SUBBUF_TRUNC((dbg->read = atomic_read(
	&channel->buf[relay_buf->cpu].consumed)),
				relay_buf->chan));
	printk(KERN_ERR "cur write : %lu, read : %lu\n",
			dbg->write, dbg->read);
}

static struct ltt_transport ltt_relay_transport = {
	.name = "relay",
	.owner = THIS_MODULE,
	.ops = {
		.create_dirs = ltt_relay_create_dirs,
		.remove_dirs = ltt_relay_remove_dirs,
		.create_channel = ltt_relay_create_channel,
		.finish_channel = ltt_relay_finish_channel,
		.remove_channel = ltt_relay_remove_channel,
		.wakeup_channel = ltt_relay_async_wakeup_chan,
		.commit_slot = ltt_relay_commit_slot,
		.reserve_slot = ltt_relay_reserve_slot,
		.print_errors = ltt_relay_print_errors,
		.user_blocking = ltt_relay_user_blocking,
		.user_errors = ltt_relay_print_user_errors,
	},
};

static int __init ltt_relay_init(void)
{
	printk(KERN_INFO "LTT : ltt-relay init\n");
	ltt_root_dentry = debugfs_create_dir(LTT_RELAY_ROOT, NULL);
	if (ltt_root_dentry == NULL)
		return -EEXIST;

	ltt_file_operations = relay_file_operations;
	ltt_file_operations.owner = THIS_MODULE;
	ltt_file_operations.poll = ltt_poll;
	ltt_file_operations.ioctl = ltt_ioctl;
#ifdef CONFIG_COMPAT
	ltt_file_operations.compat_ioctl = ltt_compat_ioctl;
#endif //CONFIG_COMPAT

	ltt_transport_register(&ltt_relay_transport);

	return 0;
}

static void __exit ltt_relay_exit(void)
{
	printk(KERN_INFO "LTT : ltt-relay exit\n");

	ltt_transport_unregister(&ltt_relay_transport);

	debugfs_remove(ltt_root_dentry);
}

module_init(ltt_relay_init);
module_exit(ltt_relay_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Linux Trace Toolkit Next Generation Tracer");

