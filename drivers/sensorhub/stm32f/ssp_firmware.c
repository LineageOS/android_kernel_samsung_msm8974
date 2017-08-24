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
#include "ssp.h"

#define SSP_FIRMWARE_REVISION		101901
#ifdef CONFIG_SENSORS_SSP_BOUNCE_FIRMWARE
#define SSP_FIRMWARE_REVISION_STM	14010300 /* Latest, 88922*/
#else
#define SSP_FIRMWARE_REVISION_STM	15041500 /* Latest, 88922*/
#endif
#define SSP_FIRMWARE_REVISION_STM_88921	13072401 /* 88921 */
#define SSP_FIRMWARE_REVISION_STM_RVS	13051500
#define SSP_FIRMWARE_REVISION_JSGLTE	14120500

#define BOOT_SPI_HZ	960000
#define NORM_SPI_HZ	4800000

/* Bootload mode cmd */
#ifdef CONFIG_SENSORS_SSP_BOUNCE_FIRMWARE
#define BL_FW_NAME			"ssp_stm_bf.fw"
#else
#define BL_FW_NAME			"ssp_stm32f.fw"
#endif
#define BL_FW_JSGLTE_NAME			"ssp_stm32f_jsglte.fw"
#define BL_UMS_FW_NAME			"ssp_stm.bin"
#define BL_CRASHED_FW_NAME		"ssp_crashed.fw"

#define BL_UMS_FW_PATH			255

#define APP_SLAVE_ADDR			0x18
#define BOOTLOADER_SLAVE_ADDR		0x26

/* Bootloader mode status */
#define BL_WAITING_BOOTLOAD_CMD		0xc0	/* valid 7 6 bit only */
#define BL_WAITING_FRAME_DATA		0x80	/* valid 7 6 bit only */
#define BL_FRAME_CRC_CHECK		0x02
#define BL_FRAME_CRC_FAIL		0x03
#define BL_FRAME_CRC_PASS		0x04
#define BL_APP_CRC_FAIL			0x40	/* valid 7 8 bit only */
#define BL_BOOT_STATUS_MASK		0x3f

/* Command to unlock bootloader */
#define BL_UNLOCK_CMD_MSB		0xaa
#define BL_UNLOCK_CMD_LSB		0xdc

/* STM */
#define SSP_STM_DEBUG	0
#define STM_SHOULD_BE_IMPLEMENT	0

#define SEND_ADDR_LEN	5
#define BL_SPI_SOF 0x5A
#define BL_ACK  0x79
#define BL_ACK2  0xF9
#define BL_NACK 0x1F
#define BL_IDLE 0xA5

#define STM_MAX_XFER_SIZE 256
#define STM_MAX_BUFFER_SIZE 260
#define STM_APP_ADDR	0x08000000

#define BYTE_DELAY_READ 10
#define BYTE_DELAY_WRITE 8

#define DEF_ACK_ERASE_NUMBER 7000
#define DEF_ACKCMD_NUMBER    30
#define DEF_ACKROOF_NUMBER   20

#define WMEM_COMMAND           0x31  /* Write Memory command          */
#define GO_COMMAND             0x21  /* GO command                    */
#define EXT_ER_COMMAND         0x44  /* Erase Memory command          */

#define XOR_RMEM_COMMAND       0xEE  /* Read Memory command           */

#define XOR_WMEM_COMMAND       0xCE  /* Write Memory command          */
#define XOR_GO_COMMAND         0xDE  /* GO command                    */
#define XOR_EXT_ER_COMMAND     0xBB  /* Erase Memory command          */

#define EXT_ER_DATA_LEN	3
#define BLMODE_RETRYCOUNT 3
#define BYTETOBYTE_USED 0

struct stm32fwu_spi_cmd {
	u8 cmd;
	u8 xor_cmd;
	u8 ack_pad; /* Send this when waiting for an ACK */
	u8 reserved;
	int status; /* ACK or NACK (or error) */
	int timeout; /* This is number of retries */
	int ack_loops;
};

unsigned int get_module_rev(struct ssp_data *data)
{
#if defined (CONFIG_MACH_VIKALCU)
		return SSP_FIRMWARE_REVISION_STM;
#elif defined (CONFIG_MACH_JSGLTE_CHN_CMCC)
		return SSP_FIRMWARE_REVISION_JSGLTE;
#else
	if (data->ap_rev > 3)
		return SSP_FIRMWARE_REVISION_STM;
	else
		return SSP_FIRMWARE_REVISION_STM_88921;
#endif
}

static int stm32fwu_spi_wait_for_ack(struct spi_device *spi,
				     struct stm32fwu_spi_cmd *cmd, u8 dummy_bytes)
{
	static int check_spi_wait_cnt = 1;

	struct spi_message m;
	char tx_buf = 0x0;
	char rx_buf = 0x0;
	struct spi_transfer	t = {
		.tx_buf		= &tx_buf,
		.rx_buf		= &rx_buf,
		.len		= 1,
		.bits_per_word = 8,
	};
	int i = 0;
	int ret;

#if SSP_STM_DEBUG
	pr_info("[SSP] %s: dummy byte = 0x%02hhx\n",
		__func__, dummy_bytes);
#endif
	while (i < cmd->timeout) {
		tx_buf = dummy_bytes;
		spi_message_init(&m);
		spi_message_add_tail(&t, &m);

		ret = spi_sync(spi, &m);
		if (ret < 0) {
			dev_err(&spi->dev, "%s: spi error %d\n", __func__, ret);
			return ret;
		} else  if ((rx_buf == BL_ACK) || (rx_buf == BL_ACK2)) {
			cmd->ack_loops = i;
			return BL_ACK;
		} else if (rx_buf == BL_NACK) {
			return (int)rx_buf;
		}
		if (check_spi_wait_cnt % 20 == 0)
			msleep(1);
		else
			usleep_range(1000, 1100);
		i++;
		check_spi_wait_cnt++;
	}
#if SSP_STM_DEBUG
	dev_err(&spi->dev, "%s: Timeout after %d loops\n", __func__, cmd->timeout);
#endif
	return -EIO;
}

static int stm32fwu_spi_send_cmd(struct spi_device *spi,
				 struct stm32fwu_spi_cmd *cmd)
{
	u8 tx_buf[3] = {0,};
	u8 rx_buf[3] = {0,};
	u8 dummy_byte = 0;
	struct spi_message m;
	int ret;
#if BYTETOBYTE_USED
	int i;
	struct spi_transfer t[STM_MAX_BUFFER_SIZE];
	memset(t, 0, STM_MAX_BUFFER_SIZE * sizeof(struct spi_transfer));
#else
	struct spi_transfer	t = {
		.tx_buf		= tx_buf,
		.rx_buf		= rx_buf,
		.len		= 3,
		.bits_per_word = 8,
	};
#endif
	spi_message_init(&m);
	tx_buf[0] = BL_SPI_SOF;
	tx_buf[1] = cmd->cmd;
	tx_buf[2] = cmd->xor_cmd;

#if BYTETOBYTE_USED
	for (i = 0; i < 3; i++) {
		t[i].tx_buf = &tx_buf[i];
		t[i].rx_buf = &rx_buf[i];
		t[i].len = 1;
		t[i].bits_per_word = 8;
		t[i].delay_usecs = BYTE_DELAY_WRITE;
		spi_message_add_tail(&t[i], &m);
        }
#else
	spi_message_add_tail(&t, &m);
#endif

	ret = spi_sync(spi, &m);
	if (ret < 0) {
		dev_err(&spi->dev, "%s: spi error %d\n", __func__, ret);
		return ret;
	}

	dummy_byte = cmd->ack_pad;

	/* check for ack/nack and loop until found */
	ret = stm32fwu_spi_wait_for_ack(spi, cmd, dummy_byte);
	cmd->status = ret;

	if (ret != BL_ACK) {
		pr_err("[SSP] %s: Got NAK or Error %d\n", __func__, ret);
		return ret;
	}

	return ret;
}
#if STM_SHOULD_BE_IMPLEMENT
static int stm32fwu_spi_read(struct spi_device *spi,
	u8 *buffer, ssize_t len)
{
	int ret;
	int i;
	u8 tx_buf[STM_MAX_BUFFER_SIZE] = {0,};
	struct spi_message m;
	struct spi_transfer	t[STM_MAX_BUFFER_SIZE];

	memset(t, 0, STM_MAX_BUFFER_SIZE * sizeof(struct spi_transfer));

	spi_message_init(&m);

	for (i = 0; i < len; i++) {
		t[i].tx_buf = tx_buf;
		t[i].rx_buf = &buffer[i];
		t[i].len = 1;
		t[i].bits_per_word = 8;
		t[i].delay_usecs = BYTE_DELAY_READ;
		spi_message_add_tail(&t[i], &m);
	}

	ret = spi_sync(spi, &m);

	if (ret < 0) {
		pr_err("[SSP] Error in %d spi_read()\n", ret);
		return ret;
	}

	return len;
}
#endif

static int stm32fwu_spi_write(struct spi_device *spi,
	const u8 *buffer, ssize_t len)
{
	int ret;
	u8 rx_buf[STM_MAX_BUFFER_SIZE] = {0,};
	struct spi_message m;
#if BYTETOBYTE_USED
	struct spi_transfer t[STM_MAX_BUFFER_SIZE];
	memset(t, 0, STM_MAX_BUFFER_SIZE * sizeof(struct spi_transfer));
	int i;
#else
	struct spi_transfer	t = {
		.tx_buf		= buffer,
		.rx_buf		= rx_buf,
		.len		= len,
		.bits_per_word = 8,
	};
#endif
	spi_message_init(&m);
#if BYTETOBYTE_USED
	for (i = 0; i < len; i++) {
		t[i].tx_buf = &buffer[i];
		t[i].rx_buf = &rx_buf[i];
		t[i].len = 1;
		t[i].bits_per_word = 8;
		t[i].delay_usecs = BYTE_DELAY_WRITE;
		spi_message_add_tail(&t[i], &m);
	}
#else
	spi_message_add_tail(&t, &m);
#endif
	ret = spi_sync(spi, &m);

	if (ret < 0) {
		pr_err("[SSP] Error in %d spi_write()\n", ret);
		return ret;
	}

	return len;
}

static int send_addr(struct spi_device *spi, u32 fw_addr, int send_short)
{
	int res;
	int i = send_short;
	int len = SEND_ADDR_LEN - send_short;
	u8 header[SEND_ADDR_LEN];
	struct stm32fwu_spi_cmd dummy_cmd;
	dummy_cmd.timeout = DEF_ACKROOF_NUMBER;


	header[0] = (u8)((fw_addr >> 24) & 0xFF);
	header[1] = (u8)((fw_addr >> 16) & 0xFF);
	header[2] = (u8)((fw_addr >> 8) & 0xFF);
	header[3] = (u8)(fw_addr & 0xFF);
	header[4] = header[0] ^ header[1] ^ header[2] ^ header[3];

	res = stm32fwu_spi_write(spi, &header[i], len);

	if (res <  len) {
		pr_err("[SSP] Error in sending address. Res  %d\n", res);
		return ((res > 0) ? -EIO : res);
	}

	res = stm32fwu_spi_wait_for_ack(spi, &dummy_cmd, BL_ACK);
	if (res != BL_ACK) {
		pr_err("[SSP] send_addr(): rcv_ack returned 0x%x\n",
			res);
		return res;
	}
	return 0;
}

#if STM_SHOULD_BE_IMPLEMENT
static int send_byte_count(struct spi_device *spi, int bytes, int get_ack)
{
	int res;
	uchar bbuff[3];
	struct stm32fwu_spi_cmd dummy_cmd;

	if (bytes > 256) {
		return -EINVAL;
	}

	bbuff[0] = bytes - 1;
	bbuff[1] = ~bbuff[0];

	res = stm32fwu_spi_write(spi, bbuff, 2);

	if (res <  2) {
		return -EPROTO;
	}

	if (get_ack) {
		dummy_cmd.timeout = DEF_ACKROOF_NUMBER;
		res = stm32fwu_spi_wait_for_ack(spi, &dummy_cmd, BL_ACK);

		if (res != BL_ACK) {
			return -EPROTO;
		}
	}
	return 0;
}

static int fw_read_stm(struct spi_device *spi, u32 fw_addr,
	int len, const u8 *buffer)
{
	int res;
	struct stm32fwu_spi_cmd cmd;
	struct stm32fwu_spi_cmd dummy_cmd;
	int i;
	u8 xor = 0;
	u8 send_buff[STM_MAX_BUFFER_SIZE] = {0,};

	cmd.cmd = WMEM_COMMAND;
	cmd.xor_cmd = XOR_RMEM_COMMAND;
	cmd.timeout = DEF_ACKCMD_NUMBER;
	cmd.ack_pad = (u8)((fw_addr >> 24) & 0xFF);

	res = stm32fwu_spi_send_cmd(spi, &cmd);

	if (res != BL_ACK) {
		pr_err("[SSP] Error %d sending read_mem cmd\n", res);
		return res;
	}

	if (cmd.ack_loops > 0)
		res = send_addr(spi, fw_addr, 1);
	else
		res = send_addr(spi, fw_addr, 0);

	if (res != 0) {
		pr_err("[SSP] Error %d sending read_mem Address\n", res);
		return res;
	}

	res = send_byte_count(spi, len, 1);

	if (res != 0) {
		return -EPROTO;
	}

	res = stm32fwu_spi_read(spi, buffer, len);

	if (res < len) {
		return -EIO;
	}

	return len;
}
#endif

static int fw_write_stm(struct spi_device *spi, u32 fw_addr,
	int len, const u8 *buffer)
{
	int res;
	struct stm32fwu_spi_cmd cmd;
	struct stm32fwu_spi_cmd dummy_cmd;
	int i;
	u8 xor = 0;
	u8 send_buff[STM_MAX_BUFFER_SIZE] = {0,};

	cmd.cmd = WMEM_COMMAND;
	cmd.xor_cmd = XOR_WMEM_COMMAND;
	cmd.timeout = DEF_ACKCMD_NUMBER;
	cmd.ack_pad = (u8)((fw_addr >> 24) & 0xFF);

#if SSP_STM_DEBUG
	pr_info("[SSP] sending WMEM_COMMAND\n");
#endif

	if (len > STM_MAX_XFER_SIZE) {
		pr_err("[SSP] Can't send more than 256 bytes per transaction\n");
		return -EINVAL;
	}

	send_buff[0] = len - 1;
	memcpy(&send_buff[1], buffer, len);
	for (i = 0; i < (len + 1); i++)
		xor ^= send_buff[i];

	send_buff[len + 1] = xor;

	res = stm32fwu_spi_send_cmd(spi, &cmd);
	if (res != BL_ACK) {
		pr_err("[SSP] Error %d sending read_mem cmd\n", res);
		return res;
	}

	if (cmd.ack_loops > 0)
		res = send_addr(spi, fw_addr, 1);
	else
		res = send_addr(spi, fw_addr, 0);

	if (res != 0) {
		pr_err("[SSP] Error %d sending write_mem Address\n", res);
		return res;
	}

	res = stm32fwu_spi_write(spi, send_buff, len + 2);
	if (res < len) {
		pr_err("[SSP] Error writing to flash. res = %d\n", res);
		return ((res > 0) ? -EIO : res);
	}

	dummy_cmd.timeout = DEF_ACKROOF_NUMBER;
	usleep_range(100, 150); /* Samsung added */
	res = stm32fwu_spi_wait_for_ack(spi, &dummy_cmd, BL_ACK);
	if (res == BL_ACK) {
		return len;
	}

	if (res == BL_NACK) {
		pr_err("[SSP] Got NAK waiting for WRITE_MEM to complete\n");
		return -EPROTO;
	}
	pr_err("[SSP] timeout waiting for ACK for WRITE_MEM command\n");
	return -ETIME;
}

static int load_ums_fw_bootmode(struct spi_device *spi, const char *pFn)
{
	const u8 *buff = NULL;
	char fw_path[BL_UMS_FW_PATH+1];
	unsigned int uFSize = 0, uNRead = 0;
	unsigned int uPos = 0;
	int iRet = SUCCESS;
	int remaining;
	int block = STM_MAX_XFER_SIZE;
	unsigned int fw_addr = STM_APP_ADDR;
	int retry_count = 0;
	int err_count = 0;
	int count = 0;
	struct file *fp = NULL;
	mm_segment_t old_fs = get_fs();

	pr_info("[SSP] ssp_load_ums_fw start!!!\n");

	old_fs = get_fs();
	set_fs(get_ds());

	snprintf(fw_path, BL_UMS_FW_PATH, "/sdcard/ssp/%s", pFn);

	fp = filp_open(fw_path, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		iRet = ERROR;
		pr_err("file %s open error:%d\n", fw_path, (s32)fp);
		goto err_open;
	}

	uFSize = (unsigned int)fp->f_path.dentry->d_inode->i_size;
	pr_info("ssp_load_ums firmware size: %u\n", uFSize);

	buff = kzalloc((size_t)uFSize, GFP_KERNEL);
	if (!buff) {
		iRet = ERROR;
		pr_err("fail to alloc buffer for fw\n");
		goto err_alloc;
	}

	uNRead = (unsigned int)vfs_read(fp, (char __user *)buff,
			(unsigned int)uFSize, &fp->f_pos);
	if (uNRead != uFSize) {
		iRet = ERROR;
		pr_err("fail to read file %s (nread = %u)\n", fw_path, uNRead);
		goto err_fw_size;
	}
	remaining = uFSize;

	while (remaining > 0) {
		if (block > remaining)
			block = remaining;

		while (retry_count < 3) {
			iRet = fw_write_stm(spi, fw_addr, block, buff + uPos);
			if (iRet < block) {
				pr_err("[SSP] Error writing to addr 0x%08X\n", fw_addr);
				if (iRet < 0) {
					pr_err("[SSP] Erro was %d\n", iRet);
				} else {
					pr_err("[SSP] Incomplete write of %d bytes\n",
						iRet);
					iRet = -EIO;
				}
				retry_count++;
				err_count++;
			} else {
				retry_count = 0;
				break;
			}
		}
		if (iRet < 0) {
			pr_err("[SSP] Writing MEM failed: %d, retry cont: %d\n", iRet, err_count);
			goto out;
		}
		remaining -= block;
		uPos += block;
		fw_addr += block;
		if (count++ == 50) {
			pr_info("[SSP] Updated %u bytes / %u bytes\n", uPos,
				uFSize);
			count = 0;
		}
	}
	pr_info("[SSP] Firm up(UMS) success(%d bytes, retry %d)\n", uPos, err_count);
out:
err_fw_size:
	kfree(buff);
err_alloc:
	filp_close(fp, NULL);
err_open:
	set_fs(old_fs);

	return iRet;
}

static int fw_erase_stm(struct spi_device *spi)
{
	struct stm32fwu_spi_cmd cmd;
	struct stm32fwu_spi_cmd dummy_cmd;
	int ret;
	char buff[EXT_ER_DATA_LEN] = {0xff, 0xff, 0x00};

	cmd.cmd = EXT_ER_COMMAND;
	cmd.xor_cmd = XOR_EXT_ER_COMMAND;
	cmd.timeout = DEF_ACKCMD_NUMBER;
	cmd.ack_pad = 0xFF;

	ret = stm32fwu_spi_send_cmd(spi, &cmd);

	if (ret < 0 || ret != BL_ACK) {
		pr_err("[SSP] fw_erase failed\n");
		return ret;
	}
	if (cmd.ack_loops == 0)
		ret = stm32fwu_spi_write(spi, buff, EXT_ER_DATA_LEN);
	else
		ret = stm32fwu_spi_write(spi, buff, EXT_ER_DATA_LEN-1);

	if (ret < (EXT_ER_DATA_LEN - cmd.ack_loops)) {
		return -EPROTO;
	}
	dummy_cmd.timeout = DEF_ACK_ERASE_NUMBER;
	ret = stm32fwu_spi_wait_for_ack(spi, &dummy_cmd, BL_ACK);


	if (ret == BL_ACK)
		return 0;
	else if (ret == BL_NACK)
		return -EPROTO;
	else
		return -ETIME;

}

static int load_kernel_fw_bootmode(struct spi_device *spi, const char *pFn)
{
	const struct firmware *fw = NULL;
	int remaining;
	unsigned int uPos = 0;
	unsigned int fw_addr = STM_APP_ADDR;
	int iRet;
	int block = STM_MAX_XFER_SIZE;
	int count = 0;
	int err_count = 0;
	int retry_count = 0;

	pr_info("[SSP] ssp_load_fw start!!!\n");

	iRet = request_firmware(&fw, pFn, &spi->dev);
	if (iRet) {
		pr_err("[SSP] Unable to open firmware %s\n", pFn);
		return iRet;
	}

	remaining = fw->size;
	while (remaining > 0) {
		if (block > remaining)
			block = remaining;

		while (retry_count < 3) {
			iRet = fw_write_stm(spi, fw_addr, block, fw->data + uPos);
			if (iRet < block) {
				pr_err("[SSP] Error writing to addr 0x%08X\n", fw_addr);
				if (iRet < 0) {
					pr_err("[SSP] Erro was %d\n", iRet);
				} else {
					pr_err("[SSP] Incomplete write of %d bytes\n",
						iRet);
					iRet = -EIO;
				}
				retry_count++;
				err_count++;
			} else {
				retry_count = 0;
				break;
			}
		}
		if (iRet < 0) {
			pr_err("[SSP] Writing MEM failed: %d, retry cont: %d\n", iRet, err_count);
			goto out_load_kernel;
		}
		remaining -= block;
		uPos += block;
		fw_addr += block;
		if (count++ == 20) {
			pr_info("[SSP] Updated %u bytes / %u bytes\n", uPos,
				fw->size);
			count = 0;
		}
	}

	pr_info("[SSP] Firmware download is success.(%d bytes, retry %d)\n", uPos, err_count);

out_load_kernel:
	release_firmware(fw);
	return iRet;
}

static int change_to_bootmode(struct ssp_data *data)
{
	int iCnt;
	int ret;
	char syncb = BL_SPI_SOF;
	struct stm32fwu_spi_cmd dummy_cmd;

	dummy_cmd.timeout = DEF_ACKCMD_NUMBER;

	gpio_set_value_cansleep(data->rst, 0);
	mdelay(4);
	gpio_set_value_cansleep(data->rst, 1);
	usleep_range(45000, 47000);


	for (iCnt = 0; iCnt < 9; iCnt++) {
		gpio_set_value_cansleep(data->rst, 0);
		mdelay(4);
		gpio_set_value_cansleep(data->rst, 1);
		usleep_range(15000, 15500);
	}
	ret = stm32fwu_spi_write(data->spi, &syncb, 1);
#if SSP_STM_DEBUG
	pr_info("[SSP] stm32fwu_spi_write(sync byte) returned %d\n", ret);
#endif
	ret = stm32fwu_spi_wait_for_ack(data->spi, &dummy_cmd, BL_ACK);
#if SSP_STM_DEBUG
	pr_info("[SSP] stm32fwu_spi_wait_for_ack returned %d (0x%x)\n",
		ret, ret);
#endif
	return ret;
}

void toggle_mcu_reset(struct ssp_data *data)
{
	gpio_set_value_cansleep(data->rst, 0);

	usleep_range(1000, 1200);

	gpio_set_value_cansleep(data->rst, 1);
	msleep(50);
}

static int update_mcu_bin(struct ssp_data *data, int iBinType)
{
	int retry = BLMODE_RETRYCOUNT;
	int iRet = SUCCESS;
	struct stm32fwu_spi_cmd cmd;

	cmd.cmd = GO_COMMAND;
	cmd.xor_cmd = XOR_GO_COMMAND;
	cmd.timeout = 1000;
	cmd.ack_pad = (u8)((STM_APP_ADDR >> 24) & 0xFF);

	do {
		iRet = change_to_bootmode(data);
		pr_info("[ssp] bootmode %d retry: %d\n", iRet, 3 - retry);
	} while (retry-- > 0 && iRet != BL_ACK );

	if(iRet != BL_ACK) {
		pr_err("[SSP]: %s - change_to_bootmode %d\n",
			__func__, iRet);
		return iRet;
	}

	iRet = fw_erase_stm(data->spi);
	if (iRet < 0) {
		pr_err("[SSP]: %s - fw_erase_stm %d\n",
			__func__, iRet);
		return iRet;
	}

	switch (iBinType) {
	case KERNEL_BINARY:
	 /* HW request: I2C line is reversed */
#if defined(CONFIG_MACH_JSGLTE_CHN_CMCC)
	iRet = load_kernel_fw_bootmode(data->spi,
		BL_FW_JSGLTE_NAME);
#else
	iRet = load_kernel_fw_bootmode(data->spi,
		BL_FW_NAME);
#endif
		break;
	case KERNEL_CRASHED_BINARY:
		iRet = load_kernel_fw_bootmode(data->spi,
		BL_CRASHED_FW_NAME);
	break;
	case UMS_BINARY:
		iRet = load_ums_fw_bootmode(data->spi,
			BL_UMS_FW_NAME);
		break;

	default:
		pr_err("[SSP] binary type error!!\n");
	}
/* STM : GO USER ADDR */
	stm32fwu_spi_send_cmd(data->spi, &cmd);
	if (cmd.ack_loops > 0)
		send_addr(data->spi, STM_APP_ADDR, 1);
	else
		send_addr(data->spi, STM_APP_ADDR, 0);

	msleep(SSP_SW_RESET_TIME);

	return iRet;
}

int forced_to_download_binary(struct ssp_data *data, int iBinType)
{
	int iRet = 0;
	int retry = 3;

	ssp_dbg("[SSP]: %s - mcu binany update!\n", __func__);

	ssp_enable(data, false);

	data->fw_dl_state = FW_DL_STATE_DOWNLOADING;
	pr_info("[SSP] %s, DL state = %d\n", __func__,
	data->fw_dl_state);
	data->spi->max_speed_hz = BOOT_SPI_HZ;
	if (spi_setup(data->spi))
		pr_err("failed to setup spi for ssp_boot\n");

	do {
		pr_info("[SSP] %d try\n", 3 - retry);
		iRet = update_mcu_bin(data, iBinType);
	} while (retry -- > 0 && iRet < 0);
	data->spi->max_speed_hz = NORM_SPI_HZ;
	if (spi_setup(data->spi))
		pr_err("failed to setup spi for ssp_norm\n");

	if (iRet < 0) {
		ssp_dbg("[SSP]: %s - update_mcu_bin failed!\n", __func__);
		goto out;
	}

	data->fw_dl_state = FW_DL_STATE_SYNC;
	pr_info("[SSP] %s, DL state = %d\n", __func__, data->fw_dl_state);
	clean_pending_list(data);
	ssp_enable(data, true);

	iRet = initialize_mcu(data);
	if (iRet < 0) {
		iRet = ERROR;
		ssp_dbg("[SSP]: %s - initialize_mcu failed!\n", __func__);
		goto out;
	}

	proximity_open_lcd_ldi(data);
	proximity_open_calibration(data);
	accel_open_calibration(data);
	gyro_open_calibration(data);
	pressure_open_calibration(data);
	if (mag_open_hwoffset(data) < 0)
		pr_info("[SSP]: %s - mag_open_hw_offset"
			" failed, %d\n", __func__, iRet);

	sync_sensor_state(data);

#ifdef CONFIG_SENSORS_SSP_SENSORHUB
	ssp_sensorhub_report_notice(data, MSG2SSP_AP_STATUS_RESET);
#endif

	data->fw_dl_state = FW_DL_STATE_DONE;
	pr_info("[SSP] %s, DL state = %d\n", __func__, data->fw_dl_state);

	iRet = SUCCESS;
out:
	return iRet;
}

int check_fwbl(struct ssp_data *data)
{

	unsigned int fw_revision;

	pr_info("[SSP] change_rev = %d\n", data->ssp_changes);
#if defined (CONFIG_MACH_JSGLTE_CHN_CMCC)
	fw_revision = SSP_FIRMWARE_REVISION_JSGLTE;
#elif defined (CONFIG_MACH_VIKALCU)
	fw_revision = SSP_FIRMWARE_REVISION_STM;
#elif defined (CONFIG_MACH_HLTEVZW) || defined (CONFIG_MACH_HLTESPR) \
	|| defined (CONFIG_MACH_HLTEUSC)
		if (data->ap_rev > 3)
			fw_revision = SSP_FIRMWARE_REVISION_STM;
		else if (data->ap_rev > 2)
			fw_revision = SSP_FIRMWARE_REVISION_STM_88921;
		else
			fw_revision = SSP_FIRMWARE_REVISION_STM_RVS;
#else
	if (data->ap_rev > 3)
		fw_revision = SSP_FIRMWARE_REVISION_STM;
	else
		fw_revision = SSP_FIRMWARE_REVISION_STM_88921;
#endif

	data->uCurFirmRev = get_firmware_rev(data);

	if ((data->uCurFirmRev == SSP_INVALID_REVISION) \
		|| (data->uCurFirmRev == SSP_INVALID_REVISION2)) {
#if STM_SHOULD_BE_IMPLEMENT
		data->client->addr = BOOTLOADER_SLAVE_ADDR;
		iRet = check_bootloader(data->client, BL_WAITING_BOOTLOAD_CMD);

		if (iRet >= 0) {
			pr_info("[SSP] ssp_load_fw_bootmode\n");

			return FW_DL_STATE_NEED_TO_SCHEDULE;
		} else {
			pr_warn("[SSP] Firm Rev is invalid(%8u). Retry.\n",
				data->uCurFirmRev);
			data->client->addr = APP_SLAVE_ADDR;
			data->uCurFirmRev = get_firmware_rev(data);
			if (data->uCurFirmRev == SSP_INVALID_REVISION ||\
				data->uCurFirmRev == ERROR) {
				pr_err("[SSP] MCU is not working, FW download failed\n");
				return FW_DL_STATE_FAIL;
			} else if (data->uCurFirmRev != fw_revision) {
				pr_info("[SSP] MCU Firm Rev : Old = %8u, New = %8u\n",
					data->uCurFirmRev, fw_revision);

				return FW_DL_STATE_NEED_TO_SCHEDULE;
			}
			pr_info("[SSP] MCU Firm Rev : Old = %8u, New = %8u\n",
				data->uCurFirmRev, fw_revision);
		}
#endif
		pr_err("[SSP] SSP_INVALID_REVISION\n");
		return FW_DL_STATE_NEED_TO_SCHEDULE;
	} else {
		if (data->uCurFirmRev != fw_revision) {
			pr_info("[SSP] MCU Firm Rev : Old = %8u, New = %8u\n",
				data->uCurFirmRev, fw_revision);

			return FW_DL_STATE_NEED_TO_SCHEDULE;
		}
		pr_info("[SSP] MCU Firm Rev : Old = %8u, New = %8u\n",
			data->uCurFirmRev, fw_revision);
	}

	return FW_DL_STATE_NONE;
}
