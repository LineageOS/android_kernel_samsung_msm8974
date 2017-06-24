/*
 * include/linux/rtdefs.h
 *
 * Richtek driver common definitions
 *
 * Copyright (C) 2013 Richtek Technology Corp.
 * Author: Patrick Chang <patrick_chang@richtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef RTDEFS_H
#define RTDEFS_H
#include <linux/kernel.h>

#ifndef RTDBGLEVEL
#define RTDBGLEVEL  3
#endif

#define RTDBGINFO_LEVEL 3
#define RTDBGWARN_LEVEL 2
#define RTDBGERR_LEVEL  1
#define RTDBGFPRN_LEVEL 0


#if RTDBGINFO_LEVEL<=RTDBGLEVEL
#define RTINFO(format, args...) \
    printk(KERN_INFO "%s:%s() line-%d: " format, \
            ALIAS_NAME, __FUNCTION__, __LINE__, ## args)
#define RTINFO_IF(cond, format, args...) \
    if (cond) \
        printk(KERN_INFO "%s:%s() line-%d: " format, \
                ALIAS_NAME, __FUNCTION__, __LINE__, ## args)
#else
#define RTINFO(format, args...)
#define RTINFO_IF(cond, format, args...)
#endif

#if RTDBGWARN_LEVEL<=RTDBGLEVEL
#define RTWARN(format, args...) \
    printk(KERN_WARNING "%s:%s() line-%d: " format, \
            ALIAS_NAME, __FUNCTION__, __LINE__, ## args)
#define RTWARN_IF(cond, format, args...) \
    if (cond) \
        printk(KERN_WARNING "%s:%s() line-%d: " format, \
                ALIAS_NAME, __FUNCTION__, __LINE__, ## args)
#else
#define RTWARN(format, args...)
#define RTWARN_IF(cond, format, args...)
#endif

#if RTDBGERR_LEVEL<=RTDBGLEVEL
#define RTERR(format, args...) \
    printk(KERN_ERR "%s:%s() line-%d: " format, \
            ALIAS_NAME, __FUNCTION__, __LINE__, ## args)
#define RTERR_IF(cond, format, args...) \
    if (cond) printk(KERN_ERR "%s:%s() line-%d: " format, \
            ALIAS_NAME, __FUNCTION__, __LINE__, ## args)
#else
#define RTERR(format, args...)
#define RTERR_IF(cond, format, args...)
#endif

#if RTDBGFPRN_LEVEL<=RTDBGLEVEL
#define RTPRN(format, args...) \
    printk(KERN_DEBUG "%s:%s() line-%d: " format, \
            ALIAS_NAME, __FUNCTION__, __LINE__, ## args)
#define RTPRN_IF(cond, format, args...) \
    if (cond) printk(KERN_DEBUG "%s:%s() line-%d: " format, \
            ALIAS_NAME, __FUNCTION__, __LINE__, ## args)
#else
#define RTPRN(format, args...)
#define RTPRN_IF(cond, format, args...)
#endif



#endif // RTDEFS_H
