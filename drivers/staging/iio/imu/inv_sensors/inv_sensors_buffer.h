#ifndef _INV_SENSORS_BUFFER_H_
#define _INV_SENSORS_BUFFER_H_

#include "inv_sensors_iio.h"
#include "inv_sensors_common.h"

struct iio_buffer *invsens_kfifo_allocate(struct iio_dev *indio_dev);
void invsens_kfifo_free(struct iio_buffer *r);

#endif /* _INV_SENSORS_BUFFER_H_ */
