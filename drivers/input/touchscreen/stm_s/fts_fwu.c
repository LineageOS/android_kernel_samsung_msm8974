
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/hrtimer.h>
#include <linux/interrupt.h>

#include "fts_ts.h"

#define WRITE_CHUNK_SIZE 64
#define FTS_DEFAULT_UMS_FW "/sdcard/stm.fw"
#define FTS64FILE_SIGNATURE 0xaaaa5555

enum {
	BUILT_IN = 0,
	UMS,
};

struct fts64_header {
	unsigned int signature;
	unsigned short fw_ver;
	unsigned char fw_id;
	unsigned char reserved1;
	unsigned char internal_ver[8];
	unsigned char released_ver[8];
	unsigned int reserved2;
	unsigned int checksum;
};

int fts_fw_wait_for_flash_ready(struct fts_ts_info *info)
{
	unsigned char regAdd;
	unsigned char buf[3];
	int retry = 0;

	regAdd = FTS_CMD_READ_FLASH_STAT;

	while (info->fts_read_reg
		(info, &regAdd, 1, (unsigned char *)buf, 1)) {
		if ((buf[0] & 0x01) == 0)
			break;

		if (retry++ > FTS_RETRY_COUNT * 10) {
			tsp_debug_err(true, info->dev,
				 "%s: Time Over\n",
				 __func__);
			return -1;
			}
		msleep(20);
	}

	return 0;
}

int fts_fw_burn(struct fts_ts_info *info, unsigned char *fw_data)
{
	unsigned char regAdd[WRITE_CHUNK_SIZE + 3];
	int section;

	// Check busy Flash
	if (fts_fw_wait_for_flash_ready(info)<0)
		return -1;

	// FTS_CMD_UNLOCK_FLASH
	tsp_debug_info(true, info->dev, "%s: Unlock Flash\n", __func__);
	regAdd[0] = FTS_CMD_UNLOCK_FLASH;
	regAdd[1] = 0x74;
	regAdd[2] = 0x45;
	info->fts_write_reg(info, &regAdd[0], 3);
	msleep(500);

	// Copy to PRAM
	tsp_debug_info(true, info->dev, "%s: Copy to PRAM\n", __func__);
	regAdd[0] = FTS_CMD_WRITE_PRAM;
	for (section = 0; section < (64 * 1024 / WRITE_CHUNK_SIZE); section++) {
		regAdd[0] = FTS_CMD_WRITE_PRAM + (((section * WRITE_CHUNK_SIZE) >> 16) & 0x0f);
		regAdd[1] = ((section * WRITE_CHUNK_SIZE) >> 8) & 0xff;
		regAdd[2] = (section * WRITE_CHUNK_SIZE) & 0xff;
		memcpy(&regAdd[3],
			&fw_data[section * WRITE_CHUNK_SIZE +
				 sizeof(struct fts64_header)],
			WRITE_CHUNK_SIZE);
		info->fts_write_reg(info, &regAdd[0], WRITE_CHUNK_SIZE + 3);
	}

	msleep(100);

	// Erase Program Flash
	tsp_debug_info(true, info->dev, "%s: Erase Program Flash\n", __func__);
	info->fts_command(info, FTS_CMD_ERASE_PROG_FLASH);
	msleep(100);

	// Check busy Flash
	if (fts_fw_wait_for_flash_ready(info)<0)
		return -1;

	// Burn Program Flash
	tsp_debug_info(true, info->dev, "%s: Burn Program Flash\n", __func__);
	info->fts_command(info, FTS_CMD_BURN_PROG_FLASH);
	msleep(100);

	// Check busy Flash
	if (fts_fw_wait_for_flash_ready(info)<0)
		return -1;

	// Reset FTS
	tsp_debug_info(true, info->dev, "%s: Reset FTS\n", __func__);
	info->fts_systemreset(info);
	return 0;
}

int GetSystemStatus(struct fts_ts_info *info, unsigned char *val1, unsigned char *val2)
{
	bool rc = -1;
	unsigned char regAdd1[4] = { 0xb2, 0x07, 0xfb, 0x04 };
	unsigned char regAdd2[4] = { 0xb2, 0x17, 0xfb, 0x04 };
	unsigned char data[FTS_EVENT_SIZE];
	int retry = 0;

	info->fts_write_reg(info, &regAdd1[0], 4);
	info->fts_write_reg(info, &regAdd2[0], 4);

	memset(data, 0x0, FTS_EVENT_SIZE);
	regAdd1[0] = READ_ONE_EVENT;

	while (info->fts_read_reg(info, &regAdd1[0], 1, (unsigned char *)data,
				   FTS_EVENT_SIZE)) {
		if ((data[0] == 0x12) && (data[1] == regAdd1[1])
			&& (data[2] == regAdd1[2])) {
			rc = 0;
			*val1 = data[3];
			tsp_debug_info(true, info->dev,
				"%s: System Status 1 : 0x%02x\n",
			__func__, data[3]);
		}
		else if ((data[0] == 0x12) && (data[1] == regAdd2[1])
			&& (data[2] == regAdd2[2])) {
			rc = 0;
			*val2 = data[3];
			tsp_debug_info(true, info->dev,
				"%s: System Status 2 : 0x%02x\n",
			__func__, data[3]);
			break;
		}

		if (retry++ > FTS_RETRY_COUNT) {
			rc = -1;
			tsp_debug_err(true, info->dev,
				"Time Over - GetSystemStatus\n");
			break;
		}
	}
	return rc;
}

int fts_fw_wait_for_event(struct fts_ts_info *info, unsigned char eid)
{
	int rc;
	unsigned char regAdd;
	unsigned char data[FTS_EVENT_SIZE];
	int retry = 0;

	memset(data, 0x0, FTS_EVENT_SIZE);

	regAdd = READ_ONE_EVENT;
	rc = -1;
	while (info->fts_read_reg
	       (info, &regAdd, 1, (unsigned char *)data, FTS_EVENT_SIZE)) {
		if ((data[0] == EVENTID_STATUS_EVENT) &&
			(data[1] == eid)) {
			rc = 0;
			break;
		}

		if (retry++ > FTS_RETRY_COUNT * 15) {
			rc = -1;
			tsp_debug_info(true, info->dev, "%s: Time Over\n", __func__);
			break;
		}
		msleep(20);
	}

	return rc;
}


void fts_fw_init(struct fts_ts_info *info)
{
	tsp_debug_info(true, info->dev,"%s, line:%d\n",__func__, __LINE__);

	info->fts_command(info, SLEEPOUT);
	msleep(50);
	info->fts_command(info, CX_TUNNING);
	msleep(300);
	fts_fw_wait_for_event(info, 0x01);

    info->fts_command(info, SELF_AUTO_TUNE);
    msleep(300);


	fts_fw_wait_for_event(info, 0x42);

	info->fts_command(info, FTS_CMD_SAVE_CX_TUNING);
	msleep(400);

	info->fts_systemreset(info);
	info->fts_wait_for_ready(info);

	info->fts_command(info, SLEEPOUT);
	msleep(50);

	info->fts_command(info, SENSEON);

}

const int fts_fw_updater(struct fts_ts_info *info, unsigned char *fw_data)
{
	const struct fts64_header *header;
	int retval;
	int retry;
	unsigned short fw_main_version;

	if (!fw_data) {
		tsp_debug_err(true, info->dev, "%s: Firmware data is NULL\n",
			__func__);
		return -ENODEV;
	}

	header = (struct fts64_header *)fw_data;
	fw_main_version = (header->released_ver[0] << 8) +
							(header->released_ver[1]);

	tsp_debug_info(true, info->dev,
		  "Starting firmware update : 0x%04X\n",
		  fw_main_version);

	retry = 0;
	while (1) {
		retval = fts_fw_burn(info, fw_data);
		if (retval >= 0) {
			info->fts_wait_for_ready(info);
			info->fts_get_version_info(info);

#ifdef FTS_SUPPORT_NOISE_PARAM
			info->fts_get_noise_param_address(info);
#endif

			if (fw_main_version == info->fw_main_version_of_ic) {
				tsp_debug_info(true, info->dev,
					  "%s: Success Firmware update\n",
					  __func__);
				fts_fw_init(info);
				retval = 0;
				break;
			}
		}

		if (retry++ > 3) {
			tsp_debug_err(true, info->dev, "%s: Fail Firmware update\n",
				 __func__);
			retval = -1;
			break;
			}
	}

	return retval;
}
EXPORT_SYMBOL(fts_fw_updater);

int fts_fw_update_on_probe(struct fts_ts_info *info)
{
	int retval;
	const struct firmware *fw_entry = NULL;
	unsigned char *fw_data = NULL;
	char fw_path[FTS_MAX_FW_PATH];
	const struct fts64_header *header;
	unsigned char SYS_STAT[2] = {0, };

	snprintf(fw_path, FTS_MAX_FW_PATH, "%s", info->board->firmware_name);
	tsp_debug_info(true, info->dev, "%s: Load firmware : %s\n", __func__,
		  fw_path);
	retval = request_firmware(&fw_entry, fw_path, info->dev);
	if (retval) {
		tsp_debug_err(true, info->dev,
			"%s: Firmware image %s not available\n", __func__,
			fw_path);
		goto done;
	}
	fw_data = (unsigned char *)fw_entry->data;
	header = (struct fts64_header *)fw_data;

	info->fw_version_of_bin = (fw_data[5] << 8)+fw_data[4];
	info->fw_main_version_of_bin = (header->released_ver[0] << 8) +
								(header->released_ver[1]);
	info->config_version_of_bin = (fw_data[0xf822] << 8)+fw_data[0xf821];

	tsp_debug_info(true, info->dev,
					"Bin Firmware Version : 0x%04X "
					"Bin Config Version : 0x%04X "
					"Bin Main Version : 0x%04X\n",
					info->fw_version_of_bin,
					info->config_version_of_bin,
					info->fw_main_version_of_bin);

	if ((info->fw_main_version_of_ic < info->fw_main_version_of_bin)
		|| (info->config_version_of_ic < info->config_version_of_bin)
		|| (info->fw_version_of_ic < info->fw_version_of_bin))
		retval = fts_fw_updater(info, fw_data);
	else
		retval = -2;

	if (GetSystemStatus(info, &SYS_STAT[0], &SYS_STAT[1]) >= 0) {
		if (SYS_STAT[0] != SYS_STAT[1]) {
			info->fts_systemreset(info);
			msleep(20);
			info->fts_wait_for_ready(info);
			fts_fw_init(info);
		}
	}

done:
	if (fw_entry)
		release_firmware(fw_entry);

	return retval;
}
EXPORT_SYMBOL(fts_fw_update_on_probe);

static int fts_load_fw_from_kernel(struct fts_ts_info *info,
				 const char *fw_path)
{
	int retval;
	const struct firmware *fw_entry = NULL;
	unsigned char *fw_data = NULL;

	if (!fw_path) {
		tsp_debug_err(true, info->dev, "%s: Firmware name is not defined\n",
			__func__);
		return -EINVAL;
	}

	tsp_debug_info(true, info->dev, "%s: Load firmware : %s\n", __func__,
		 fw_path);

	retval = request_firmware(&fw_entry, fw_path, info->dev);

	if (retval) {
		tsp_debug_err(true, info->dev,
			"%s: Firmware image %s not available\n", __func__,
			fw_path);
		goto done;
	}

	fw_data = (unsigned char *)fw_entry->data;
	// Disable Interrupt
	if (info->irq)
		disable_irq(info->irq);
	else
		hrtimer_cancel(&info->timer);

	info->fts_systemreset(info);
	info->fts_wait_for_ready(info);

	retval = fts_fw_updater(info, fw_data);
	if (retval)
		tsp_debug_err(true, info->dev, "%s: failed update firmware\n",
			__func__);

	if (info->irq)
		enable_irq(info->irq);
	else
		hrtimer_start(&info->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
 done:
	if (fw_entry)
		release_firmware(fw_entry);

	return retval;
}

static int fts_load_fw_from_ums(struct fts_ts_info *info)
{
	struct file *fp;
	mm_segment_t old_fs;
	unsigned int fw_size, nread;
	int error = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(FTS_DEFAULT_UMS_FW, O_RDONLY, S_IRUSR);
	if (IS_ERR(fp)) {
		tsp_debug_err(true, info->dev, "%s: failed to open %s.\n", __func__,
			FTS_DEFAULT_UMS_FW);
		error = -ENOENT;
		goto open_err;
	}

	fw_size = fp->f_path.dentry->d_inode->i_size;

	if (0 < fw_size) {
		unsigned char *fw_data;
		const struct fts64_header *header;
		fw_data = kzalloc(fw_size, GFP_KERNEL);
		nread = vfs_read(fp, (char __user *)fw_data,
				 fw_size, &fp->f_pos);

		tsp_debug_info(true, info->dev,
			 "%s: start, file path %s, size %u Bytes\n",
			 __func__, FTS_DEFAULT_UMS_FW, fw_size);

		if (nread != fw_size) {
			tsp_debug_err(true, info->dev,
				"%s: failed to read firmware file, nread %u Bytes\n",
				__func__, nread);
			error = -EIO;
		} else {
			header = (struct fts64_header *)fw_data;
			if (header->signature == FTS64FILE_SIGNATURE) {
				if (info->irq)
					disable_irq(info->irq);
				else
					hrtimer_cancel(&info->timer);

				info->fts_systemreset(info);
				info->fts_wait_for_ready(info);

				tsp_debug_info(true, info->dev,
					"[UMS] Firmware Version : 0x%04X "
					"[UMS] Main Version : 0x%04X\n",
					(fw_data[5] << 8)+fw_data[4],
					(header->released_ver[0] << 8) +
							(header->released_ver[1]));

				error = fts_fw_updater(info, fw_data);

				if (info->irq)
					enable_irq(info->irq);
				else
					hrtimer_start(&info->timer,
						  ktime_set(1, 0),
						  HRTIMER_MODE_REL);
			} else {
				error = -1;
				tsp_debug_err(true, info->dev,
					 "%s: File type is not match with FTS64 file. [%8x]\n",
					 __func__, header->signature);
				}
		}

		if (error < 0)
			tsp_debug_err(true, info->dev, "%s: failed update firmware\n",
				__func__);

		kfree(fw_data);
	}

	filp_close(fp, NULL);

 open_err:
	set_fs(old_fs);
	return error;
}

int fts_fw_update_on_hidden_menu(struct fts_ts_info *info, int update_type)
{
	int retval = 0;

	/* Factory cmd for firmware update
	 * argument represent what is source of firmware like below.
	 *
	 * 0 : [BUILT_IN] Getting firmware which is for user.
	 * 1 : [UMS] Getting firmware from sd card.
	 */
	switch (update_type) {
	case BUILT_IN:
		retval = fts_load_fw_from_kernel(info, info->board->firmware_name);
		break;

	case UMS:
		retval = fts_load_fw_from_ums(info);
		break;

	default:
		tsp_debug_err(true, info->dev, "%s: Not support command[%d]\n",
			__func__, update_type);
		break;
	}

	return retval;
}
EXPORT_SYMBOL(fts_fw_update_on_hidden_menu);

