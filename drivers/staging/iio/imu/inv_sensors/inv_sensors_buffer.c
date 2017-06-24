
#include "inv_sensors_buffer.h"


struct invsens_fifo_t {
	struct iio_buffer buffer;
	struct kfifo kf;
	int update_needed;
};

#define iio_to_kfifo(r) container_of(r, struct invsens_fifo_t, buffer)

static int kfifo_request_update(struct iio_buffer *r)
{
	int ret = 0;
	struct invsens_fifo_t *buf = iio_to_kfifo(r);

	INV_DBG_FUNC_NAME;

	if (!buf->update_needed)
		goto error_ret;
	kfifo_free(&buf->kf);

	__iio_update_buffer(&buf->buffer, buf->buffer.bytes_per_datum,
		buf->buffer.length);
	ret = kfifo_alloc(&buf->kf,
		buf->buffer.bytes_per_datum*buf->buffer.length,
		GFP_KERNEL);
	r->stufftoread = false;
error_ret:
	return ret;
}

static int kfifo_get_length(struct iio_buffer *r)
{
	INV_DBG_FUNC_NAME;

	return r->length;
}


static int kfifo_get_bytes_per_datum(struct iio_buffer *r)
{
	INV_DBG_FUNC_NAME;

	return r->bytes_per_datum;
}

static int kfifo_mark_update_needed(struct iio_buffer *r)
{
	struct invsens_fifo_t *kf = iio_to_kfifo(r);
	kf->update_needed = true;

	INV_DBG_FUNC_NAME;

	return 0;
}

static int kfifo_set_bytes_per_datum(struct iio_buffer *r, size_t bpd)
{

	INV_DBG_FUNC_NAME;


	if (r->bytes_per_datum != bpd) {
		r->bytes_per_datum = bpd;
		kfifo_mark_update_needed(r);
	}
	return 0;
}

static int kfifo_set_length(struct iio_buffer *r, int length)
{
	INV_DBG_FUNC_NAME;

	if (r->length != length) {
		r->length = length;
		kfifo_mark_update_needed(r);
	}
	return 0;
}

static int kfifo_store_to(struct iio_buffer *r,
			      u8 *data,
			      s64 timestamp)
{
	int ret;
	struct invsens_fifo_t *kf = iio_to_kfifo(r);
	u16 packet_header;
	u16 packet_size_offset;
	u16 packet_size;

	INV_DBG_FUNC_NAME;

	/*check the header and the packet size*/
	memcpy((void *)&packet_header, data, sizeof(packet_header));
	packet_size_offset = sizeof(packet_header);
	memcpy((void *) &packet_size,
		&data[packet_size_offset], sizeof(packet_size));

	INVSENS_LOGD("header = %x,\n", packet_header);

	if (unlikely(packet_header != INVSENS_PACKET_HEADER)) {
		INVSENS_LOGE("%s : invalid header = %x\n",
			__func__, packet_header);
		return -EFAULT;
	}

	/*put packet into fifo as much as packet size*/
	ret = kfifo_in(&kf->kf, data, packet_size);
	if (unlikely(ret != packet_size))
		return -EBUSY;

	r->stufftoread = true;
	wake_up_interruptible(&r->pollq);

	return 0;
}

static int kfifo_read_first_n_kfifo(struct iio_buffer *r,
			   size_t n, char __user *buf)
{
	int ret, copied;
	struct invsens_fifo_t *kf = iio_to_kfifo(r);
	u16 header[2];
	u16 packet_header;
	u16 packet_size;
	u16 data_size;

	if (unlikely(n < r->bytes_per_datum))
		return -EINVAL;

	/*get the header of packet and read the packet size*/
	ret = kfifo_out(&kf->kf, (void *)header, sizeof(header));
	if (unlikely(!ret))
		return -EINVAL;

	packet_header = header[0];
	packet_size = header[1];

	if (unlikely(packet_header != INVSENS_PACKET_HEADER)) {
		INVSENS_LOGE("%s : invalid header = %x\n",
			__func__, packet_header);
		kfifo_reset_out(&kf->kf);
		return -EFAULT;
	}

	/*find data size*/
	data_size = packet_size - sizeof(header);

	if(data_size > n) {
		INVSENS_LOGE("Data overflow, reset!!, datasize=%d, buffer=%d\n", data_size, n);
		kfifo_reset_out(&kf->kf);
		return -EFAULT;
	}

	n = data_size;

	INVSENS_LOGD("%s : n = %d\n", __func__, n);

	/*copy packet from fifo*/
	ret = kfifo_to_user(&kf->kf, buf, n, &copied);

	if (kfifo_is_empty(&kf->kf))
		r->stufftoread = false;

	/* verify it is still empty to avoid race */
	if (!kfifo_is_empty(&kf->kf))
		r->stufftoread = true;

	return copied;
}



static const struct iio_buffer_access_funcs invsens_kfifo_access_funcs = {
	.store_to = &kfifo_store_to,
	.read_first_n = &kfifo_read_first_n_kfifo,
	.request_update = &kfifo_request_update,
	.get_bytes_per_datum = &kfifo_get_bytes_per_datum,
	.set_bytes_per_datum = &kfifo_set_bytes_per_datum,
	.get_length = &kfifo_get_length,
	.set_length = &kfifo_set_length,
};

static IIO_BUFFER_ENABLE_ATTR;
static IIO_BUFFER_LENGTH_ATTR;

static struct attribute *invsens_kfifo_attributes[] = {
	&dev_attr_length.attr,
	&dev_attr_enable.attr,
	NULL,
};

static struct attribute_group invsens_kfifo_attribute_group = {
	.attrs = invsens_kfifo_attributes,
	.name = "buffer",
};


struct iio_buffer *invsens_kfifo_allocate(struct iio_dev *indio_dev)
{
	struct invsens_fifo_t *kf;

	kf = kzalloc(sizeof *kf, GFP_KERNEL);
	if (!kf)
		return NULL;
	kf->update_needed = true;
	iio_buffer_init(&kf->buffer);
	kf->buffer.attrs = &invsens_kfifo_attribute_group;
	kf->buffer.access = &invsens_kfifo_access_funcs;

	return &kf->buffer;
}


void invsens_kfifo_free(struct iio_buffer *r)
{
	kfree(iio_to_kfifo(r));
}
