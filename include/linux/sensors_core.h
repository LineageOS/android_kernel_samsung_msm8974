/*
 * Driver model for sensor
 *
 * Copyright (C) 2008 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */


#ifndef _SENSORS_CORE_H_
#define _SENSORS_CORE_H_

int sensors_create_symlink(struct kobject *, const char *);
void sensors_remove_symlink(struct kobject *, const char *);
int sensors_register(struct device *, void *,
	struct device_attribute *[], char *);
void sensors_unregister(struct device *, struct device_attribute *[]);
void destroy_sensor_class(void);
void remap_sensor_data(s16 *, int);

#endif	/* __LINUX_SENSORS_CORE_H_INCLUDED */
