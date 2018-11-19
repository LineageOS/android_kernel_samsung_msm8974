/*
 *  Generic Bluetooth HID support
 *
 *  Based upon hid-generic.c from Henrik Rydberg
 *  found in (Android) kernel 3.10+
 *
 *  Copyright (c) 1999 Andreas Gal
 *  Copyright (c) 2000-2005 Vojtech Pavlik <vojtech@suse.cz>
 *  Copyright (c) 2005 Michael Haboustak <mike-@cinci.rr.com> for Concept2, Inc
 *  Copyright (c) 2007-2008 Oliver Neukum
 *  Copyright (c) 2006-2012 Jiri Kosina
 *  Copyright (c) 2012 Henrik Rydberg
 *  Copyright (c) 2018 Thomas Jarosch
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <asm/unaligned.h>
#include <asm/byteorder.h>

#include <linux/hid.h>

static const struct hid_device_id hid_table[] = {
	{ HID_BLUETOOTH_DEVICE(HID_ANY_ID, HID_ANY_ID) },
	{ }
};
MODULE_DEVICE_TABLE(hid, hid_table);

static struct hid_driver hid_generic_bt = {
	/* TJ: Start name with "generic-" so specialized drivers are allowed to
	   override us. See hid-core.c for the "generic-" match logic */
	.name = "generic-bt",
	.id_table = hid_table,
};

static int __init hid_init(void)
{
	return hid_register_driver(&hid_generic_bt);
}

static void __exit hid_exit(void)
{
	hid_unregister_driver(&hid_generic_bt);
}

module_init(hid_init);
module_exit(hid_exit);

MODULE_AUTHOR("Thomas Jarosch");
MODULE_DESCRIPTION("HID generic Bluetooth driver");
MODULE_LICENSE("GPL");
