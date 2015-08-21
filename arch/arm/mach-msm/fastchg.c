/*
 * Author: Chad Froebel <chadfroebel@gmail.com>
 *
 * Ported to Note 3 (n9005) and extended : Jean-Pierre Rasquin <yank555.lu@gmail.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
 * Forced Fast Charge - SysFS interface :
 * --------------------------------------
 *
 * /sys/kernel/fast_charge/force_fast_charge (rw)
 *
 *   0 - disabled (default)
 *   1 - substitute AC to USB
 *   2 - use custom mA configured through sysfs interface (see below)
 *
 * /sys/kernel/fast_charge/use_mtp_during_fast_charge (rw)
 *
 *   0 - disabled
 *   1 - enabled (default)
 *
 * /sys/kernel/fast_charge/screen_on_current_limit (rw)
 *
 *   0 - disabled
 *   1 - enabled (default)
 *
 * /sys/kernel/fast_charge/ac_charge_level (rw)
 *
 *   rate at which to charge when on AC (1.0A/h to 2.1A/h)
 *
 * /sys/kernel/fast_charge/usb_charge_level (r/w)
 *
 *   rate at which to charge when on USB (0.460A/h to 1.0A/h)
 *
 * /sys/kernel/fast_charge/wireless_charge_level (r/w)
 *
 *   rate at which to charge when on WIRELESS (0.650A/h to 1.2A/h)
 *
 * /sys/kernel/fast_charge/failsafe (rw)
 *
 *   0 - disabled - allow anything up to 2.1A/h to be used as AC / USB custom current
 *   1 - enabled  - behaviour as described above (default)
 *
 * /sys/kernel/fast_charge/ac_levels (ro)
 *
 *   display available levels for AC (for failsafe enabled mode)
 *
 * /sys/kernel/fast_charge/usb_levels (ro)
 *
 *   display available levels for USB (for failsafe enabled mode)
 *
 * /sys/kernel/fast_charge/wireless_levels (ro)
 *
 *   display available levels for WIRELESS (for failsafe enabled mode)
 *
 * /sys/kernel/fast_charge/version (ro)
 *
 *   display fast charge version information
 *
 * /sys/kernel/fast_charge/info (ro)
 *
 *   display complete fast charge configuration in human readable format
 *
 */

#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/fastchg.h>

int force_fast_charge;

/* sysfs interface for "force_fast_charge" */
static ssize_t force_fast_charge_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", force_fast_charge);
}

static ssize_t force_fast_charge_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{

	int new_force_fast_charge;

	sscanf(buf, "%du", &new_force_fast_charge);

	switch(new_force_fast_charge) {
		case FAST_CHARGE_DISABLED:
		case FAST_CHARGE_FORCE_AC:
		case FAST_CHARGE_FORCE_CUSTOM_MA:
			force_fast_charge = new_force_fast_charge;
			return count;
		default:
			return -EINVAL;
	}
}

static struct kobj_attribute force_fast_charge_attribute =
	__ATTR(force_fast_charge, 0666,
		force_fast_charge_show,
		force_fast_charge_store);

int use_mtp_during_fast_charge;

/* sysfs interface for "use_mtp_during_fast_charge" */
static ssize_t use_mtp_during_fast_charge_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", use_mtp_during_fast_charge);
}

static ssize_t use_mtp_during_fast_charge_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{

	int new_use_mtp_during_fast_charge;

	sscanf(buf, "%du", &new_use_mtp_during_fast_charge);

	switch(new_use_mtp_during_fast_charge) {
		case USE_MTP_DURING_FAST_CHARGE_DISABLED:
		case USE_MTP_DURING_FAST_CHARGE_ENABLED:
			use_mtp_during_fast_charge = new_use_mtp_during_fast_charge;
			return count;
		default:
			return -EINVAL;
	}
}

static struct kobj_attribute use_mtp_during_fast_charge_attribute =
	__ATTR(use_mtp_during_fast_charge, 0666,
		use_mtp_during_fast_charge_show,
		use_mtp_during_fast_charge_store);

int screen_on_current_limit;

/* sysfs interface for "screen_on_current_limit" */
static ssize_t screen_on_current_limit_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", screen_on_current_limit);
}

static ssize_t screen_on_current_limit_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{

	int new_screen_on_current_limit;

	sscanf(buf, "%du", &new_screen_on_current_limit);

	switch(new_screen_on_current_limit) {
		case SCREEN_ON_CURRENT_LIMIT_DISABLED:
		case SCREEN_ON_CURRENT_LIMIT_ENABLED:
			screen_on_current_limit = new_screen_on_current_limit;
			return count;
		default:
			return -EINVAL;
	}
}

static struct kobj_attribute screen_on_current_limit_attribute =
	__ATTR(screen_on_current_limit, 0666,
		screen_on_current_limit_show,
		screen_on_current_limit_store);


/* sysfs interface for "ac_charge_level" */

int ac_charge_level;

static ssize_t ac_charge_level_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", ac_charge_level);
}

static ssize_t ac_charge_level_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{

	int new_ac_charge_level;

	sscanf(buf, "%du", &new_ac_charge_level);

	if (failsafe == FAIL_SAFE_DISABLED &&
		new_ac_charge_level <= MAX_CHARGE_LEVEL) {

		ac_charge_level = new_ac_charge_level;
		return count;

	}

	else {

		switch (new_ac_charge_level) {
			case AC_CHARGE_1000:
			case AC_CHARGE_1100:
			case AC_CHARGE_1200:
			case AC_CHARGE_1300:
			case AC_CHARGE_1400:
			case AC_CHARGE_1500:
			case AC_CHARGE_1600:
			case AC_CHARGE_1700:
			case AC_CHARGE_1800:
			case AC_CHARGE_1900:
			case AC_CHARGE_2000:
			case AC_CHARGE_2100:
				ac_charge_level = new_ac_charge_level;
				return count;
			default:
				return -EINVAL;

		}

	}

	return -EINVAL;

}

static struct kobj_attribute ac_charge_level_attribute =
	__ATTR(ac_charge_level, 0666,
		ac_charge_level_show,
		ac_charge_level_store);

/* sysfs interface for "usb_charge_level" */

int usb_charge_level;

static ssize_t usb_charge_level_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", usb_charge_level);
}

static ssize_t usb_charge_level_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{

	int new_usb_charge_level;

	sscanf(buf, "%du", &new_usb_charge_level);

	if (failsafe == FAIL_SAFE_DISABLED &&
		new_usb_charge_level <= MAX_CHARGE_LEVEL) {

		usb_charge_level = new_usb_charge_level;
		return count;

	}

	else {

		switch (new_usb_charge_level) {
			case USB_CHARGE_460:
			case USB_CHARGE_500:
			case USB_CHARGE_600:
			case USB_CHARGE_700:
			case USB_CHARGE_800:
			case USB_CHARGE_900:
			case USB_CHARGE_1000:
				usb_charge_level = new_usb_charge_level;
				return count;
			default:
				return -EINVAL;
		}

	}

	return -EINVAL;

}

static struct kobj_attribute usb_charge_level_attribute =
	__ATTR(usb_charge_level, 0666,
		usb_charge_level_show,
		usb_charge_level_store);

/* sysfs interface for "wireless_charge_level" */

int wireless_charge_level;

static ssize_t wireless_charge_level_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", wireless_charge_level);
}

static ssize_t wireless_charge_level_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{

	int new_wireless_charge_level;

	sscanf(buf, "%du", &new_wireless_charge_level);

	if (failsafe == FAIL_SAFE_DISABLED && new_wireless_charge_level <= MAX_CHARGE_LEVEL) {

		wireless_charge_level = new_wireless_charge_level;
		return count;

	}

	else {

		switch (new_wireless_charge_level) {
			case WIRELESS_CHARGE_650:
			case WIRELESS_CHARGE_800:
			case WIRELESS_CHARGE_900:
			case WIRELESS_CHARGE_1000:
			case WIRELESS_CHARGE_1100:
			case WIRELESS_CHARGE_1200:	wireless_charge_level = new_wireless_charge_level;
							return count;
			default:			return -EINVAL;
		}

	}

	return -EINVAL;

}

static struct kobj_attribute wireless_charge_level_attribute =
__ATTR(wireless_charge_level, 0666, wireless_charge_level_show, wireless_charge_level_store);

/* sysfs interface for "failsafe" */

int failsafe;

static ssize_t failsafe_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", failsafe);
}

static ssize_t failsafe_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf, size_t count)
{

	int new_failsafe;

	sscanf(buf, "%du", &new_failsafe);

	switch (new_failsafe) {
		case FAIL_SAFE_ENABLED:
			usb_charge_level = USB_CHARGE_460;
			ac_charge_level = AC_CHARGE_1800;
			failsafe = new_failsafe;
			return count;
		case FAIL_SAFE_DISABLED:
			failsafe = new_failsafe;
			return count;
		default:
			return -EINVAL;
	}
}

static struct kobj_attribute failsafe_attribute =
	__ATTR(failsafe, 0666, failsafe_show, failsafe_store);

/* sysfs interface for "ac_levels" */
static ssize_t ac_levels_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", AC_LEVELS);
}

static struct kobj_attribute ac_levels_attribute =
	__ATTR(ac_levels, 0444, ac_levels_show, NULL);

/* sysfs interface for "usb_levels" */
static ssize_t usb_levels_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", USB_LEVELS);
}

static struct kobj_attribute usb_levels_attribute =
	__ATTR(usb_levels, 0444, usb_levels_show, NULL);

/* sysfs interface for "wireless_levels" */
static ssize_t wireless_levels_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", WIRELESS_LEVELS);
}

static struct kobj_attribute wireless_levels_attribute =
__ATTR(wireless_levels, 0444, wireless_levels_show, NULL);

/* sysfs interface for "info" */
static ssize_t info_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(
		buf,
		"Forced Fast Charge for Samsung Note 3 %s\n\n"
		"Fast charge mode      : %s\n"
		"MTP while charging mode : %s\n"
		"Screen on Current Limit mode : %s\n"
		"Custom  AC level      : %dmA/h\n"
		"Custom USB level      : %dmA/h\n"
		"Custom Wireless level : %dmA/h\n"
		"Failsafe mode         : %s\n"
		"Valid AC  levels      : %s\n"
		"Valid USB levels      : %s\n"
		"Valid Wireless levels : %s\n",
		 FAST_CHARGE_VERSION,
		 force_fast_charge == FAST_CHARGE_DISABLED 	   ? "0 - Disabled (default)" :
		(force_fast_charge == FAST_CHARGE_FORCE_AC         ? "1 - Use stock AC level on USB" :
		(force_fast_charge == FAST_CHARGE_FORCE_CUSTOM_MA  ? "2 - Use custom mA on AC and USB" : "Problem : value out of range")),
		 use_mtp_during_fast_charge          == USE_MTP_DURING_FAST_CHARGE_DISABLED           ? "0 - Disabled" :
		(use_mtp_during_fast_charge          == USE_MTP_DURING_FAST_CHARGE_ENABLED            ? "1 - Enabled" : "Problem : value out of range"),
		 screen_on_current_limit          == SCREEN_ON_CURRENT_LIMIT_DISABLED           ? "0 - Disabled" :
		(screen_on_current_limit          == SCREEN_ON_CURRENT_LIMIT_ENABLED            ? "1 - Enabled" : "Problem : value out of range"),
		 ac_charge_level,
		 usb_charge_level,
		 wireless_charge_level,
		 failsafe          == FAIL_SAFE_DISABLED           ? "0 - Failsafe disabled - please be careful !" :
		(failsafe          == FAIL_SAFE_ENABLED            ? "1 - Failsafe active (default)" : "Problem : value out of range"),
		 failsafe          == FAIL_SAFE_ENABLED            ? AC_LEVELS : ANY_LEVELS,
		 failsafe          == FAIL_SAFE_ENABLED            ? USB_LEVELS : ANY_LEVELS,
		 failsafe          == FAIL_SAFE_ENABLED            ? WIRELESS_LEVELS : ANY_LEVELS
		);
}

static struct kobj_attribute info_attribute =
	__ATTR(info, 0444, info_show, NULL);

/* sysfs interface for "version" */
static ssize_t version_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", FAST_CHARGE_VERSION);
}

static struct kobj_attribute version_attribute =
	__ATTR(version, 0444, version_show, NULL);

/* Initialize fast charge sysfs folder */
static struct kobject *force_fast_charge_kobj;

static struct attribute *force_fast_charge_attrs[] = {
	&force_fast_charge_attribute.attr,
	&use_mtp_during_fast_charge_attribute.attr,
	&screen_on_current_limit_attribute.attr,
	&ac_charge_level_attribute.attr,
	&usb_charge_level_attribute.attr,
	&wireless_charge_level_attribute.attr,
	&failsafe_attribute.attr,
	&ac_levels_attribute.attr,
	&usb_levels_attribute.attr,
	&wireless_levels_attribute.attr,
	&info_attribute.attr,
	&version_attribute.attr,
	NULL,
};

static struct attribute_group force_fast_charge_attr_group = {
.attrs = force_fast_charge_attrs,
};

int force_fast_charge_init(void)
{
	int force_fast_charge_retval;

	/* Forced fast charge disabled by default */
	force_fast_charge = FAST_CHARGE_DISABLED;
	/* Use MTP during fast charge, enabled by default */
	use_mtp_during_fast_charge = USE_MTP_DURING_FAST_CHARGE_ENABLED;
	/* Use Samsung Screen ON current limit while charging, enabled by default */
	screen_on_current_limit = SCREEN_ON_CURRENT_LIMIT_ENABLED;
	/* Default AC charge level to 1800mA/h    */
	ac_charge_level   = AC_CHARGE_1800;
	/* Default USB charge level to 460mA/h    */
	usb_charge_level  = USB_CHARGE_460;
	/* Default USB charge level to 650mA/h    */
	wireless_charge_level = WIRELESS_CHARGE_650;
	/* Allow only values in list by default   */
	failsafe              = FAIL_SAFE_ENABLED;

        force_fast_charge_kobj =
		kobject_create_and_add("fast_charge", kernel_kobj);

        if (!force_fast_charge_kobj)
                return -ENOMEM;

        force_fast_charge_retval =
		sysfs_create_group(force_fast_charge_kobj,
			&force_fast_charge_attr_group);

        if (force_fast_charge_retval)
                kobject_put(force_fast_charge_kobj);

        return (force_fast_charge_retval);
}
/* end sysfs interface */

void force_fast_charge_exit(void)
{
	kobject_put(force_fast_charge_kobj);
}

module_init(force_fast_charge_init);
module_exit(force_fast_charge_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jean-Pierre Rasquin <yank555.lu@gmail.com>");
MODULE_AUTHOR("Paul Reioux <reioux@gmail.com>");
MODULE_DESCRIPTION("Fast Charge Hack for Android");