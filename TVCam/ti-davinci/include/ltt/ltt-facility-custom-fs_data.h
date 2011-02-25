#ifndef _LTT_FACILITY_CUSTOM_FS_DATA_H_
#define _LTT_FACILITY_CUSTOM_FS_DATA_H_

#include <linux/types.h>
#include <ltt/ltt-facility-id-fs_data.h>
#include <ltt/ltt-tracer.h>
#include <asm/uaccess.h>

/* Named types */

/* Event read structures */
typedef struct lttng_sequence_fs_data_read_data lttng_sequence_fs_data_read_data;
struct lttng_sequence_fs_data_read_data {
	unsigned int len;
	const unsigned char *array;
};

static inline size_t lttng_get_alignment_sequence_fs_data_read_data(
		lttng_sequence_fs_data_read_data *obj)
{
	size_t align=0, localign;
	localign = sizeof(unsigned int);
	align = max(align, localign);

	localign = sizeof(unsigned char);
	align = max(align, localign);

	return align;
}

static inline void lttng_write_custom_sequence_fs_data_read_data(
		char *buffer,
		size_t *to_base,
		size_t *to,
		const char **from,
		size_t *len,
		lttng_sequence_fs_data_read_data *obj)
{
	size_t align;

	/* Flush pending memcpy */
	if (*len != 0) {
		if (buffer != NULL)
			memcpy(buffer+*to_base+*to, *from, *len);
	}
	*to += *len;
	*len = 0;

	align = lttng_get_alignment_sequence_fs_data_read_data(obj);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	/* Contains variable sized fields : must explode the structure */

	/* Copy members */
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	if (buffer != NULL)
		memcpy(buffer+*to_base+*to, &obj->len, *len);
	*to += *len;
	*len = 0;

	align = sizeof(unsigned char);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned char);

	*len = obj->len * (*len);
	if (buffer != NULL) {
		unsigned long noncopy;
		//memcpy(buffer+*to_base+*to, obj->array, *len);
		// Copy from userspace
		// This copy should not fail, as it is called right after
		// a write or read.
		noncopy = __copy_from_user_inatomic(buffer+*to_base+*to,
				obj->array, *len);
		memset(buffer+*to_base+*to+*len-noncopy, 0, noncopy);
	}
	*to += *len;
	*len = 0;


	/* Realign the *to_base on arch size, set *to to 0 */
	*to += ltt_align(*to, sizeof(void *));
	*to_base = *to_base+*to;
	*to = 0;

	/* Put source *from just after the C sequence */
	*from = (const char*)(obj+1);
}

/* Event write structures */
typedef struct lttng_sequence_fs_data_write_data lttng_sequence_fs_data_write_data;
struct lttng_sequence_fs_data_write_data {
	unsigned int len;
	const unsigned char *array;
};

static inline size_t lttng_get_alignment_sequence_fs_data_write_data(
		lttng_sequence_fs_data_write_data *obj)
{
	size_t align=0, localign;
	localign = sizeof(unsigned int);
	align = max(align, localign);

	localign = sizeof(unsigned char);
	align = max(align, localign);

	return align;
}

static inline void lttng_write_custom_sequence_fs_data_write_data(
		char *buffer,
		size_t *to_base,
		size_t *to,
		const char **from,
		size_t *len,
		lttng_sequence_fs_data_write_data *obj)
{
	size_t align;

	/* Flush pending memcpy */
	if (*len != 0) {
		if (buffer != NULL)
			memcpy(buffer+*to_base+*to, *from, *len);
	}
	*to += *len;
	*len = 0;

	align = lttng_get_alignment_sequence_fs_data_write_data(obj);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	/* Contains variable sized fields : must explode the structure */

	/* Copy members */
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	if (buffer != NULL)
		memcpy(buffer+*to_base+*to, &obj->len, *len);
	*to += *len;
	*len = 0;

	align = sizeof(unsigned char);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned char);

	*len = obj->len * (*len);
	if (buffer != NULL) {
		unsigned long noncopy;
		//memcpy(buffer+*to_base+*to, obj->array, *len);
		// Copy from userspace
		// This copy should not fail, as it is called right
		// after a read or write.
		noncopy = __copy_from_user_inatomic(buffer+*to_base+*to,
				(const void __user *)obj->array, *len);
		memset(buffer+*to_base+*to+*len-noncopy, 0, noncopy);
	}
	*to += *len;
	*len = 0;


	/* Realign the *to_base on arch size, set *to to 0 */
	*to += ltt_align(*to, sizeof(void *));
	*to_base = *to_base+*to;
	*to = 0;

	/* Put source *from just after the C sequence */
	*from = (const char*)(obj+1);
}

#endif //_LTT_FACILITY_CUSTOM_FS_DATA_H_
