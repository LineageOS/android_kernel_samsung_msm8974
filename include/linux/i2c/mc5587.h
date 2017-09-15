/*
 * Analog Devices MC5587 I/O Expander and QWERTY Keypad Controller
 *
 * Copyright 2009-2010 Analog Devices Inc.
 *
 * Licensed under the GPL-2 or later.
 */

#ifndef _MC5587_H
#define _MC5587_H

#define DEVID		0x00	/* Device ID */
#define CFGREG		0x01	/* Configuration Register1 */
#define INTPND		0x02	/* Interrupt Status Register */
#define KLCK_EC		0x03	/* Key Lock and Event Counter Register */
#define KEYEVNT0	0x04	/* Key Event Register A */
#define KEYEVNT1	0x05	/* Key Event Register B */
#define KEYEVNT2	0x06	/* Key Event Register C */
#define KEYEVNT3	0x07	/* Key Event Register D */
#define KEYEVNT4	0x08	/* Key Event Register E */
#define KEYEVNT5	0x09	/* Key Event Register F */
#define KEYEVNT6	0x0A	/* Key Event Register G */
#define KEYEVNT7	0x0B	/* Key Event Register H */
#define KEYEVNT8	0x0C	/* Key Event Register I */
#define KEYEVNT9	0x0D	/* Key Event Register J */
#define KLCKTMR		0x0E	/* Keypad Lock1 to Lock2 Timer */
#define UNLCK1		0x0F	/* Unlock Key1 */
#define UNLCK2		0x10	/* Unlock Key2 */
#define GPINTST1	0x11	/* GPIO Interrupt Status */
#define GPINTST2	0x12	/* GPIO Interrupt Status */
#define GPINTST3	0x13	/* GPIO Interrupt Status */
#define GPIDAT1		0x14	/* GPIO Data Status, Read twice to clear */
#define GPIDAT2		0x15	/* GPIO Data Status, Read twice to clear */
#define GPIDAT3		0x16	/* GPIO Data Status, Read twice to clear */
#define GPODAT1		0x17	/* GPIO DATA OUT */
#define GPODAT2		0x18	/* GPIO DATA OUT */
#define GPODAT3		0x19	/* GPIO DATA OUT */
#define GPINTEN1	0x1A	/* GPIO Interrupt Enable */
#define GPINTEN2	0x1B	/* GPIO Interrupt Enable */
#define GPINTEN3	0x1C	/* GPIO Interrupt Enable */
#define KPMSEL1		0x1D	/* Keypad or GPIO Selection */
#define KPMSEL2		0x1E	/* Keypad or GPIO Selection */
#define KPMSEL3		0x1F	/* Keypad or GPIO Selection */
#define GPIEMSEL1	0x20	/* GPI Event Mode 1 */
#define GPIEMSEL2	0x21	/* GPI Event Mode 2 */
#define GPIEMSEL3	0x22	/* GPI Event Mode 3 */
#define GPIODIR1	0x23	/* GPIO Data Direction */
#define GPIODIR2	0x24	/* GPIO Data Direction */
#define GPIODIR3	0x25	/* GPIO Data Direction */
#define GPINTLVL1	0x26	/* GPIO Edge/Level Detect */
#define GPINTLVL2	0x27	/* GPIO Edge/Level Detect */
#define GPINTLVL3	0x28	/* GPIO Edge/Level Detect */
#define DBNCON1		0x29	/* Debounce Disable */
#define DBNCON2		0x2A	/* Debounce Disable */
#define DBNCON3		0x2B	/* Debounce Disable */
#define GPIPU1		0x2C	/* GPIO Pull Disable */
#define GPIPU2		0x2D	/* GPIO Pull Disable */
#define GPIPU3		0x2E	/* GPIO Pull Disable */

#define MC5587_DEVICE_ID_MASK	0x0F
#define MC5587_MFG_ID_MASK	0xF0

/* Configuration Register 0x01 */
#define MC5587_AUTOINC		(1 << 7)
#define MC5587_GPGEVTCFG	(1 << 6)
#define MC5587_OVFMD		(1 << 5)
#define MC5587_INTCFG		(1 << 4)
#define MC5587_OVFIEN		(1 << 3)
#define MC5587_KLCKIEN		(1 << 2)
#define MC5587_GPIIEN		(1 << 1)
#define MC5587_KEIEN		(1 << 0)

/* Interrupt Status Register 0x02 */
#define MC5587_OVFPND		(1 << 3)
#define MC5587_KLCKPND		(1 << 2)
#define MC5587_GPIPND		(1 << 1)
#define MC5587_KEPND		(1 << 0)

/* Keylock and Event Counter Register */
#define MC5587_KLCKEN		(1 << 6)
#define MC5587_LCKST		0x30
#define MC5587_KECNT		0x0F

#define MC5587_MAXGPIO		18
#define MC5587_BANK(offs)	((offs) >> 3)
#define MC5587_BIT(offs)	(1u << ((offs) & 0x7))

/* Put one of these structures in i2c_board_info platform_data */

#define MC5587_KEYMAPSIZE	80

#define GPI_PIN_ROW0 97
#define GPI_PIN_ROW1 98
#define GPI_PIN_ROW2 99
#define GPI_PIN_ROW3 100
#define GPI_PIN_ROW4 101
#define GPI_PIN_ROW5 102
#define GPI_PIN_ROW6 103
#define GPI_PIN_ROW7 104
#define GPI_PIN_COL0 105
#define GPI_PIN_COL1 106
#define GPI_PIN_COL2 107
#define GPI_PIN_COL3 108
#define GPI_PIN_COL4 109
#define GPI_PIN_COL5 110
#define GPI_PIN_COL6 111
#define GPI_PIN_COL7 112
#define GPI_PIN_COL8 113
#define GPI_PIN_COL9 114

#define GPO_PIN_VT_CAM_12_EN		0
#define GPO_PIN_MAIN_CAM_A_EN		8
#define GPI_PIN_CAM_SENSOR_DET		9
#define GPO_PIN_NFC_EN			10
#define GPO_PIN_FLASH_LED_SET		11
#define GPI_PIN_TX_GTR_THRES		12
#define GPO_PIN_VT_CAM_STBY		13
#define GPO_PIN_VT_CAM_A_EN		14
#define GPO_PIN_CRESET_B		15
#define GPO_PIN_VT_CAM_IO_EN		16
#define GPO_PIN_VT_CAM_NRST		17

#define GPI_PIN_ROW_BASE GPI_PIN_ROW0
#define GPI_PIN_ROW_END GPI_PIN_ROW7
#define GPI_PIN_COL_BASE GPI_PIN_COL0
#define GPI_PIN_COL_END GPI_PIN_COL9

#define GPI_PIN_BASE GPI_PIN_ROW_BASE
#define GPI_PIN_END GPI_PIN_COL_END

#define MC5587_GPIMAPSIZE_MAX (GPI_PIN_END - GPI_PIN_BASE + 1)

struct mc5587_gpi_map {
	unsigned short pin;
	unsigned short sw_evt;
};

struct mc5587_kpad_platform_data {
	int rows;			/* Number of rows */
	int cols;			/* Number of columns */
	const unsigned short *keymap;	/* Pointer to keymap */
	unsigned short keymapsize;	/* Keymap size */
	unsigned repeat:1;		/* Enable key repeat */
	unsigned en_keylock:1;		/* Enable Key Lock feature */
	unsigned short unlock_key1;	/* Unlock Key 1 */
	unsigned short unlock_key2;	/* Unlock Key 2 */
	const struct mc5587_gpi_map *gpimap;
	unsigned short gpimapsize;
	const struct mc5587_gpio_platform_data *gpio_data;
	int hall_gpio;
	int led_gpio;
};

struct i2c_client; /* forward declaration */

struct mc5587_gpio_platform_data {
	int gpio_start;		/* GPIO Chip base # */
	int ngpio;		
	unsigned irq_base;	/* interrupt base # */
	int reset_gpio;		/* reset gpio pin   */
	int irq_gpio;		/* irq gpio pin	    */
	unsigned pullup_dis_mask; /* Pull-Up Disable Mask */
	int	(*setup)(struct i2c_client *client,
				int gpio, unsigned ngpio,
				void *context);
	int	(*teardown)(struct i2c_client *client,
				int gpio, unsigned ngpio,
				void *context);
	void	*context;
};

#endif
