#ifndef _LINUX_CYPRESS_TOUCHKEY_I2C_H
#define _LINUX_CYPRESS_TOUCHKEY_I2C_H

#include <linux/types.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#if	defined(CONFIG_SEC_LOCALE_KOR_FRESCO)
#define TK_USE_LDO_CONTROL
#define TK_INFORM_CHARGER
#endif

#if defined(CONFIG_SEC_S_PROJECT)
#define TK_USE_MAX_SUBPM_CONTROL
#define TK_USE_LDO_CONTROL
#define TK_LED_DIRECT_CONTORL		// 0x08 firmware over..
#endif


#ifdef TK_INFORM_CHARGER
struct touchkey_callbacks {
	void (*inform_charger)(struct touchkey_callbacks *, bool);
};
#endif


struct touchkey_platform_data {
	int gpio_sda;
	int gpio_scl;
	int gpio_int;
	void (*init_platform_hw)(void);
	int (*suspend) (void);
	int (*resume) (void);
	int (*power_on) (bool);
	int (*led_power_on) (bool);
	int (*reset_platform_hw)(void);
	void (*register_cb)(void *);	
#ifdef GPIO_2TOUCH_RST
	int gpio_rst;
	int (*rst_reset)(void);
#endif
#ifdef GPIO_2_TOUCH_ID
	int gpio_id;
#endif
#ifdef TKEY_GRIP_MODE
	int gpio_grip;
#endif
#if defined(TK_USE_MAX_SUBPM_CONTROL)
	const char **name_of_supply;
#endif
};

#endif /* _LINUX_CYPRESS_TOUCHKEY_I2C_H */
