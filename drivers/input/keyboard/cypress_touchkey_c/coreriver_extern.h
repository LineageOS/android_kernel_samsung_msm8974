/* filename: coreriver_extern.h */
#include <linux/i2c/touchkey_i2c.h>

/*
 * CORERIVER TOUCHCORE touchkey fw update
 *
 * Copyright (C) 2012 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

extern int coreriver_fw_update(struct cypress_touchkey_info *info, bool force);
