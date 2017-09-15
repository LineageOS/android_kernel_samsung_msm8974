/*
*  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <net/sock.h>
#include <net/netlink.h>
#include <linux/skbuff.h>
#include <linux/netlink.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/err.h>
#include "adsp.h"
/* The netlink socket */
struct adsp_data *data;
struct class *sensors_factory_class;
static atomic_t sensor_count;
DEFINE_MUTEX(factory_mutex);

unsigned int raw_data_stream;
EXPORT_SYMBOL(raw_data_stream);
struct mutex raw_stream_lock;
EXPORT_SYMBOL(raw_stream_lock);

/* Function used to send message to the user space */
int adsp_unicast(struct msg_data param, int message, int flags, u32 pid)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	struct msg_data *msg;
	int ret = -1;
	skb = nlmsg_new(sizeof(struct msg_data), GFP_KERNEL);
	nlh = nlmsg_put(skb, pid, 0, message, sizeof(struct msg_data), flags);
	if (nlh == NULL){
		nlmsg_free(skb);
		return -EMSGSIZE;
	}
	msg = nlmsg_data(nlh);
	msg->sensor_type = param.sensor_type;
	msg->param1 = param.param1;
	msg->param2 = param.param2;
	msg->param3 = param.param3;
	NETLINK_CB(skb).dst_group = 0;
	ret = nlmsg_unicast(data->adsp_skt, skb, PID);
	return ret;
}


static void set_sensor_attr(struct device *dev,
	struct device_attribute *attributes[])
{
	int i;
	for (i = 0; attributes[i] != NULL; i++)
		if ((device_create_file(dev, attributes[i])) < 0)
			pr_err("[FACTORY] fail device_create_file"\
				"(dev, attributes[%d])\n", i);
}

#ifdef CONFIG_ADSP_FACTORY
extern  struct class* get_adsp_sensor_class(void);
#endif

int adsp_factory_register(char *dev_name, int type,
	struct device_attribute *attributes[], struct adsp_fac_ctl *fact_ctl)
{
	int ret = 0;
	pr_err("[FACTORY] %s\n", __func__);
	data->ctl[type] = fact_ctl;
#ifdef CONFIG_ADSP_FACTORY
	sensors_factory_class = get_adsp_sensor_class();
#endif
	data->sensor_device[type] =
		device_create(sensors_factory_class,
			NULL, 0, data, "%s", dev_name);
	if (IS_ERR(data->sensor_device[type])) {
		ret = PTR_ERR(data->sensor_device[type]);
		pr_err("[FACTORY] %s: device_create failed![%d]\n",
			__func__, ret);
		return ret;
	}
	set_sensor_attr(data->sensor_device[type], attributes);
	atomic_inc(&sensor_count);
	data->ctl[type]->receive_data_fnc(data, CALLBACK_REGISTER_SUCCESS);
	return ret;
}

int adsp_factory_start_timer(const unsigned int ms)
{
	return mod_timer(&data->command_timer, jiffies + msecs_to_jiffies(ms));
}

static int process_received_msg(struct sk_buff *skb, struct nlmsghdr *nlh)
{
	switch (nlh->nlmsg_type) {
	case NETLINK_MESSAGE_RAW_DATA_RCVD:
	{
		struct sensor_value *pdata =
			(struct sensor_value*)NLMSG_DATA(nlh);
		data->sensor_data[pdata->sensor_type] = *pdata;
		if (raw_data_stream & 1 << pdata->sensor_type)
			data->data_ready_flag |= 1 << pdata->sensor_type;
		break;
	}
	case NETLINK_MESSAGE_REACTIVE_ALERT_RCVD:
	{
		struct sensor_value *pdata =
			(struct sensor_value*)NLMSG_DATA(nlh);
		pr_info("NETLINK_MESSAGE_REACTIVE_ALERT_RCVD type=%d,motion=%d\n",
			pdata->sensor_type, pdata->reactive_alert);
		data->reactive_alert = pdata->reactive_alert;
		if (data->reactive_alert != 4 && data->reactive_alert != 1)
			data->alert_ready_flag |= 1 << pdata->sensor_type;
		break;
	}
	case NETLINK_MESSAGE_CALIB_DATA_RCVD:
	{
		struct sensor_calib_value *pdata =
			(struct sensor_calib_value*)NLMSG_DATA(nlh);
		pr_info("NETLINK_MESSAGE_CALIB_DATA_RCVD type=%d,x=%d,y=%d,z=%d\n",
			pdata->sensor_type, pdata->x, pdata->y, pdata->z);
		data->sensor_calib_data[pdata->sensor_type] = *pdata;
		data->calib_ready_flag |= 1 << pdata->sensor_type;
		break;
	}
	case NETLINK_MESSAGE_CALIB_STORE_RCVD:
	{
		struct sensor_calib_store_result *pdata =
			(struct sensor_calib_store_result*)NLMSG_DATA(nlh);
		pr_info("NETLINK_MESSAGE_CALIB_STORE_RCVD type=%d,nCalibstoreresult=%d\n",
			pdata->sensor_type, pdata->nCalibstoreresult);
		data->sensor_calib_result[pdata->sensor_type] = *pdata;
			data->calib_store_ready_flag |= 1 << pdata->sensor_type;
		break;
	}
	case NETLINK_MESSAGE_SELFTEST_SHOW_RCVD:
	{
		struct sensor_selftest_show_result *pdata =
			(struct sensor_selftest_show_result*)NLMSG_DATA(nlh);
		pr_info("NETLINK_MESSAGE_SELFTEST_SHOW_RCVD type=%d, SelftestResult1=%d, SelftestResult2=%d\n",
			pdata->sensor_type, pdata->nSelftestresult1, pdata->nSelftestresult2);
		data->sensor_selftest_result[pdata->sensor_type] = *pdata;
		data->selftest_ready_flag |= 1 << pdata->sensor_type;
		break;
	}
	case NETLINK_MESSAGE_STOP_RAW_DATA:
	{
		struct sensor_stop_value *pdata =
			(struct sensor_stop_value*)NLMSG_DATA(nlh);
		pr_info("NETLINK_MESSAGE_STOP_RAW_DATA type=%d, StopResult=%d\n",
			pdata->sensor_type, pdata->result);
		data->sensor_stop_data[pdata->sensor_type] = *pdata;
		data->data_ready_flag &= 0 << pdata->sensor_type;
		break;
	}
	case NETLINK_MESSAGE_GYRO_TEMP_RCVD:
	{
		struct sensor_value  *pdata =
			(struct sensor_value*)NLMSG_DATA(nlh);
		pr_info("NETLINK_MESSAGE_RAW_DATA_RCVD type=%d,x=%d,y=%d,z=%d\n",
			pdata->sensor_type, pdata->x, pdata->y, pdata->z);
		data->gyro_temp = pdata->temperature;
		data->data_ready |= ADSP_DATA_GYRO_TEMP;
		break;
	}
	default:
		break;
	}
	return 0;
}

static void factory_receive_skb(struct sk_buff *skb)
{
	struct nlmsghdr *nlh;
	int len;
	int err;
	nlh = (struct nlmsghdr*)skb->data;
	len = skb->len;
	while (NLMSG_OK(nlh, len)) {
		err = process_received_msg(skb, nlh);
		/* if err or if this message says it wants a response */
		if (err || (nlh->nlmsg_flags & NLM_F_ACK))
			netlink_ack(skb, nlh, err);
		nlh = NLMSG_NEXT(nlh, len);
	}
}

static void factory_adsp_command_timer(unsigned long value)
{
	int i;
	for (i = 0; i < ADSP_FACTORY_SENSOR_MAX; i++) {
		if (data->ctl[i])
			data->ctl[i]->receive_data_fnc(data,
				CALLBACK_TIMER_EXPIRED);
	}
}

/* Receive messages from netlink socket. */
static void factory_test_result_receive(struct sk_buff *skb)
{
	mutex_lock(&factory_mutex);
	factory_receive_skb(skb);
	mutex_unlock(&factory_mutex);
}

static int __devinit factory_adsp_init(void)
{
	pr_info("[FACTORY] %s\n", __func__);
	data = kzalloc(sizeof(*data), GFP_KERNEL);
	data->adsp_skt = netlink_kernel_create(&init_net, NETLINK_ADSP_FAC, 0,
		factory_test_result_receive, NULL, THIS_MODULE);
	data->data_ready_flag = 0;
	data->calib_ready_flag = 0;
	data->calib_store_ready_flag = 0;
	data->selftest_ready_flag = 0;
	init_timer(&data->command_timer);
	data->command_timer.function = factory_adsp_command_timer;
	data->command_timer.data = (unsigned int)NULL;
	data->command_timer.expires = 0xffffffffL;
	add_timer(&data->command_timer);
	mutex_init(&raw_stream_lock);
	pr_err("[FACTORY] %s: Timer Init \n", __func__);
	return 0;
}
module_init(factory_adsp_init);

static void __devexit factory_adsp_exit(void)
{
	del_timer(&data->command_timer);
	mutex_destroy(&raw_stream_lock);
	pr_info("[FACTORY] %s\n", __func__);
}
module_exit(factory_adsp_exit);
MODULE_DESCRIPTION("Support for factory test sensors (adsp)");
MODULE_LICENSE("GPL");





