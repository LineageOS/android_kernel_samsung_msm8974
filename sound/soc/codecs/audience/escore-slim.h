/*
 * escore-slim.h  --  Slimbus interface for Audience earSmart chips
 *
 * Copyright 2011 Audience, Inc.
 *
 * Author: Greg Clemson <gclemson@audience.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ESCORE_SLIM_H
#define _ESCORE_SLIM_H

#define ES_SLIM_BOOT_CMD			0x0001
#define ES_SLIM_BOOT_ACK			0x01010101

void escore_init_slim_slave(struct slim_device *sbdev);
extern int escore_slimbus_init(void);
extern void es515_slim_setup(struct escore_priv *escore_priv);

extern struct slim_driver escore_slim_driver;
extern struct snd_soc_dai_ops escore_slim_port_dai_ops;
#endif
