/*
 * secgpio_dvs.h -- Samsung GPIO debugging and verification system
 */

#ifndef __SECGPIO_DVS_H
#define __SECGPIO_DVS_H

#include <linux/types.h>

enum gdvs_phone_status {
	PHONE_INIT = 0,
	PHONE_SLEEP,
	GDVS_PHONE_STATUS_MAX
};

struct gpiomap_result {
	unsigned char *init;
	unsigned char *sleep;
};

struct gpio_dvs {
	struct gpiomap_result *result;
	unsigned int count;
	bool check_sleep;
	void (*check_gpio_status)(unsigned char phonestate);
};

void gpio_dvs_check_initgpio(void);
void gpio_dvs_check_sleepgpio(void);

#endif /* __SECGPIO_DVS_H */
