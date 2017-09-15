/**
\file ad7146.h
This is the Header file for AD7146_Driver.
The file can be found at <KERNEL>/include/linux/input
*/
/*
 * include/linux/input/ad7146.h
 *
 * The platform_data for the device's "struct device" holds this
 * information.
 *
 * Copyright 2013 Analog Devices Inc.
 *
 * Licensed under the GPL version 3 or later.
 */

#ifndef __LINUX_INPUT_AD714X_H__
#define __LINUX_INPUT_AD714X_H__

/**
Total number of register count used in the initialization.
\note
This should be kept exactly as the register count in the
initialization of the platform device structure ad7146_platform_data.
*/
#define REGCNT       18

/**
Total number of register count used in the Normal mode transistion.
\note
This should be kept exactly as the register count in the "normal_regs"
of the platform device structure ad7146_platform_data.
*/
#define NORMAL_REGCNT 2

/**
Platform data structure of AD7146_MLD driver.
*/
struct ad7146_platform_data {
/**
This hold the Initial Register Configurations of AD7146
*/
	unsigned int regs[REGCNT];
/**
This hold the Normal Mode Register Configurations of AD7146
*/
	unsigned int normal_regs[NORMAL_REGCNT];
/**
This holds the Full Grip threshold value for POWER_ON_GRIP
*/
	unsigned short fixed_th_full;
	unsigned short cal_offset;
	unsigned short cal_fixed_th_full;

	void (*power_en)(int);
};

/**
READ function for AD7146
*/
typedef int (*ad7146_read_t)(struct device *, unsigned short, unsigned short *);
/**
WRITE function for AD7146
*/
typedef int (*ad7146_write_t)(struct device *, unsigned short, unsigned short);


/**
  Event value for No Grip State
 */
#define EVENT_NO_GRIP			(5)
/**
  Event value for Full Grip State
 */
#define EVENT_FULL_GRIP			(0)
/**
  Minimum OFFSET for POWER_ON grip Detection
*/
#define PW_ON_OFFSET_MIN		(0x200) /* 512 codes */
/**
  Hysteresis Percentage Reduction
 */
#define HYS_PERCENT			(20)

#if (HYS_PERCENT > 99)
#error "Hysteris percentage invalid"
#endif
/**
  Hysteresis Compensation Macro
 */
#define HYS(S, T)			((T) - ((((T)-(S)) * HYS_PERCENT)/100))
/**
  Positive Hysteresis Compensation Macro
 */
#define HYS_POS(S, T)			((T) + ((((T)-(S)) * HYS_PERCENT) \
						/ (100 - HYS_PERCENT)))
/**
  Sleep time required to go to Low Power mode in milliseconds
 */
#define SLEEP_TIME_TO_LOW_POWER		(200)
/**
  Sleep time for forced calibration during init in milliseconds
 */
#define SLEEP_TIME_TO_CALI_INIT		(100)
/**
  Sleep time for forced calibration during INT in milliseconds
 */
#define SLEEP_TIME_TO_CALI_INT		(20)

/**
  Driver State Normal
 */
#define DRIVER_STATE_NORMAL		(0)
/**
  Driver State Full Grip
 */
#define DRIVER_STATE_FULL_GRIP		(1)

/**
  Driver name of this ad7146 driver
 */
#define DRIVER_NAME			"ad7146_MLD"
/**
  This hold the product ID of AD7146
 */
#define AD7146_PRODUCT_ID		0x7146
/**
  \def AD7146_PARTID_REG
  Device ID Register Address
 */
#define AD7146_PARTID_REG		(0x17)
/**
  \def AD7146_PARTID
  Device ID for AD7146 chip
 */
#define AD7146_PARTID			(0x1490)
/**
  Clear Interrupt Enable Register
 */
#define DISABLE_INT			(0x0)
/**
  Enable Stage 0
  */
#define ENABLE_STG0			(0x01)
/**
  \def AD7146_PWR_CTRL
  Power control Register
 */
#define AD7146_PWR_CTRL			(0x0)
/**
  \def AD7146_STG_CAL_EN_REG
  Calibration and Control Register
 */
#define AD7146_STG_CAL_EN_REG		(0x1)
/**
  \def AD7146_AMB_COMP_CTRL0_REG
  Device Control Register 0
 */
#define AD7146_AMB_COMP_CTRL0_REG	(0x2)
/**
  \def STG_LOW_INT_EN_REG
  Lower Threshold Interrupt Enable register
 */
#define STG_LOW_INT_EN_REG		(0x5)
/**
  \def STG_HIGH_INT_EN_REG
  Higher Threshold Interrupt Enable register
 */
#define STG_HIGH_INT_EN_REG		(0x6)
/**
  \def STG_COM_INT_EN_REG
  Conversion complete Interrupt Enable register
 */
#define STG_COM_INT_EN_REG		(0x7)
/**
  \def STG_LOW_INT_STA_REG
  Lower Threshold Interrupt Status register
 */
#define STG_LOW_INT_STA_REG		(0x8)
/**
  \def STG_HIGH_INT_STA_REG
  Higher Threshold Interrupt Status register
 */
#define STG_HIGH_INT_STA_REG		(0x9)
/**
  \def CDC_RESULT_S0_REG
  CDC Result of Stage 0 Register
 */
#define CDC_RESULT_S0_REG		(0xB)
/**
  Register address of Stage 0 Sensitivity
 */
#define DRIVER_STG0_AFEOFFSET		(0x82)
/**
  Register address of Stage 0 Sensitivity
 */
#define DRIVER_STG0_SENSITIVITY		(0x83)
/**
  Register address of Stage 0 LOW OFFSET
 */
#define LOW_OFFSET_0_REG		(0x84)
/**
  Register address of Stage 0 HIGH OFFSET
 */
#define HIGH_OFFSET_0_REG		(0x85)
/**
  Register address of Stage 0 HIGH CLAMP
 */
#define HIGH_OFFSET_CLAMP_0_REG		(0x86)
/**
  Register address of Stage 0 LOW CLAMP
 */
#define LOW_OFFSET_CLAMP_0_REG		(0x87)
/**
  \def DRIVER_STG0_SF_AMBIENT
  Register address of Stage 0 sf ambient value
 */
#define DRIVER_STG0_SF_AMBIENT		(0xF1)
/**
  \def DRIVER_STG0_HIGH_THRESHOLD
  Register address of Stage 0 High Threshold value
 */
#define DRIVER_STG0_HIGH_THRESHOLD	(0xFA)
/**
  Stage 0 Low threshold Register
 */
#define STG_0_LOW_THRESHOLD		(0x101)
/**
  Mask For the Force calibration
 */
#define AD7146_FORCED_CAL_MASK		(1<<14)

/**
  Negative Sensitivity Mask
 */
#define NEG_SENS			(0xF)
/**
  Positive Sensitivity Mask
 */
#define POS_SENS			(0xF00)
/**
  Used For the Disable of the interrupts
 */
#define DISABLE_INTERRUPTS		(0x0)
/**
  Used For the Enable of interrupts
 */
#define ENABLE_INTERRUPTS		(0x1)
/**
  AD7146 Full Scale value
*/
#define FULL_SCALE_VALUE		(0xFFFF)
/**
  AD7146 Full Scale value
*/
#define OVER_FLOW_SCALE_VALUE		(0x0005)

enum bank2_register_type {
	STAGE_CONNECTION_1,
	STAGE_CONNECTION_2,
	STAGE_AFE_OFFSET,
	STAGE_SENSITIVITY,
	STAGE_OFFSET_LOW,
	STAGE_OFFSET_HIGH,
	STAGE_OFFSET_HIGH_CLAMP,
	STAGE_OFFSET_LOW_CLAMP
};
/**
 * This structure provides chip information of AD7146.
 * \note Contains chip information of the AD7146 chip with attributes
 * like Product, Status ,version,drive data etc.
 * which are used in the control or to read the status of the device
 */
struct ad7146_chip {
	unsigned short high_status;
	unsigned short low_status;
	struct ad7146_platform_data *hw;
	struct input_dev *input;
	struct device *grip_dev;
	int irq;
	int grip_int;
	struct work_struct work;
	struct device *dev;
	struct wake_lock grip_wake_lock;
	ad7146_read_t read;
	ad7146_write_t write;

	/**
	power on grip status
	0 - Normal Mode
	1 - Half Grip Mode
	2 - Full Grip Mode
	*/
	unsigned short	    pw_on_grip_status;

	/*State check variables */
	unsigned short prevhigh;
	short prev_state_value;
	short state_value;
	/**
	event check to send or block the event in the driver
	0 - Disabled(Do not send event)
	1 - Enabled(Send event)
	*/
	unsigned short eventcheck;
	unsigned short during_cal_data;
	char onoff_flags;
	char cal_flags;
	char during_cal_flags;
	unsigned short stg0_low_offset;
	unsigned product;
	unsigned version;
};

#endif
