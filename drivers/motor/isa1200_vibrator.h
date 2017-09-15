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
#ifndef _ISA1200_VIBRATOR_H
#define _ISA1200_VIBRATOR_H

#include <mach/msm_iomap.h>

#define VIBRATION_ON            1
#define VIBRATION_OFF           0

#define HCTRL0     (0x30)     /* 0x09 */ /* Haptic Motor Driver Control Register Group 0*/
#define HCTRL1     (0x31)     /* 0x4B */ /* Haptic Motor Driver Control Register Group 1*/

#define MAX_INTENSITY		10000

#define GP_CLK_M_DEFAULT			2

#if defined (CONFIG_MACH_MATISSELTE_ATT)
#define GP_CLK_N_DEFAULT			122
#define GP_CLK_D_DEFAULT			61	/* 50% duty cycle */

#else
#if defined CONFIG_MACH_MATISSE3G_OPEN || defined CONFIG_SEC_MATISSELTE_COMMON
#define GP_CLK_N_DEFAULT			99
#define GP_CLK_D_DEFAULT			44	/* 50% duty cycle */
#else
#define GP_CLK_N_DEFAULT                       93
#define GP_CLK_D_DEFAULT                       47      /* 50% duty cycle */
#endif

#endif//CONFIG_MACH_MATISSELTE_ATT

#define IMM_PWM_MULTIPLIER		    GP_CLK_N_DEFAULT	/* Must be integer */
/*
 * ** Global variables for LRA PWM M,N and D values.
 * */

#define MOTOR_MIN_STRENGTH                      54/*IMMERSION VALUE*/
#define MOTOR_STRENGTH                  98/*MOTOR_STRENGTH 98 %*/
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

#define HWIO_GP1_CMD_RCGR_ADDR ((void __iomem *)(virt_mmss_gp1_base + 0))       //MMSS_CC_GP1_CMD_RCGR
#define HWIO_GP1_CFG_RCGR_ADDR ((void __iomem *)(virt_mmss_gp1_base + 4))       //MMSS_CC_GP1_CFG_RCGR
#define HWIO_GP_M_REG_ADDR ((void __iomem *)(virt_mmss_gp1_base + 8))   //MMSS_CC_GP1_M
#define HWIO_GP_NS_REG_ADDR ((void __iomem *)(virt_mmss_gp1_base + 0xc))        //MMSS_CC_GP1_N
#define HWIO_GP_D_REG_ADDR ((void __iomem *)(virt_mmss_gp1_base + 0x10))        //MMSS_CC_GP1_D

#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI) || defined(CONFIG_MACH_JS01LTEDCM)
        #define HWIO_CAMSS_GP1_CBCR_ADDR ((void __iomem *)(virt_mmss_gp1_base - 0x4))   //MMSS_CC_CAMSS_GP3_CBCR
#else
        #define HWIO_CAMSS_GP1_CBCR_ADDR ((void __iomem *)(virt_mmss_gp1_base + 0x24))  //MMSS_CC_CAMSS_GP1_CBCR
#endif
#define HWIO_GP_MD_REG_RMSK             0xffffffff
#define HWIO_GP_NS_REG_RMSK             0xffffffff

#define HWIO_GP_MD_REG_M_VAL_BMSK               0xff
#define HWIO_GP_MD_REG_M_VAL_SHFT               0
#define HWIO_GP_MD_REG_D_VAL_BMSK               0xff
#define HWIO_GP_MD_REG_D_VAL_SHFT               0
#define HWIO_GP_NS_REG_GP_N_VAL_BMSK    0xff
#define HWIO_GP_SRC_SEL_VAL_BMSK                0x700
#define HWIO_GP_SRC_SEL_VAL_SHFT                8
#define HWIO_GP_SRC_DIV_VAL_BMSK                0x1f
#define HWIO_GP_SRC_DIV_VAL_SHFT                0
#define HWIO_GP_MODE_VAL_BMSK                   0x3000
#define HWIO_GP_MODE_VAL_SHFT                   12

#define HWIO_CLK_ENABLE_VAL_BMSK        0x1
#define HWIO_CLK_ENABLE_VAL_SHFT        0
#define HWIO_UPDATE_VAL_BMSK    0x1
#define HWIO_UPDATE_VAL_SHFT    0
#define HWIO_ROOT_EN_VAL_BMSK   0x2
#define HWIO_ROOT_EN_VAL_SHFT   1


#define HWIO_GP1_CMD_RCGR_IN            \
                in_dword_masked(HWIO_GP1_CMD_RCGR_ADDR, HWIO_GP_NS_REG_RMSK)
#define HWIO_GP1_CMD_RCGR_OUTM(m, v)    \
        out_dword_masked_ns(HWIO_GP1_CMD_RCGR_ADDR, m, v, HWIO_GP1_CMD_RCGR_IN)



#define HWIO_GP1_CFG_RCGR_IN            \
                in_dword_masked(HWIO_GP1_CFG_RCGR_ADDR, HWIO_GP_NS_REG_RMSK)
#define HWIO_GP1_CFG_RCGR_OUTM(m, v)    \
        out_dword_masked_ns(HWIO_GP1_CFG_RCGR_ADDR, m, v, HWIO_GP1_CFG_RCGR_IN)



#define HWIO_CAMSS_GP1_CBCR_IN          \
                in_dword_masked(HWIO_CAMSS_GP1_CBCR_ADDR, HWIO_GP_NS_REG_RMSK)
#define HWIO_CAMSS_GP1_CBCR_OUTM(m, v)  \
       out_dword_masked_ns(HWIO_CAMSS_GP1_CBCR_ADDR, m, v, HWIO_CAMSS_GP1_CBCR_IN)

#define HWIO_GP_D_REG_IN                \
                in_dword_masked(HWIO_GP_D_REG_ADDR, HWIO_GP_MD_REG_RMSK)

#define HWIO_GP_D_REG_OUTM(m, v)\
        out_dword_masked_ns(HWIO_GP_D_REG_ADDR, m, v, HWIO_GP_D_REG_IN)


#define HWIO_GP_M_REG_IN                \
                in_dword_masked(HWIO_GP_M_REG_ADDR, HWIO_GP_MD_REG_RMSK)
#define HWIO_GP_M_REG_OUTM(m, v)\
        out_dword_masked_ns(HWIO_GP_M_REG_ADDR, m, v, HWIO_GP_M_REG_IN)


#define HWIO_GP_NS_REG_IN               \
                in_dword_masked(HWIO_GP_NS_REG_ADDR, HWIO_GP_NS_REG_RMSK)
#define HWIO_GP_NS_REG_OUTM(m, v)       \
        out_dword_masked_ns(HWIO_GP_NS_REG_ADDR, m, v, HWIO_GP_NS_REG_IN)


#define __msmhwio_outm(hwiosym, mask, val)  HWIO_##hwiosym##_OUTM(mask, val)
#define HWIO_OUTM(hwiosym, mask, val)   __msmhwio_outm(hwiosym, mask, val)

#endif

int vibrator_write_register(u8 addr, u8 w_data);

extern struct vibrator_platform_data_isa1200 vibrator_drvdata;
#endif
