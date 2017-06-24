/* Copyright (c) 2009-2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/ctype.h>
#include <linux/miscdevice.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fb.h>
#include <linux/msm_mdp.h>
#include <linux/ioctl.h>
#include <linux/lcd.h>

#include "mdss_dsi.h"
#include "cabc_tuning.h"
#include "cabc_tuning_data_mondrian.h"

static char cabc_tune_data1[CABC_TUNE_FIRST_SIZE] = {0,};
static char cabc_tune_data2[CABC_TUNE_SECOND_SIZE] = {0,};
static char cabc_tune_data3[CABC_TUNE_THIRD_SIZE] = {0,};
static char cabc_tune_data4[CABC_TUNE_FOURTH_SIZE] = {0,};
static char cabc_select_data[CABC_TUNE_SELECT_SIZE] = {0,};

static char tuning_file[128];
static char cabc_tuning[200];

static struct class *cabc_class;
static struct device *tune_cabc_dev;
static struct class *mdnie_class; /* Using for CABC Key String */
static struct device *tune_mdnie_dev; /* Using for CABC Key String */


static struct mdss_dsi_ctrl_pdata *cabc_master_dsi_ctrl;
static struct mdss_dsi_ctrl_pdata *cabc_slave_dsi_ctrl;


static struct dsi_cmd_desc cabc_tune_cmd[] = {
	{{DTYPE_GEN_LWRITE, 1, 0, 0, 0,
		sizeof(cabc_tune_data1)}, cabc_tune_data1},
	{{DTYPE_GEN_LWRITE, 1, 0, 0, 0,
		sizeof(cabc_tune_data2)}, cabc_tune_data2},
	{{DTYPE_GEN_LWRITE, 1, 0, 0, 0,
		sizeof(cabc_tune_data3)}, cabc_tune_data3},
	{{DTYPE_GEN_LWRITE, 1, 0, 0, 0,
		sizeof(cabc_tune_data4)}, cabc_tune_data4},
	{{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(cabc_select_data)}, cabc_select_data},
};

static struct cabc_tun_type cabc_tun_state = {
	.cabc_enable = 0,
	.luxvalue = CABC_LUX_2,
	.auto_br = CABC_AUTO_BR_OFF,
	.mode = CABC_MODE_UI,
	.negative = CABC_NEGATIVE_OFF,
};

static void mdss_dsi_panel_cmds_send(struct mdss_dsi_ctrl_pdata *ctrl,
						struct dsi_panel_cmds *pcmds)
{
	struct dcs_cmd_req cmdreq;

	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds = pcmds->cmds;
	cmdreq.cmds_cnt = pcmds->cmd_cnt;
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mdss_dsi_cmdlist_put(ctrl, &cmdreq);
}


static void mdss_dsi_cmds_send(struct mdss_dsi_ctrl_pdata *ctrl,
				struct dsi_cmd_desc *cmds, int cnt,int flag)
{
	struct dcs_cmd_req cmdreq;

	memset(&cmdreq, 0, sizeof(cmdreq));

	if (flag & CMD_REQ_SINGLE_TX) {
		cmdreq.flags = CMD_REQ_SINGLE_TX | CMD_CLK_CTRL | CMD_REQ_COMMIT;
	}else
		cmdreq.flags = CMD_REQ_COMMIT | CMD_CLK_CTRL;

	cmdreq.cmds = cmds;
	cmdreq.cmds_cnt = cnt;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mdss_dsi_cmdlist_put(ctrl, &cmdreq);
}
//#define CABC_TUN_DATA_DEBUG
#ifdef CABC_TUN_DATA_DEBUG
static void print_tun_data(void)
{
	int i;

	DPRINT("\n");
	DPRINT("---- size1 : %d", PAYLOAD1.dchdr.dlen);
	for (i = 0; i < CABC_TUNE_FIRST_SIZE ; i++)
		DPRINT("0x%x ", PAYLOAD1.payload[i]);
	DPRINT("\n");
	DPRINT("---- size2 : %d", PAYLOAD2.dchdr.dlen);
	for (i = 0; i < CABC_TUNE_SECOND_SIZE ; i++)
		DPRINT("0x%x ", PAYLOAD2.payload[i]);
	DPRINT("\n");
	DPRINT("---- size3 : %d", PAYLOAD3.dchdr.dlen);
	for (i = 0; i < CABC_TUNE_THIRD_SIZE ; i++)
		DPRINT("0x%x ", PAYLOAD3.payload[i]);
	DPRINT("\n");
	DPRINT("---- size4 : %d", PAYLOAD4.dchdr.dlen);
	for (i = 0; i < CABC_TUNE_FOURTH_SIZE ; i++)
		DPRINT("0x%x ", PAYLOAD4.payload[i]);
	DPRINT("\n");
	DPRINT("---- size5 : %d", SELECT.dchdr.dlen);
	for (i = 0; i < CABC_TUNE_SELECT_SIZE ; i++)
		DPRINT("0x%x ", SELECT.payload[i]);
	DPRINT("\n");
}
#endif

static void free_tun_cmd(void)
{
	memset(cabc_tune_data1, 0, CABC_TUNE_FIRST_SIZE);
	memset(cabc_tune_data2, 0, CABC_TUNE_SECOND_SIZE);
	memset(cabc_tune_data3, 0, CABC_TUNE_THIRD_SIZE);
	memset(cabc_select_data, 0, CABC_TUNE_SELECT_SIZE);
}


void sending_tuning_cmd(void)
{

	mutex_lock(&cabc_tun_state.cabc_mutex);

#ifdef CABC_TUN_DATA_DEBUG
	print_tun_data();
#else
	DPRINT("Send CABC tuning cmd!!\n");
#endif

	mdss_dsi_cmds_send(cabc_slave_dsi_ctrl, cabc_tune_cmd,
					ARRAY_SIZE(cabc_tune_cmd),0);
	mdss_dsi_cmds_send(cabc_master_dsi_ctrl, cabc_tune_cmd,
					ARRAY_SIZE(cabc_tune_cmd),0);

	mutex_unlock(&cabc_tun_state.cabc_mutex);
}

void CABC_Set_Mode(void)
{
	if (!cabc_tun_state.cabc_enable) {
		DPRINT("[ERROR] CABC engine is OFF.\n");
		return;
	}

	if (!get_panel_power_state()) {
		pr_info("%s : get_panel_power_state off", __func__);
		return;
	}

	DPRINT("CABC_Set_Mode start , mode(%d), negative(%d), lux(%d)\n",
		cabc_tun_state.mode, cabc_tun_state.negative,
						cabc_tun_state.luxvalue);

	switch (cabc_tun_state.mode) {
	case CABC_MODE_UI:
		DPRINT(" = UI MODE =\n");
		INPUT_PAYLOAD1(CABC_NORMAL_1);
		INPUT_PAYLOAD2(CABC_NORMAL_2);
		INPUT_PAYLOAD3(CABC_NORMAL_3);
		break;

	case CABC_MODE_VIDEO:
		DPRINT(" = VIDEO MODE =\n");
		INPUT_PAYLOAD1(CABC_NORMAL_1);
		INPUT_PAYLOAD2(CABC_NORMAL_2);
		INPUT_PAYLOAD3(CABC_NORMAL_3);
		break;
	default:
		DPRINT("[%s] no option for mode (%d)\n", __func__,
							cabc_tun_state.mode);
		return;
	}

	switch (cabc_tun_state.negative) {
	case CABC_NEGATIVE_OFF:
		DPRINT(" = Negative Disabled =\n");
		INPUT_PAYLOAD4(CABC_NORMAL_4);
		break;
	case CABC_NEGATIVE_ON:
		DPRINT(" = Negative Enabled =\n");
		INPUT_PAYLOAD4(CABC_NEGATIVE_4);
		break;
	default:
		DPRINT("[%s] no option for Negative (%d)\n", __func__,
						cabc_tun_state.negative);
		return;
	}

	if(cabc_tun_state.auto_br) {
		DPRINT(" = Auto Br Enabled =\n");
		switch (cabc_tun_state.luxvalue) {
		case CABC_LUX_0:
			DPRINT(" = LUX 0 ~ 150 =\n");
			INPUT_SELECT(CABC_SELECT_2);
			break;
		case CABC_LUX_1:
			DPRINT(" = LUX 150 ~ 5000 =\n");
			if(cabc_tun_state.mode == CABC_MODE_VIDEO)
				INPUT_SELECT(CABC_SELECT_2);
			else
				INPUT_SELECT(CABC_SELECT_1);
			break;
		case CABC_LUX_2:
			DPRINT(" = LUX 5000 ~ =\n");
			INPUT_SELECT(CABC_SELECT_0);
			break;
		default:
			DPRINT("[%s] no option (%d)\n", __func__,
							cabc_tun_state.mode);
			INPUT_SELECT(CABC_SELECT_0);
			return;
		}
	} else {
		DPRINT(" = Auto Br Disabled =\n");
		INPUT_SELECT(CABC_SELECT_0);
	}

	sending_tuning_cmd();
	free_tun_cmd();

	DPRINT("CABC_Set_Mode end , mode(%d), negative(%d), lux(%d)\n",
		cabc_tun_state.mode, cabc_tun_state.negative,
						cabc_tun_state.luxvalue);

}

static ssize_t show_auto_br(struct device *dev,
			     struct device_attribute *dev_attr, char *buf)
{
	return sprintf(buf, "%d\n", cabc_tun_state.auto_br);
}

static ssize_t store_auto_br(struct device *dev,
			    struct device_attribute *dev_attr,
			    const char *buf, size_t count)
{
	int ret;
	unsigned int value;

	ret = kstrtouint(buf, 10, &value);

	if (ret)
		return ret;

	if (value >= CABC_AUTO_BR_MAX) {
		pr_err("Undefied CABC auto br value : %d\n", value);
		return count;
	}

	if (value != cabc_tun_state.auto_br) {
		cabc_tun_state.auto_br = value;
		CABC_Set_Mode();
	}

	return count;
}
static DEVICE_ATTR(auto_br, 0664, show_auto_br, store_auto_br);

static unsigned int lux_to_value(unsigned int input_lux)
{
	if(input_lux <= 150)
		return 0;
	else if (input_lux <= 5000)
		return 1;
	else
		return 2;
}

void update_lux(unsigned int input_lux)
{
	unsigned int value;

	value = lux_to_value(input_lux);

	pr_info("%s : Input Lux=%d Lux Value=%d\n", __func__, input_lux, value);

	if (value >= CABC_LUX_MAX) {
		pr_err("Undefied  CABC lux value : %d\n\n", value);
		return;
	}
	if (value != cabc_tun_state.luxvalue) {
		cabc_tun_state.luxvalue = value;
		CABC_Set_Mode();
	}
}

static ssize_t show_mode(struct device *dev,
			     struct device_attribute *dev_attr, char *buf)
{
	return 0;

}

static ssize_t store_mode(struct device *dev,
			    struct device_attribute *dev_attr,
			    const char *buf, size_t count)
{
	int ret;
	unsigned int value;

	ret = kstrtouint(buf, 10, &value);

	if (ret)
		return ret;

	if (value >= CABC_MODE_MAX) {
		pr_err("Undefied CABC MODE value : %d\n\n", value);
		return count;
	}
	if (value != cabc_tun_state.mode) {
		cabc_tun_state.mode = value;
		CABC_Set_Mode();
	}

	return count;
}

static DEVICE_ATTR(mode, 0664, show_mode, store_mode);

static ssize_t store_lux(struct device *dev,
			    struct device_attribute *dev_attr,
			    const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(lux, 0664, NULL, store_lux);


static ssize_t accessibility_show(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	DPRINT("%s %s\n", __func__, cabc_tun_state.negative ?
					"NEGATIVE" : "ACCESSIBILITY_OFF");
	return snprintf(buf, 256, "%s %s\n", __func__, cabc_tun_state.negative ?
		 			"NEGATIVE" : "ACCESSIBILITY_OFF");
}

static ssize_t accessibility_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	int cmd_value;

	sscanf(buf, "%d", &cmd_value);

	switch (cmd_value) {
	case ACCESSIBILITY_OFF :
		cabc_tun_state.negative = CABC_NEGATIVE_OFF;
		break;
	case NEGATIVE :
		cabc_tun_state.negative = CABC_NEGATIVE_ON;
		break;
	default :
		pr_info("%s Undefined Command (%d)", __func__, cmd_value);
		return size;
	}

	pr_info("%s cmd_value : %d size : %d", __func__, cmd_value, size);

	CABC_Set_Mode();
	return size;
}
static DEVICE_ATTR(accessibility, 0664, accessibility_show,
		   accessibility_store);


/* Using for CABC Key String */
static ssize_t show_cabc(struct device *dev,
			     struct device_attribute *dev_attr, char *buf)
{
	return 0;

}
/* Using for CABC Key String */
static ssize_t store_cabc(struct device *dev,
			    struct device_attribute *dev_attr,
			    const char *buf, size_t count)
{
	int ret;
	unsigned int value;

	ret = kstrtouint(buf, 10, &value);

	if (ret)
		return ret;

	if (!get_panel_power_state()) {
		pr_info("%s : Panel is off state", __func__);
		return count;
	}

	if (value == CABC_OFF) {
		cabc_tun_state.auto_br = CABC_AUTO_BR_OFF;
	} else if (value == CABC_ON) {
		cabc_tun_state.auto_br = CABC_AUTO_BR_ON;
		cabc_tun_state.luxvalue = CABC_LUX_1;
	} else {
		pr_err("Undefied CABC On/Off value : %d\n\n", value);
		return count;
	}

	CABC_Set_Mode();

	return count;
}

static DEVICE_ATTR(cabc, 0664, show_cabc, store_cabc);


static char char_to_dec(char data1, char data2)
{
	char dec;

	dec = 0;

	if (data1 >= 'a') {
		data1 -= 'a';
		data1 += 10;
	} else if (data1 >= 'A') {
		data1 -= 'A';
		data1 += 10;
	} else
		data1 -= '0';
	dec = data1 << 4;

	if (data2 >= 'a') {
		data2 -= 'a';
		data2 += 10;
	} else if (data2 >= 'A') {
		data2 -= 'A';
		data2 += 10;
	} else
		data2 -= '0';

	dec |= data2;

	return dec;
}

static void sending_tune_cmd(char *src, int len)
{
	int data_pos;
	int cmd_step = 0;
	int cmd_pos = 0;

	if (!get_panel_power_state()) {
		pr_info("%s : Panel is off state", __func__);
		return;
	}

	pr_info(" %s : len = %d\n", __func__, len);
	for (data_pos = 0; data_pos < len;) {
		if (*(src + data_pos) == '0') {
			if (*(src + data_pos + 1) == 'x') {
				if (!cmd_step) {
					cabc_tuning[cmd_pos] =
					char_to_dec(*(src + data_pos + 2),
							*(src + data_pos + 3));
				}
				data_pos += 3;
				cmd_pos++;
			} else
				data_pos++;
		} else {
			data_pos++;
		}
	}

	pr_info(" =================== START ==================\n");
	for (data_pos = 0; data_pos <cmd_pos; data_pos++)
		printk(KERN_INFO "0x%x ", cabc_tuning[data_pos]);
	pr_info(" =================== END ==================\n");
}

static void load_tuning_file(char *filename)
{
	struct file *filp;
	char *dp;
	long l;
	loff_t pos;
	int ret;
	mm_segment_t fs;

	pr_info("%s called loading file name : [%s]\n", __func__,
	       filename);

	fs = get_fs();
	set_fs(get_ds());

	filp = filp_open(filename, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		printk(KERN_ERR "%s File open failed\n", __func__);
		goto err;
	}

	l = filp->f_path.dentry->d_inode->i_size;
	pr_info("%s Loading File Size : %ld(bytes)", __func__, l);

	dp = kmalloc(l + 10, GFP_KERNEL);
	if (dp == NULL) {
		pr_info("Can't not alloc memory for tuning file load\n");
		filp_close(filp, current->files);
		goto err;
	}
	pos = 0;
	memset(dp, 0, l);

	pr_info("%s before vfs_read()\n", __func__);
	ret = vfs_read(filp, (char __user *)dp, l, &pos);
	pr_info("%s after vfs_read()\n", __func__);

	if (ret != l) {
		pr_info("vfs_read() filed ret : %d\n", ret);
		kfree(dp);
		filp_close(filp, current->files);
		goto err;
	}

	filp_close(filp, current->files);

	set_fs(fs);

	sending_tune_cmd(dp, l);

	kfree(dp);

	return;
err:
	set_fs(fs);
}

static ssize_t ce_tuning_show(struct device *dev,
			struct device_attribute *attr, char *buf) {
	int ret = 0;
	ret = snprintf(buf, 128, "tuning name : %s\n", tuning_file);
	return ret;
}
static ssize_t ce_tuning_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size) {
	char *pt;
	int a;

	if (!get_panel_power_state()) {
		pr_info("%s : Panel is off state", __func__);
		return size;
	}

	if (sysfs_streq(buf, "1")) {
		pr_info(" %s : ce enable\n", __func__);
		mdss_dsi_panel_cmds_send(cabc_slave_dsi_ctrl, &cabc_slave_dsi_ctrl->ce_on_cmds);
		mdss_dsi_panel_cmds_send(cabc_master_dsi_ctrl, &cabc_master_dsi_ctrl->ce_on_cmds);
		return size;
	} else if (sysfs_streq(buf, "0")) {
		pr_info(" %s : ce disable\n", __func__);
		mdss_dsi_panel_cmds_send(cabc_slave_dsi_ctrl, &cabc_slave_dsi_ctrl->ce_off_cmds);
		mdss_dsi_panel_cmds_send(cabc_master_dsi_ctrl, &cabc_master_dsi_ctrl->ce_off_cmds);
		return size;
	}
	/* echo "tuning file" */
	memset(tuning_file, 0, sizeof(tuning_file));
	snprintf(tuning_file, MAX_FILE_NAME, "%s%s", TUNING_FILE_PATH, buf);

	pt = tuning_file;
	while (*pt) {
		if (*pt == '\r' || *pt == '\n') {
			*pt = 0;
			break;
		}
		pt++;
	}

	pr_info("%s:%s\n", __func__, tuning_file);

	load_tuning_file(tuning_file);

	for (a = 0; a < 33; a++) {
		printk(KERN_INFO "0x%x = 0x%x ",
			cabc_master_dsi_ctrl->ce_on_cmds.cmds[0].payload[a], cabc_tuning[a]);
		cabc_master_dsi_ctrl->ce_on_cmds.cmds[0].payload[a] = cabc_tuning[a];
	}

	mdss_dsi_panel_cmds_send(cabc_slave_dsi_ctrl, &cabc_slave_dsi_ctrl->ce_on_cmds);
	mdss_dsi_panel_cmds_send(cabc_master_dsi_ctrl, &cabc_master_dsi_ctrl->ce_on_cmds);

	return size;
}
static DEVICE_ATTR(cetuning, 0664, ce_tuning_show, ce_tuning_store);

static ssize_t cabc_tuning_show(struct device *dev,
			struct device_attribute *attr, char *buf) {
	int ret = 0;
	ret = snprintf(buf, 128, "tuning name : %s\n", tuning_file);
	return ret;
}
static ssize_t cabc_tuning_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size) {
	char *pt;
	int a, b;

	if (!get_panel_power_state()) {
		pr_info("%s : Panel is off state", __func__);
		return size;
	}

	if (sysfs_streq(buf, "1")) {
		cabc_master_dsi_ctrl->cabc_on_cmds.cmds[0].payload[1] = 0x01;
		mdss_dsi_panel_cmds_send(cabc_slave_dsi_ctrl, &cabc_slave_dsi_ctrl->cabc_on_cmds);
		mdss_dsi_panel_cmds_send(cabc_master_dsi_ctrl, &cabc_master_dsi_ctrl->cabc_on_cmds);
		return size;
	} else if (sysfs_streq(buf, "80")) {
		cabc_master_dsi_ctrl->cabc_on_cmds.cmds[0].payload[1] = 0x50;
		mdss_dsi_panel_cmds_send(cabc_slave_dsi_ctrl, &cabc_slave_dsi_ctrl->cabc_on_cmds);
		mdss_dsi_panel_cmds_send(cabc_master_dsi_ctrl, &cabc_master_dsi_ctrl->cabc_on_cmds);
		return size;
	} else if (sysfs_streq(buf, "81")) {
		cabc_master_dsi_ctrl->cabc_on_cmds.cmds[0].payload[1] = 0x51;
		mdss_dsi_panel_cmds_send(cabc_slave_dsi_ctrl, &cabc_slave_dsi_ctrl->cabc_on_cmds);
		mdss_dsi_panel_cmds_send(cabc_master_dsi_ctrl, &cabc_master_dsi_ctrl->cabc_on_cmds);
		return size;
	} else if (sysfs_streq(buf, "5")) {
		mdss_dsi_panel_cmds_send(cabc_slave_dsi_ctrl, &cabc_slave_dsi_ctrl->cabc_off_cmds);
		mdss_dsi_panel_cmds_send(cabc_master_dsi_ctrl, &cabc_master_dsi_ctrl->cabc_off_cmds);
		return size;
	}
	/* echo "tuning file" */
	memset(tuning_file, 0, sizeof(tuning_file));
	snprintf(tuning_file, MAX_FILE_NAME, "%s%s", TUNING_FILE_PATH, buf);

	pt = tuning_file;
	while (*pt) {
		if (*pt == '\r' || *pt == '\n') {
			*pt = 0;
			break;
		}
		pt++;
	}
	pr_info("%s:%s\n", __func__, tuning_file);
	load_tuning_file(tuning_file);

	for (b = 0; b < 3; b++) {
		for (a = 0; a < 7; a++) {
			printk(KERN_INFO "0x%x = 0x%x ", cabc_master_dsi_ctrl->cabc_tune_cmds.cmds[b].payload[a],
					cabc_tuning[a + (b*7)]);
			cabc_master_dsi_ctrl->cabc_tune_cmds.cmds[b].payload[a] = cabc_tuning[a + (b*7)];
		}
	}
	for (a = 0; a < 22; a++) {
			printk(KERN_INFO "0x%x = 0x%x ", cabc_master_dsi_ctrl->cabc_tune_cmds.cmds[3].payload[a],
					cabc_tuning[21 + a]);
			cabc_master_dsi_ctrl->cabc_tune_cmds.cmds[3].payload[a] = cabc_tuning[21 + a];
	}
	for (b = 0; b < 3; b++) {
		for (a = 0; a < 4; a++) {
			cabc_master_dsi_ctrl->cabc_tune_cmds.cmds[b+4].payload[a] = cabc_tuning[39 + a + (b*4)];
			printk(KERN_INFO "0x%x = 0x%x ", cabc_master_dsi_ctrl->cabc_tune_cmds.cmds[b+4].payload[a],
					cabc_tuning[39 + a + (b*4)]);
		}
	}

	mdss_dsi_panel_cmds_send(cabc_slave_dsi_ctrl, &cabc_slave_dsi_ctrl->cabc_tune_cmds);
	mdss_dsi_panel_cmds_send(cabc_master_dsi_ctrl, &cabc_master_dsi_ctrl->cabc_tune_cmds);

	return size;
}
static DEVICE_ATTR(cabctuning, 0664, cabc_tuning_show, cabc_tuning_store);

void cabc_tuning_init(struct mdss_dsi_ctrl_pdata *dsi_pdata)
{
	pr_info("%s : dsi dest = %d\n", __func__,
					dsi_pdata->panel_data.panel_info.pdest);

	if(dsi_pdata->panel_data.panel_info.pdest == MASTER_DSI_PDEST) {
		pr_debug("%s : Master DSI Ctrl Copy \n", __func__);
		cabc_master_dsi_ctrl = dsi_pdata;
	} else {
		pr_debug("%s : Slave DSI Ctrl Copy \n", __func__);
		cabc_slave_dsi_ctrl = dsi_pdata;
	}

	if (!cabc_tun_state.cabc_enable) {
		pr_info("%s : First Init \n", __func__);
		mutex_init(&cabc_tun_state.cabc_mutex);

		cabc_class = class_create(THIS_MODULE, "tcon");
		if (IS_ERR(cabc_class))
			pr_err("Failed to create class(cabc)!\n");

		/* Using for CABC Key String */
		mdnie_class = class_create(THIS_MODULE, "mdnie");
		if (IS_ERR(mdnie_class))
			pr_err("Failed to create class(mdnie)!\n");

		tune_cabc_dev = device_create(cabc_class, NULL, 0, NULL,
			  "tcon");

		if (IS_ERR(tune_cabc_dev))
			pr_err("Failed to create device(cabc)!\n");

		/* Using for CABC Key String */
		tune_mdnie_dev = device_create(mdnie_class, NULL, 0, NULL,
			  "mdnie");

		if (IS_ERR(tune_mdnie_dev))
			pr_err("Failed to create device(mdnie)!\n");

		if (device_create_file
		    (tune_cabc_dev, &dev_attr_auto_br) < 0)
			pr_err("Failed to create device file(%s)!\n",
		       dev_attr_auto_br.attr.name);

		if (device_create_file
		    (tune_cabc_dev, &dev_attr_lux) < 0)
			pr_err("Failed to create device file(%s)!\n",
		       dev_attr_lux.attr.name);

		if (device_create_file
		    (tune_cabc_dev, &dev_attr_mode) < 0)
			pr_err("Failed to create device file(%s)!\n",
		       dev_attr_mode.attr.name);

		if (device_create_file
		    (tune_cabc_dev,
		     &dev_attr_cetuning) < 0)
			pr_err("Failed to create device file(%s)!\n",
				dev_attr_cetuning.attr.name);

		if (device_create_file
		    (tune_cabc_dev, &dev_attr_cabctuning) < 0)
			pr_err("Failed to create device file(%s)!\n",
				dev_attr_cabctuning.attr.name);

		/* Using for CABC Key String */
		if (device_create_file
		    (tune_mdnie_dev, &dev_attr_cabc) < 0)
			pr_err("Failed to create device file(%s)!\n",
				dev_attr_cabc.attr.name);

		if (device_create_file
		    (tune_mdnie_dev, &dev_attr_accessibility) < 0)
			pr_err("Failed to create device file(%s)!\n",
				dev_attr_cabc.attr.name);

		cabc_tun_state.cabc_enable=1;
	}
}

