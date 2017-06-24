/*
 * es705-i2s.h  --  Audience eS705 platform dependent functions
 *
 * Copyright 2011 Audience, Inc.
 *
 * Author: Genisim Tsilker <gtsilker@audience.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ES705_PLATFORM_H
#define _ES705_PLATFORM_H

void es705_gpio_reset(struct es705_priv *es705);
int es705_gpio_init(struct es705_priv *es705);
void es705_gpio_free(struct esxxx_platform_data *pdata);
void es705_clk_init(struct es705_priv *es705);
void es705_vs_event(struct es705_priv *es705);
#if defined(SAMSUNG_ES705_FEATURE)
void es705_gpio_wakeup(struct es705_priv *es705);
int es705_init_input_device(struct es705_priv *es705);
void es705_unregister_input_device(struct es705_priv *es705);
void es705_uart_pin_preset(struct es705_priv *es705);
void es705_uart_pin_postset(struct es705_priv *es705);
#endif

struct esxxx_platform_data *es705_populate_dt_pdata(struct device *dev);

#endif
