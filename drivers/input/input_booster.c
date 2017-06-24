/*
 * input_booster.c - touch booster driver
 *
 * Copyright (C) 2014 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/input/input_booster.h>

static void input_booster_change_dvfs_tsp_work(struct work_struct *work)
{
	int retval = 0;
	struct input_booster *booster =
		container_of(work,
				struct input_booster, work_dvfs_chg.work);

	mutex_lock(&booster->dvfs_lock);

	switch (booster->dvfs_boost_mode) {
	case DVFS_STAGE_SINGLE:
	case DVFS_STAGE_TRIPLE:
	case DVFS_STAGE_PENTA:
		retval = set_freq_limit(DVFS_TOUCH_ID, -1);
		booster->dvfs_freq = -1;
		break;
	case DVFS_STAGE_DUAL:
		retval = set_freq_limit(DVFS_TOUCH_ID,
				MIN_TOUCH_LIMIT_SECOND);
		booster->dvfs_freq = MIN_TOUCH_LIMIT_SECOND;
		break;
	case DVFS_STAGE_NINTH:
		retval = set_freq_limit(DVFS_TOUCH_ID,
				MIN_TOUCH_LIMIT);
		booster->dvfs_freq = MIN_TOUCH_LIMIT;
		break;
	}

	if (retval < 0)
		pr_err("%s: booster change failed(%d).\n",
				__func__, retval);
	mutex_unlock(&booster->dvfs_lock);
}

static void input_booster_set_dvfs_tsp_lock(struct input_booster *booster, int on)
{
	int retval = 0;

	if (booster->dvfs_boost_mode == DVFS_STAGE_NONE) {
		pr_debug("%s: DVFS stage is none(%d)\n",
				__func__, booster->dvfs_boost_mode);
		return;
	}

	mutex_lock(&booster->dvfs_lock);

	if (on == 0) {
		if (booster->dvfs_lock_status) {
			switch (booster->dvfs_boost_mode) {
			case DVFS_STAGE_SINGLE:
			case DVFS_STAGE_DUAL:
			case DVFS_STAGE_TRIPLE:
			case DVFS_STAGE_PENTA:
				schedule_delayed_work(&booster->work_dvfs_off,
					msecs_to_jiffies(INPUT_BOOSTER_OFF_TIME_TSP));
				break;
			case DVFS_STAGE_NINTH:
				schedule_delayed_work(&booster->work_dvfs_off,
					msecs_to_jiffies(INPUT_BOOSTER_HIGH_OFF_TIME_TSP));
				break;
			}
		}
	} else if (on > 0) {
		cancel_delayed_work(&booster->work_dvfs_off);

		if ((!booster->dvfs_lock_status) || (booster->dvfs_old_stauts < on)) {
			cancel_delayed_work(&booster->work_dvfs_chg);

			switch (booster->dvfs_boost_mode) {
			case DVFS_STAGE_SINGLE:
			case DVFS_STAGE_DUAL:
				if (booster->dvfs_freq != MIN_TOUCH_LIMIT) {
					retval = set_freq_limit(DVFS_TOUCH_ID,
							MIN_TOUCH_LIMIT);
					booster->dvfs_freq = MIN_TOUCH_LIMIT;
				}
				schedule_delayed_work(&booster->work_dvfs_chg,
					msecs_to_jiffies(INPUT_BOOSTER_CHG_TIME_TSP));
				break;
			case DVFS_STAGE_TRIPLE:
				if (booster->dvfs_freq != MIN_TOUCH_LIMIT_SECOND) {
					retval = set_freq_limit(DVFS_TOUCH_ID,
							MIN_TOUCH_LIMIT_SECOND);
					booster->dvfs_freq = MIN_TOUCH_LIMIT_SECOND;
				}
				schedule_delayed_work(&booster->work_dvfs_chg,
					msecs_to_jiffies(INPUT_BOOSTER_CHG_TIME_TSP));
				break;
			case DVFS_STAGE_PENTA:
				if (booster->dvfs_freq != MIN_TOUCH_LOW_LIMIT) {
					retval = set_freq_limit(DVFS_TOUCH_ID,
							MIN_TOUCH_LOW_LIMIT);
					booster->dvfs_freq = MIN_TOUCH_LOW_LIMIT;
				}
				schedule_delayed_work(&booster->work_dvfs_chg,
					msecs_to_jiffies(INPUT_BOOSTER_CHG_TIME_TSP));
				break;
			case DVFS_STAGE_NINTH:
				if (booster->dvfs_freq != MIN_TOUCH_HIGH_LIMIT) {
					retval = set_freq_limit(DVFS_TOUCH_ID,
							MIN_TOUCH_HIGH_LIMIT);
					booster->dvfs_freq = MIN_TOUCH_HIGH_LIMIT;
				}
				schedule_delayed_work(&booster->work_dvfs_chg,
					msecs_to_jiffies(INPUT_BOOSTER_HIGH_CHG_TIME_TSP));
				break;

			}

			if (retval < 0)
				pr_err("%s: cpu first lock failed(%d)\n",
						__func__, retval);

			booster->dvfs_lock_status = true;
		}

	} else if (on < 0) {
		if (booster->dvfs_lock_status) {
			cancel_delayed_work(&booster->work_dvfs_off);
			cancel_delayed_work(&booster->work_dvfs_chg);
			schedule_work(&booster->work_dvfs_off.work);
		}
	}

	booster->dvfs_old_stauts = on;
	mutex_unlock(&booster->dvfs_lock);
}

static void input_booster_change_dvfs_tkey_work(struct work_struct *work)
{
	int retval = 0;
	struct input_booster *booster =
		container_of(work,
				struct input_booster, work_dvfs_chg.work);

	mutex_lock(&booster->dvfs_lock);

	retval = set_freq_limit(DVFS_TOUCH_ID, booster->dvfs_freq);
	if (retval < 0)
		pr_info("%s: booster change failed(%d).\n",
			__func__, retval);

	booster->dvfs_lock_status = false;
	mutex_unlock(&booster->dvfs_lock);
}

static void input_booster_set_dvfs_tkey_lock(struct input_booster *booster, int on)
{
	int retval = 0;
	if (booster->dvfs_boost_mode == DVFS_STAGE_NONE) {
		pr_debug("%s: DVFS stage is none(%d)\n",
				__func__, booster->dvfs_boost_mode);
		return;
	}

	mutex_lock(&booster->dvfs_lock);
	if (on == 0) {
		cancel_delayed_work(&booster->work_dvfs_chg);

		if (booster->dvfs_lock_status) {
			retval = set_freq_limit(DVFS_TOUCH_ID, booster->dvfs_freq);

			if (retval < 0)
				pr_info("%s: cpu first lock failed(%d)\n", __func__, retval);
			booster->dvfs_lock_status = false;
		}

		schedule_delayed_work(&booster->work_dvfs_off,
			msecs_to_jiffies(INPUT_BOOSTER_CHG_TIME_TKEY));

	} else if (on == 1) {
		cancel_delayed_work(&booster->work_dvfs_off);
		schedule_delayed_work(&booster->work_dvfs_chg,
			msecs_to_jiffies(INPUT_BOOSTER_OFF_TIME_TKEY));

	} else if (on == 2) {
		if (booster->dvfs_lock_status) {
			cancel_delayed_work(&booster->work_dvfs_off);
			cancel_delayed_work(&booster->work_dvfs_chg);
			schedule_work(&booster->work_dvfs_off.work);
		}
	}
	mutex_unlock(&booster->dvfs_lock);
}

static void input_booster_change_dvfs_wacom_work(struct work_struct *work)
{
	int retval = 0;
	struct input_booster *booster =
		container_of(work,
				struct input_booster, work_dvfs_chg.work);

	mutex_lock(&booster->dvfs_lock);

	switch (booster->dvfs_boost_mode) {
	case DVFS_STAGE_SINGLE:
	case DVFS_STAGE_TRIPLE:
		retval = set_freq_limit(DVFS_TOUCH_ID, -1);
		booster->dvfs_freq = -1;
		break;
	case DVFS_STAGE_DUAL:
		retval = set_freq_limit(DVFS_TOUCH_ID,
				MIN_TOUCH_LIMIT_SECOND);
		booster->dvfs_freq = MIN_TOUCH_LIMIT_SECOND;
		break;
	}

	if (retval < 0)
		pr_info("%s: booster change failed(%d).\n",
			__func__, retval);

	mutex_unlock(&booster->dvfs_lock);
}

static void input_booster_set_dvfs_wacom_lock(struct input_booster *booster, int on)
{
	int retval = 0;
	if (booster->dvfs_boost_mode == DVFS_STAGE_NONE) {
		pr_debug("%s: DVFS stage is none(%d)\n",
				__func__, booster->dvfs_boost_mode);
		return;
	}

	mutex_lock(&booster->dvfs_lock);

	if (on == 0) {
		if (booster->dvfs_lock_status) {
			schedule_delayed_work(&booster->work_dvfs_off,
				msecs_to_jiffies(INPUT_BOOSTER_OFF_TIME_WACOM));
		}
	} else if (on > 0) {
		cancel_delayed_work(&booster->work_dvfs_off);

		if (!booster->dvfs_lock_status || booster->dvfs_old_stauts < on) {
			cancel_delayed_work(&booster->work_dvfs_chg);

			switch (booster->dvfs_boost_mode) {
			case DVFS_STAGE_SINGLE:
			case DVFS_STAGE_DUAL:
				if (booster->dvfs_freq != MIN_TOUCH_LIMIT) {
					retval = set_freq_limit(DVFS_TOUCH_ID,
							MIN_TOUCH_LIMIT);
					booster->dvfs_freq = MIN_TOUCH_LIMIT;
				}
				schedule_delayed_work(&booster->work_dvfs_chg,
					msecs_to_jiffies(INPUT_BOOSTER_CHG_TIME_WACOM));
				break;
			case DVFS_STAGE_TRIPLE:
				if (booster->dvfs_freq != MIN_TOUCH_LIMIT_SECOND) {
					retval = set_freq_limit(DVFS_TOUCH_ID,
							MIN_TOUCH_LIMIT_SECOND);
					booster->dvfs_freq = MIN_TOUCH_LIMIT_SECOND;
				}
				schedule_delayed_work(&booster->work_dvfs_chg,
					msecs_to_jiffies(INPUT_BOOSTER_CHG_TIME_WACOM));
				break;
			}

			if (retval < 0)
				pr_info("%s: cpu first lock failed(%d)\n",
						__func__, retval);
			schedule_delayed_work(&booster->work_dvfs_chg,
				msecs_to_jiffies(INPUT_BOOSTER_CHG_TIME_WACOM));

			booster->dvfs_lock_status = true;
		}

	} else if (on < 0) {
		if (booster->dvfs_lock_status) {
			cancel_delayed_work(&booster->work_dvfs_off);
			cancel_delayed_work(&booster->work_dvfs_chg);
			schedule_work(&booster->work_dvfs_off.work);
		}
	}
	booster->dvfs_old_stauts = on;

	mutex_unlock(&booster->dvfs_lock);
}

static int input_booster_set_dvfs_off(struct input_booster *booster)
{
	int retval;

	mutex_lock(&booster->dvfs_lock);

	retval = set_freq_limit(DVFS_TOUCH_ID, -1);
	if (retval < 0)
		pr_err("%s: booster stop failed(%d).\n",
					__func__, retval);

	if (booster->dvfs_id == INPUT_BOOSTER_ID_TKEY) {
		if (retval < 0)
			booster->dvfs_lock_status = false;
		else
			booster->dvfs_lock_status = true;
	} else {
		booster->dvfs_freq = -1;
		booster->dvfs_lock_status = false;
	}

	mutex_unlock(&booster->dvfs_lock);

	return retval;
}

static void input_booster_off_dvfs_work(struct work_struct *work)
{
	struct input_booster *booster =
		container_of(work,
				struct input_booster, work_dvfs_off.work);

	input_booster_set_dvfs_off(booster);
}

void input_booster_init_dvfs(struct input_booster *booster, int id)
{
	booster->dvfs_id = id;
	mutex_init(&booster->dvfs_lock);

	switch (booster->dvfs_id) {
	case INPUT_BOOSTER_ID_TSP:
		booster->dvfs_set = input_booster_set_dvfs_tsp_lock;
		INIT_DELAYED_WORK(&booster->work_dvfs_chg,
				input_booster_change_dvfs_tsp_work);
		booster->dvfs_stage = DVFS_TSP_STAGE;
		break;
	case INPUT_BOOSTER_ID_TKEY:
		booster->dvfs_set = input_booster_set_dvfs_tkey_lock;
		INIT_DELAYED_WORK(&booster->work_dvfs_chg,
				input_booster_change_dvfs_tkey_work);
		booster->dvfs_stage = DVFS_TKEY_STAGE;
		break;
	case INPUT_BOOSTER_ID_WACOM:
		booster->dvfs_set = input_booster_set_dvfs_wacom_lock;
		INIT_DELAYED_WORK(&booster->work_dvfs_chg,
				input_booster_change_dvfs_wacom_work);
		booster->dvfs_stage = DVFS_WACOM_STAGE;
		break;
	default:
		booster->dvfs_off = NULL;
		booster->dvfs_set = NULL;
		pr_err("%s: Booster ID is not defined\n", __func__);
		return;
	}

	booster->dvfs_off = input_booster_set_dvfs_off;
	INIT_DELAYED_WORK(&booster->work_dvfs_off, input_booster_off_dvfs_work);

	if(booster->dvfs_stage & DVFS_STAGE_DUAL)
		booster->dvfs_boost_mode = DVFS_STAGE_DUAL;
	else
		booster->dvfs_boost_mode = DVFS_STAGE_NONE;

	if (booster->dvfs_id == INPUT_BOOSTER_ID_TKEY) {
		booster->dvfs_freq = MIN_TOUCH_LIMIT_SECOND;
		booster->dvfs_lock_status = true;
	} else {
		booster->dvfs_lock_status = false;
	}

	pr_err("%s: booster stage: 0x%04x\n", __func__, booster->dvfs_stage);
}

MODULE_DESCRIPTION("Input device[Touch, Touchkey, Hardkey] booster");
MODULE_LICENSE("GPL");
