/* linux/drivers/misc/modem_if_v2/sipc4_modem.c
 *
 * Copyright (C) 2012 Samsung Electronics.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>

#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/wakelock.h>

#include <linux/platform_data/modem.h>
//#include <linux/platform_data/modem_v2.h>
#include "modem_prj.h"
#include "modem_variation.h"
#include "modem_utils.h"


/* If iod->id is 0, do not need to store to `iodevs_tree_fmt' in SIPC4 */
#define sipc4_is_not_reserved_channel(ch) ((ch) != 0)


static struct modem_shared *create_modem_shared_data(void)
{
	struct modem_shared *msd;

	msd = kzalloc(sizeof(struct modem_shared), GFP_KERNEL);
	if (!msd)
		return NULL;

	/* initialize link device list */
	INIT_LIST_HEAD(&msd->link_dev_list);

	/* initialize tree of io devices */
	msd->iodevs_tree_chan = RB_ROOT;
	msd->iodevs_tree_fmt = RB_ROOT;

	return msd;
}

static struct modem_ctl *create_modemctl_device(struct platform_device *pdev,
				struct modem_shared *msd)
{
	int ret = 0;
	struct modem_data *pdata;
	struct modem_ctl *modemctl;
	struct device *dev = &pdev->dev;

	/* create modem control device */
	modemctl = kzalloc(sizeof(struct modem_ctl), GFP_KERNEL);
	if (!modemctl)
		return NULL;

	modemctl->msd = msd;
	modemctl->dev = dev;
	modemctl->phone_state = STATE_OFFLINE;

	pdata = pdev->dev.platform_data;
	modemctl->mdm_data = pdata;
	modemctl->name = pdata->name;

	/* init modemctl device for getting modemctl operations */
	ret = call_modem_init_func(modemctl, pdata);
	if (ret) {
		kfree(modemctl);
		return NULL;
	}

	mif_info("%s: create_modemctl_device DONE\n", modemctl->name);
	return modemctl;
}

static struct io_device *create_io_device(struct modem_io_t *io_t,
			struct modem_shared *msd, struct modem_ctl *modemctl,
			struct modem_data *pdata)
{
	int ret = 0;
	struct io_device *iod = NULL;

	iod = kzalloc(sizeof(struct io_device), GFP_KERNEL);
	if (!iod) {
		mif_err("io device memory alloc fail\n");
		return NULL;
	}

	rb_init_node(&iod->node_chan);
	rb_init_node(&iod->node_fmt);

	iod->name = io_t->name;
	iod->id = io_t->id;
	iod->format = io_t->format;
	iod->io_typ = io_t->io_type;
	iod->link_types = io_t->links;
	iod->net_typ = pdata->modem_net;
	iod->ipc_version = pdata->ipc_version;
	iod->use_handover = pdata->use_handover;
	atomic_set(&iod->opened, 0);

	/* link between io device and modem control */
	iod->mc = modemctl;
	if (iod->format == IPC_FMT)
		modemctl->iod = iod;
	else if (iod->format == IPC_BOOT)
		modemctl->bootd = iod;

	/* link between io device and modem shared */
	iod->msd = msd;

	/* add iod to rb_tree */
	if (iod->format != IPC_RAW)
		insert_iod_with_format(msd, iod->format, iod);

	if (sipc4_is_not_reserved_channel(iod->id))
		insert_iod_with_channel(msd, iod->id, iod);

	/* register misc device or net device */
	ret = sipc4_init_io_device(iod);
	if (ret) {
		kfree(iod);
		mif_err("sipc4_init_io_device fail (%d)\n", ret);
		return NULL;
	}

	mif_debug("%s: create_io_device DONE\n", io_t->name);
	return iod;
}

static int attach_devices(struct io_device *iod, enum modem_link tx_link)
{
	struct modem_shared *msd = iod->msd;
	struct link_device *ld;

	/* find link type for this io device */
	list_for_each_entry(ld, &msd->link_dev_list, list) {
		if (IS_CONNECTED(iod, ld)) {
			/* The count 1 bits of iod->link_types is count
			 * of link devices of this iod.
			 * If use one link device,
			 * or, 2+ link devices and this link is tx_link,
			 * set iod's link device with ld
			 */
			if ((countbits(iod->link_types) <= 1) ||
					(tx_link == ld->link_type)) {
				//mif_info("set %s->%s\n", iod->name, ld->name);
				set_current_link(iod, ld);

				if (iod->ipc_version == SIPC_VER_42) {
					if (iod->format == IPC_FMT) {
						int ch = iod->id & 0x03;
						mif_info("set %s->%s, ch[%d]\n", iod->name, ld->name, ch);
						ld->fmt_iods[ch] = iod;
					}
				}
			}
		}
	}

	/* if use rx dynamic switch, set tx_link at modem_io_t of
	 * board-*-modems.c
	 */
	if (!get_current_link(iod)) {
		mif_err("%s->link == NULL\n", iod->name);
		BUG();
	}

	switch (iod->format) {
	case IPC_FMT:
		wake_lock_init(&iod->wakelock, WAKE_LOCK_SUSPEND, iod->name);
		iod->waketime = FMT_WAKE_TIME;
		break;

	case IPC_RFS:
		wake_lock_init(&iod->wakelock, WAKE_LOCK_SUSPEND, iod->name);
		iod->waketime = RFS_WAKE_TIME;
		break;

	case IPC_MULTI_RAW:
		wake_lock_init(&iod->wakelock, WAKE_LOCK_SUSPEND, iod->name);
		iod->waketime = RAW_WAKE_TIME;
		break;
	default:
		break;
	}

	return 0;
}

static int __devinit modem_probe(struct platform_device *pdev)
{
	int i;
	struct modem_data *pdata = pdev->dev.platform_data;
	struct modem_shared *msd = NULL;
	struct modem_ctl *modemctl = NULL;
	struct io_device *iod[pdata->num_iodevs];
	struct link_device *ld;

	memset(iod, 0, sizeof(iod));

	msd = create_modem_shared_data();
	if (!msd) {
		mif_err("msd == NULL\n");
		goto err_free_modemctl;
	}

	modemctl = create_modemctl_device(pdev, msd);
	if (!modemctl) {
		mif_err("modemctl == NULL\n");
		goto err_free_modemctl;
	}

	/* create link device */
	/* support multi-link device */

	mif_err("call_link_init_func link type val [%d]\n", pdata->link_types);
	
	for (i = 0; i < LINKDEV_MAX ; i++) {
		/* find matching link type */
		if (pdata->link_types & LINKTYPE(i)) {

			mif_err("call_link_init_func  [%d]\n", i);
			
			ld = call_link_init_func(pdev, i);
			if (!ld) {
				mif_err("call_link_init_func errori [%d]\n", i);
				goto err_free_modemctl;
			}

			mif_err("link created : %s\n", ld->name);

			ld->link_type = i;
			ld->mc = modemctl;
			ld->msd = msd;
			list_add(&ld->list, &msd->link_dev_list);
		}
	}

	/* create io deivces and connect to modemctl device */
	for (i = 0; i < pdata->num_iodevs; i++) {
		iod[i] = create_io_device(&pdata->iodevs[i], msd, modemctl,
				pdata);
		if (!iod[i]) {
			mif_err("iod[%d] == NULL\n", i);
			goto err_free_modemctl;
		}

		attach_devices(iod[i], pdata->iodevs[i].tx_link);
	}

	platform_set_drvdata(pdev, modemctl);

	mif_info("modem_probe Done\n");
	return 0;

err_free_modemctl:
	for (i = 0; i < pdata->num_iodevs; i++)
		if (iod[i] != NULL)
			kfree(iod[i]);

	if (modemctl != NULL)
		kfree(modemctl);

	if (msd != NULL)
		kfree(msd);

	return -ENOMEM;
}

static void modem_shutdown(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct modem_ctl *mc = dev_get_drvdata(dev);

	if (mc->ops.modem_off)
		mc->ops.modem_off(mc);
	mc->phone_state = STATE_OFFLINE;
}

static int modem_suspend(struct device *pdev)
{
	struct modem_ctl *mc = dev_get_drvdata(pdev);

	if(mc->phone_state==STATE_OFFLINE)
		return 0;
	
	if (mc->gpio_pda_active)
		gpio_set_value(mc->gpio_pda_active, 0);

	return 0;
}

static int modem_resume(struct device *pdev)
{
	struct modem_ctl *mc = dev_get_drvdata(pdev);

	if(mc->phone_state==STATE_OFFLINE)
		return 0;

	if (mc->gpio_pda_active)
		gpio_set_value(mc->gpio_pda_active, 1);

	return 0;
}

static const struct dev_pm_ops modem_pm_ops = {
	.suspend    = modem_suspend,
	.resume     = modem_resume,
};

#ifdef CONFIG_OF
static const struct of_device_id gsm_modem_dt_match[] = {
	{.compatible = "samsung,sc6500_modem"},
	{}
};
MODULE_DEVICE_TABLE(of, gsm_modem_dt_match);
#endif

static struct platform_driver modem_driver = {
	.probe = modem_probe,
	.shutdown = modem_shutdown,
	.driver = {
		.name = "mif_sipc4",
		.pm   = &modem_pm_ops,
#ifdef CONFIG_OF
		.of_match_table = gsm_modem_dt_match,
#endif
	},
};

static int __init modem_init(void)
{
	return platform_driver_register(&modem_driver);
}

module_init(modem_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Samsung Modem Interface Driver");
