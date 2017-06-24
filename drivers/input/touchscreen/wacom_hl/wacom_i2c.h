#ifndef _LINUX_WACOM_I2C_H
#define _LINUX_WACOM_I2C_H

#include <linux/types.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

/*I2C address for digitizer and its boot loader*/
#define WACOM_I2C_ADDR 0x56
#if defined(CONFIG_EPEN_WACOM_G9PL) \
	|| defined(CONFIG_EPEN_WACOM_G9PLL) \
	|| defined(CONFIG_EPEN_WACOM_G10PM)
#define WACOM_I2C_BOOT 0x09
#else
#define WACOM_I2C_BOOT 0x57
#endif

#ifdef CONFIG_V1A
#define WACOM_X_INVERT 0
#define WACOM_XY_SWITCH 0

#define WACOM_MAX_COORD_X 26266
#define WACOM_MAX_COORD_Y 16416
#define WACOM_MAX_PRESSURE 1023

#elif defined(CONFIG_N1A)
#define WACOM_X_INVERT 0
#define WACOM_XY_SWITCH 0

#define WACOM_MAX_COORD_X 21658
#define WACOM_MAX_COORD_Y 13538
#define WACOM_MAX_PRESSURE 1023

#elif defined(CONFIG_HA)
#define WACOM_MAX_COORD_X 12576
#define WACOM_MAX_COORD_Y 7074
#define WACOM_MAX_PRESSURE 1023

#elif defined(CONFIG_MACH_HLLTE) || defined(CONFIG_MACH_HL3G) || \
	defined(CONFIG_MACH_FRESCONEOLTE_CTC) || defined(CONFIG_SEC_LOCALE_KOR_FRESCO)
#define WACOM_MAX_COORD_X 12160
#define WACOM_MAX_COORD_Y 6840
#define WACOM_MAX_PRESSURE 1023
#endif


#ifndef WACOM_X_INVERT
#define WACOM_X_INVERT 1
#endif
#ifndef WACOM_Y_INVERT
#define WACOM_Y_INVERT 0
#endif
#ifndef WACOM_XY_SWITCH
#define WACOM_XY_SWITCH 1
#endif


/*sec_class sysfs*/
extern struct class *sec_class;

struct wacom_g5_callbacks {
	int (*check_prox)(struct wacom_g5_callbacks *);
};

struct wacom_g5_platform_data {
	char *name;
	int x_invert;
	int y_invert;
	int xy_switch;
	int min_x;
	int max_x;
	int min_y;
	int max_y;
	int max_pressure;
	int min_pressure;
	int gpio_pendct;
	int gpio_pen_insert;
	void (*compulsory_flash_mode)(bool);
	int (*init_platform_hw)(void);
	int (*exit_platform_hw)(void);
	int (*suspend_platform_hw)(void);
	int (*resume_platform_hw)(void);
#ifdef CONFIG_HAS_EARLYSUSPEND
	int (*early_suspend_platform_hw)(void);
	int (*late_resume_platform_hw)(void);
#endif
	int (*reset_platform_hw)(void);
	void (*register_cb)(struct wacom_g5_callbacks *);
	int (*get_irq_state)(void);

	int gpio_ldo_en;
	int gpio_reset_n;
	int gpio_fwe1;
	int gpio_irq;
};

#endif /* _LINUX_WACOM_I2C_H */
