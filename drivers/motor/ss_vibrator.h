
/* Copyright (c) 2016, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _VIBRATOR_H
#define _VIBRATOR_H

extern struct vibrator_platform_data vibrator_drvdata;

struct pm_gpio vib_pwm = {
	.direction = PM_GPIO_DIR_OUT,
	.output_buffer = 0,
	.output_value = 0,
	.pull = PM_GPIO_PULL_NO,
	.vin_sel = 0,
	.out_strength = PM_GPIO_STRENGTH_HIGH,
	.function = PM_GPIO_FUNC_1,
	.inv_int_pol = 0,
};

/* Error and Return value codes */
#define VIBRATION_SUCCESS	0	/* Success */
#define VIBRATION_FAIL		-1	/* Generic error */
#define VIBRATION_ON		1
#define VIBRATION_OFF		0

#define DEFAULT_INTENSITY	5000
#define MAX_INTENSITY		10000

#if defined(CONFIG_MACH_KS01SKT) \
	   || defined(CONFIG_MACH_KS01KTT) || defined(CONFIG_MACH_KS01LGT) \
	   || defined(CONFIG_MACH_JACTIVESKT) || defined(CONFIG_MACH_HLTEDCM) \
	   || defined(CONFIG_MACH_HLTEKDI)
#define MOTOR_STRENGTH			94/*MOTOR_STRENGTH 94 %*/
#elif defined(CONFIG_MACH_LT03EUR) || defined(CONFIG_MACH_LT03SKT)\
	|| defined(CONFIG_MACH_LT03KTT)	|| defined(CONFIG_MACH_LT03LGT)
#define MOTOR_STRENGTH			98/*MOTOR_STRENGTH 98 %*/
#elif defined(CONFIG_MACH_PICASSO_LTE)
#define MOTOR_STRENGTH			91
#elif defined(CONFIG_MACH_HLTEUSC) || defined(CONFIG_MACH_HLTEVZW)
#define MOTOR_STRENGTH			99/*MOTOR_STRENGTH 99 %*/
#elif defined(CONFIG_MACH_KACTIVELTE_KOR)
#define MOTOR_STRENGTH			95/*MOTOR_STRENGTH 95 %*/
#elif defined(CONFIG_SEC_K_PROJECT) || defined(CONFIG_SEC_KACTIVE_PROJECT) || defined(CONFIG_SEC_KSPORTS_PROJECT) \
	|| defined(CONFIG_SEC_PATEK_PROJECT)
#define MOTOR_STRENGTH			98/*MOTOR_STRENGTH 98 %*/
#elif defined(CONFIG_SEC_S_PROJECT)
#define MOTOR_STRENGTH			87/*MOTOR_STRENGTH 87 %*/
#else
#define MOTOR_STRENGTH			98/*MOTOR_STRENGTH 98 %*/
#endif


#if defined (CONFIG_MACH_HLTESPR) || defined (CONFIG_MACH_HLTEEUR) || defined(CONFIG_SEC_LOCALE_KOR_H) || defined (CONFIG_MACH_HLTETMO) || defined(CONFIG_MACH_H3GDUOS) || defined(CONFIG_MACH_HLTEATT)
	#define GP_CLK_M_DEFAULT                        3
	#define GP_CLK_N_DEFAULT                        138
	#define GP_CLK_D_DEFAULT                        69  /* 50% duty cycle	*/
	#define IMM_PWM_MULTIPLIER			137
#elif defined (CONFIG_MACH_HLTEDCM) || defined (CONFIG_MACH_HLTEKDI) || defined (CONFIG_MACH_JS01LTEDCM) || defined (CONFIG_MACH_JS01LTESBM)
	#define GP_CLK_M_DEFAULT			2
	#define GP_CLK_N_DEFAULT			92
	#define GP_CLK_D_DEFAULT			46  /* 50% duty cycle */
	#define IMM_PWM_MULTIPLIER		92
#elif defined (CONFIG_MACH_HLTEUSC) || defined(CONFIG_MACH_HLTEVZW)
	#define GP_CLK_M_DEFAULT			1
	#define GP_CLK_N_DEFAULT			46
	#define GP_CLK_D_DEFAULT			23  /* 50% duty cycle */
	#define IMM_PWM_MULTIPLIER		46
#elif defined(CONFIG_MACH_FLTESKT)
	#define GP_CLK_M_DEFAULT			2
	#define GP_CLK_N_DEFAULT                        92
	#define GP_CLK_D_DEFAULT			46  /* 50% duty cycle */
	#define IMM_PWM_MULTIPLIER			92
#elif defined(CONFIG_SEC_K_PROJECT) || defined(CONFIG_SEC_KACTIVE_PROJECT) || defined(CONFIG_SEC_KSPORTS_PROJECT) ||\
	defined(CONFIG_SEC_PATEK_PROJECT)
#if defined(CONFIG_MACH_KLTE_MAX77828_JPN)
	#define GP_CLK_M_DEFAULT			1
	#define GP_CLK_N_DEFAULT                        20
	#define GP_CLK_D_DEFAULT			10  /* 50% duty cycle */
	#define IMM_PWM_MULTIPLIER			20
#else
	#define GP_CLK_M_DEFAULT			3
	#define GP_CLK_N_DEFAULT                        121
	#define GP_CLK_D_DEFAULT			61  /* 50% duty cycle */
	#define IMM_PWM_MULTIPLIER			121
#endif
#elif defined(CONFIG_SEC_S_PROJECT)
	#define GP_CLK_M_DEFAULT			3
	#define GP_CLK_N_DEFAULT                        121
	#define GP_CLK_D_DEFAULT			61  /* 50% duty cycle */
	#define IMM_PWM_MULTIPLIER			121
#elif defined(CONFIG_SEC_LOCALE_KOR_FRESCO)
	#define GP_CLK_M_DEFAULT			3
	#define GP_CLK_N_DEFAULT                        120
	#define GP_CLK_D_DEFAULT			60  /* 50% duty cycle */
	#define IMM_PWM_MULTIPLIER			120
#elif defined(CONFIG_SEC_BERLUTI_PROJECT) || defined(CONFIG_MACH_S3VE3G_EUR)
	#define GP_CLK_M_DEFAULT			1
	#define GP_CLK_N_DEFAULT			61
	#define GP_CLK_D_DEFAULT			31  /* 50% duty cycle */
	#define IMM_PWM_MULTIPLIER			61
#elif defined(CONFIG_MACH_HESTIALTE_EUR)
	#define GP_CLK_M_DEFAULT			1
	#define GP_CLK_N_DEFAULT			40
	#define GP_CLK_D_DEFAULT			20
	#define IMM_PWM_MULTIPLIER			40
#else
	#define GP_CLK_M_DEFAULT			2
	#define GP_CLK_N_DEFAULT                        91
	#define GP_CLK_D_DEFAULT			46  /* 50% duty cycle */
	#define IMM_PWM_MULTIPLIER			91
#endif


#define MOTOR_MIN_STRENGTH			54/*IMMERSION VALUE*/
/*
 * ** Global variables for LRA PWM M,N and D values.
 * */
int32_t g_nlra_gp_clk_m = GP_CLK_M_DEFAULT;
int32_t g_nlra_gp_clk_n = GP_CLK_N_DEFAULT;
int32_t g_nlra_gp_clk_d = GP_CLK_D_DEFAULT;
int32_t g_nlra_gp_clk_pwm_mul = IMM_PWM_MULTIPLIER;
int32_t motor_strength = MOTOR_STRENGTH;
int32_t motor_min_strength;

#define __inp(port) ioread8(port)

#define __inpw(port) ioread16(port)

#define __inpdw(port) ioread32(port)

#define __outp(port, val) iowrite8(val, port)

#define __outpw(port, val) iowrite16(val, port)

#define __outpdw(port, val) iowrite32(val, port)

#define in_dword(addr)              (__inpdw(addr))
#define in_dword_masked(addr, mask) (__inpdw(addr) & (mask))
#define out_dword(addr, val)        __outpdw(addr, val)
#define out_dword_masked(io, mask, val, shadow)  \
	(void) out_dword(io, \
	((shadow & (unsigned int)(~(mask))) | ((unsigned int)((val) & (mask)))))
#define out_dword_masked_ns(io, mask, val, current_reg_content) \
	(void) out_dword(io, \
	((current_reg_content & (unsigned int)(~(mask))) \
	| ((unsigned int)((val) & (mask)))))

#if !defined(CONFIG_MOTOR_DRV_ISA1400)
static void __iomem *virt_mmss_gp1_base;

#define HWIO_GP1_CMD_RCGR_ADDR ((void __iomem *)(virt_mmss_gp1_base + 0))	//MMSS_CC_GP1_CMD_RCGR
#define HWIO_GP1_CFG_RCGR_ADDR ((void __iomem *)(virt_mmss_gp1_base + 4))	//MMSS_CC_GP1_CFG_RCGR
#define HWIO_GP_M_REG_ADDR ((void __iomem *)(virt_mmss_gp1_base + 8))	//MMSS_CC_GP1_M
#define HWIO_GP_NS_REG_ADDR ((void __iomem *)(virt_mmss_gp1_base + 0xc))	//MMSS_CC_GP1_N
#define HWIO_GP_D_REG_ADDR ((void __iomem *)(virt_mmss_gp1_base + 0x10))	//MMSS_CC_GP1_D

#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI) || \
	defined(CONFIG_MACH_JS01LTEDCM) || defined(CONFIG_MACH_JS01LTESBM)
	#define HWIO_CAMSS_GP1_CBCR_ADDR ((void __iomem *)(virt_mmss_gp1_base - 0x4))	//MMSS_CC_CAMSS_GP3_CBCR
#else
	#define HWIO_CAMSS_GP1_CBCR_ADDR ((void __iomem *)(virt_mmss_gp1_base + 0x24))	//MMSS_CC_CAMSS_GP1_CBCR
#endif

#define HWIO_GP_MD_REG_RMSK		0xffffffff
#define HWIO_GP_NS_REG_RMSK		0xffffffff

#define HWIO_GP_MD_REG_M_VAL_BMSK		0xff
#define HWIO_GP_MD_REG_M_VAL_SHFT		0
#define HWIO_GP_MD_REG_D_VAL_BMSK		0xff
#define HWIO_GP_MD_REG_D_VAL_SHFT		0
#define HWIO_GP_NS_REG_GP_N_VAL_BMSK	0xff
#define HWIO_GP_SRC_SEL_VAL_BMSK		0x700
#define HWIO_GP_SRC_SEL_VAL_SHFT		8
#define HWIO_GP_SRC_DIV_VAL_BMSK		0x1f
#define HWIO_GP_SRC_DIV_VAL_SHFT		0
#define HWIO_GP_MODE_VAL_BMSK			0x3000
#define HWIO_GP_MODE_VAL_SHFT			12

#define HWIO_CLK_ENABLE_VAL_BMSK	0x1
#define HWIO_CLK_ENABLE_VAL_SHFT	0
#define HWIO_UPDATE_VAL_BMSK	0x1
#define HWIO_UPDATE_VAL_SHFT	0
#define HWIO_ROOT_EN_VAL_BMSK	0x2
#define HWIO_ROOT_EN_VAL_SHFT	1


#define HWIO_GP1_CMD_RCGR_IN		\
		in_dword_masked(HWIO_GP1_CMD_RCGR_ADDR, HWIO_GP_NS_REG_RMSK)
#define HWIO_GP1_CMD_RCGR_OUTM(m, v)	\
	out_dword_masked_ns(HWIO_GP1_CMD_RCGR_ADDR, m, v, HWIO_GP1_CMD_RCGR_IN)



#define HWIO_GP1_CFG_RCGR_IN		\
		in_dword_masked(HWIO_GP1_CFG_RCGR_ADDR, HWIO_GP_NS_REG_RMSK)
#define HWIO_GP1_CFG_RCGR_OUTM(m, v)	\
	out_dword_masked_ns(HWIO_GP1_CFG_RCGR_ADDR, m, v, HWIO_GP1_CFG_RCGR_IN)



#define HWIO_CAMSS_GP1_CBCR_IN		\
		in_dword_masked(HWIO_CAMSS_GP1_CBCR_ADDR, HWIO_GP_NS_REG_RMSK)
#define HWIO_CAMSS_GP1_CBCR_OUTM(m, v)	\
	out_dword_masked_ns(HWIO_CAMSS_GP1_CBCR_ADDR, m, v, HWIO_CAMSS_GP1_CBCR_IN)

#define HWIO_GP_D_REG_IN		\
		in_dword_masked(HWIO_GP_D_REG_ADDR, HWIO_GP_MD_REG_RMSK)

#define HWIO_GP_D_REG_OUTM(m, v)\
	out_dword_masked_ns(HWIO_GP_D_REG_ADDR, m, v, HWIO_GP_D_REG_IN)


#define HWIO_GP_M_REG_IN		\
		in_dword_masked(HWIO_GP_M_REG_ADDR, HWIO_GP_MD_REG_RMSK)
#define HWIO_GP_M_REG_OUTM(m, v)\
	out_dword_masked_ns(HWIO_GP_M_REG_ADDR, m, v, HWIO_GP_M_REG_IN)


#define HWIO_GP_NS_REG_IN		\
		in_dword_masked(HWIO_GP_NS_REG_ADDR, HWIO_GP_NS_REG_RMSK)
#define HWIO_GP_NS_REG_OUTM(m, v)	\
	out_dword_masked_ns(HWIO_GP_NS_REG_ADDR, m, v, HWIO_GP_NS_REG_IN)


#define __msmhwio_outm(hwiosym, mask, val)  HWIO_##hwiosym##_OUTM(mask, val)
#define HWIO_OUTM(hwiosym, mask, val)	__msmhwio_outm(hwiosym, mask, val)
int32_t vibe_pwm_onoff(u8 onoff);
int32_t vibe_set_pwm_freq(int intensity);
int vib_config_pwm_device(void);
#endif

#if defined(CONFIG_MOTOR_DRV_MAX77803)
extern void max77803_vibtonz_en(bool en);
#elif defined(CONFIG_MOTOR_DRV_MAX77804K)
extern void max77804k_vibtonz_en(bool en);
#elif defined(CONFIG_MOTOR_DRV_MAX77828)
extern void max77828_vibtonz_en(bool en);
#elif defined(CONFIG_MOTOR_DRV_MAX77888)
void max77888_gpio_en(bool);
void max77888_vibtonz_en(bool);
static int32_t max77888_gpio_init(void);
#elif defined(CONFIG_MOTOR_DRV_DRV2603)
void drv2603_gpio_en(bool);
static int32_t drv2603_gpio_init(void);
#endif

#endif  /* _VIBRATOR_H */
