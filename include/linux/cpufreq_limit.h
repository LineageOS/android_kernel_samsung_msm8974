/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *	Minsung Kim <ms925.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LINUX_CPUFREQ_LIMIT_H__
#define __LINUX_CPUFREQ_LIMIT_H__

struct cpufreq_limit_handle;

#ifdef CONFIG_CPU_FREQ_LIMIT

#if defined(CONFIG_ARCH_MSM8974PRO)
#define MAX_FREQ_LIMIT	2457600
#else
#define MAX_FREQ_LIMIT	2265600
#endif
#define MIN_FREQ_LIMIT	300000

struct cpufreq_limit_handle *cpufreq_limit_get(unsigned long min_freq,
		unsigned long max_freq, char *label);
int cpufreq_limit_put(struct cpufreq_limit_handle *handle);

static inline
struct cpufreq_limit_handle *cpufreq_limit_min_freq(unsigned long min_freq,
						    char *label)
{
	return cpufreq_limit_get(min_freq, 0, label);
}

static inline
struct cpufreq_limit_handle *cpufreq_limit_max_freq(unsigned long max_freq,
						    char *label)
{
	return cpufreq_limit_get(0, max_freq, label);
}
#else
static inline
struct cpufreq_limit_handle *cpufreq_limit_get(unsigned long min_freq,
		unsigned long max_freq char *label)
{
	return NULL;
}

int cpufreq_limit_put(struct cpufreq_limit_handle *handle)
{
	return 0;
}

static inline
struct cpufreq_limit_handle *cpufreq_limit_min_freq(unsigned long min_freq,
						    char *label)
{
	return NULL;
}

static inline
struct cpufreq_limit_handle *cpufreq_limit_max_freq(unsigned long max_freq,
						    char *label)
{
	return NULL;
}
#endif
#endif /* __LINUX_CPUFREQ_LIMIT_H__ */
