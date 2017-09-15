/*
 *  wacom_i2c_flash.c - Wacom G5 Digitizer Controller (I2C bus)
 *
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/wacom_i2c.h>
#include "wacom_i2c_flash.h"
#include <linux/uaccess.h>

#define ERR_HEX 0x056
#define RETRY_TRANSFER 5
#define ERR_RET	1
unsigned char checksum_data[4];

int wacom_i2c_select_flash_code(struct wacom_i2c *wac_i2c)
{
	int ret;

	dev_info(&wac_i2c->client->dev, "%s\n", __func__);

	if(wac_i2c->ic_mpu_ver < MPU_W9007)
		ret = wacom_i2c_flash(wac_i2c);
	else
		ret = wacom_i2c_flash_9007(wac_i2c);

	return ret;
}

int wacom_check_mpu_version(struct wacom_i2c *wac_i2c)
{
	u8 buf[2] = {0xE0, 0};
	int ret;
	ret = wacom_i2c_send(wac_i2c, &buf[0], 1, WACOM_I2C_MODE_BOOT);
	if (ret == -ETIMEDOUT)
		goto err_i2c_timeout;

	usleep(1*1000);
	ret = wacom_i2c_recv(wac_i2c,
				&buf[1], 1, WACOM_I2C_MODE_BOOT);
	if (ret > 0)
		wac_i2c->ic_mpu_ver = MPU_W9007;
	else
		wac_i2c->ic_mpu_ver = MPU_W9001;

	dev_info(&wac_i2c->client->dev,
			"%s: boot mode : %x\n", __func__, buf[1]);
	return buf[1];
err_i2c_timeout:
	return ret;
}

int calc_checksum(unsigned char *flash_data)
{
	unsigned long i;

	if (ums_binary)
		return 0;

	for (i = 0; i < 4; i++)
		checksum_data[i] = 0;

	for (i = 0x0000; i < 0xC000; i += 4) {
		checksum_data[0] ^= flash_data[i];
		checksum_data[1] ^= flash_data[i+1];
		checksum_data[2] ^= flash_data[i+2];
		checksum_data[3] ^= flash_data[i+3];
	}

	printk(KERN_DEBUG "wacom_%s: %02x, %02x, %02x, %02x\n",
			__func__, checksum_data[0], checksum_data[1],
			checksum_data[2], checksum_data[3]);

	for (i = 0; i < 4; i++) {
		if (checksum_data[i] != Firmware_checksum[i+1])
			return -ERR_HEX;
	}

	return 0;
}

int wacom_i2c_flash_chksum(struct wacom_i2c *wac_i2c, unsigned char *flash_data,
			   unsigned long *max_address)
{
	unsigned long i;
	unsigned long chksum = 0;

	for (i = 0x0000; i <= *max_address; i++)
		chksum += flash_data[i];

	chksum &= 0xFFFF;

	return (int)chksum;
}

int wacom_i2c_flash_cmd(struct wacom_i2c *wac_i2c)
{
	int ret, len, i;
	u8 buf[10], flashq;

	buf[0] = 0x0d;
	buf[1] = FLASH_START0;
	buf[2] = FLASH_START1;
	buf[3] = FLASH_START2;
	buf[4] = FLASH_START3;
	buf[5] = FLASH_START4;
	buf[6] = FLASH_START5;
	buf[7] = 0x0d;
	flashq = 0xE0;
	len = 1;

	ret = wacom_i2c_send(wac_i2c, &flashq, len, true);
	if (ret >= 0) {

		i = 0;
		do {
			usleep(1*1000);
			ret = wacom_i2c_recv(wac_i2c,
						&flashq, len, true);
			dev_err(&wac_i2c->client->dev,
					"%s: recv boot mode : %x\n", __func__, flashq);
			i++;

			if (i > RETRY)
				return -ERR_RET;
		} while (flashq == 0xff);
	} else {
		dev_err(&wac_i2c->client->dev, "%s: enter boot mode failed[%d], 1\n", __func__, ret);
		ret = wacom_i2c_send(wac_i2c, &flashq, len, true);
		if (ret < 0)
			dev_err(&wac_i2c->client->dev, "%s: enter boot mode failed[%d], 2\n", __func__, ret);	
		usleep(1*1000);
		len = 8;
		ret = wacom_i2c_send(wac_i2c, buf, len, false);
		if (ret < 0) {
			dev_err(&wac_i2c->client->dev,
					 "%s: Sending flash command failed\n",
					 __func__);
			return -ERR_RET;
		}
		dev_info(&wac_i2c->client->dev,
				 "%s: flash send?:%d\n", __func__, ret);
		msleep(270);
	}

	wac_i2c->boot_mode = true;

	return 0;
}

int wacom_i2c_flash_query(struct wacom_i2c *wac_i2c, u8 query, u8 recvdQuery)
{
	int ret, len, i;
	u8 flashq;

	flashq = query;
	len = 1;

	ret = wacom_i2c_send(wac_i2c, &flashq, len, true);
	if (ret < 0) {
		dev_err(&wac_i2c->client->dev,
				 "%s: query unsent:%d\n", __func__, ret);
		return -ERR_RET;
	}

	/*sleep*/
	usleep(10*1000);
	i = 0;
	do {
		usleep(1*1000);
		flashq = query;
		ret = wacom_i2c_recv(wac_i2c,
						&flashq, len, true);
		i++;

		if (i > RETRY)
			return -ERR_RET;
		dev_info(&wac_i2c->client->dev,
				 "%s: ret:%d flashq:%x\n", __func__, ret, flashq);
	} while (recvdQuery == 0xff && flashq != recvdQuery);

	dev_info(&wac_i2c->client->dev,
			 "%s: query:%x\n", __func__, flashq);

	return flashq;
}

int wacom_i2c_flash_end(struct wacom_i2c *wac_i2c)
{
	int ret;

	/* 2012/07/04 Evaluation for 0x80 and 0xA0 added by Wacom*/
	do {
		ret = wacom_i2c_flash_query(wac_i2c, FLASH_END, FLASH_END);
		if (ret == -1)
			return ERR_FAILED_EXIT;
	} while (ret != 0x80);	/*P130312-6278 (ret == 0xA0 || ret != 0x80);*/
	/* 2012/07/04 Evaluation for 0x80 and 0xA0 added by Wacom*/

	/*2012/07/05
	below added for giving firmware enough time to change to user mode*/
	msleep(1000);

	dev_info(&wac_i2c->client->dev,
			 "%s: Digitizer activated\n", __func__);
	wac_i2c->boot_mode = false;
	return 0;
}

int wacom_i2c_flash_enter(struct wacom_i2c *wac_i2c)
{
	if (wacom_i2c_flash_query(wac_i2c, FLASH_QUERY, FLASH_ACK) == -1)
		return ERR_NOT_FLASH;
	return 0;
}

int wacom_i2c_flash_BLVer(struct wacom_i2c *wac_i2c)
{
	int ret = 0;
	ret = wacom_i2c_flash_query(wac_i2c, FLASH_BLVER, 0x40);
	if (ret == -1)
		return ERR_UNSENT;

	return ret;
}

int wacom_i2c_flash_mcuId(struct wacom_i2c *wac_i2c)
{
	int ret = 0;

	ret = wacom_i2c_flash_query(wac_i2c, FLASH_MPU, 0x26);
	if (ret == -1)
		return ERR_UNSENT;

	return ret;
}

int wacom_i2c_flash_erase(struct wacom_i2c *wac_i2c, u8 cmd_erase,
			  u8 cmd_block, u8 endBlock)
{
	int len, ret, i, j;
	u8 buf[3], sum, block, flashq;
	unsigned long swtich_slot_time;

	ret = 0;

	for (i = cmd_block; i >= endBlock; i--) {
		block = i;
		block |= 0x80;

		sum = cmd_erase;
		sum += block;
		sum = ~sum + 1;

		buf[0] = cmd_erase;
		buf[1] = block;
		buf[2] = sum;

		len = 3;
		ret = wacom_i2c_send(wac_i2c, buf, len, true);
		if (ret < 0) {
			dev_err(&wac_i2c->client->dev,
					 "%s: Erase failed\n", __func__);
			return -ERR_RET;
		}

		len = 1;
		flashq = 0;
		j = 0;

		do {
			/*sleep */
			msleep(100);
			ret = wacom_i2c_recv(wac_i2c,
						&flashq, len, true);
			j++;

			if (j > RETRY || flashq == 0x84 || flashq == 0x88
			    || flashq == 0x8A || flashq == 0x90) {
				/*
				   0xff:No data
				   0x84:Erase failure
				   0x88:Erase time parameter error
				   0x8A:Write time parameter error
				   0x90:Checksum error
				 */
			dev_err(&wac_i2c->client->dev,
					 "%s: Error:%x\n", __func__, flashq);
				return -ERR_RET;
			}

		/* 2012/07/04 Evaluation if 0x06 or not added by Wacom*/
		} while (flashq != 0x06);
		/*P130312-6278 (flashq == 0xff || flashq != 0x06);*/

		if (printk_timed_ratelimit(&swtich_slot_time, 5000))
			dev_info(&wac_i2c->client->dev,
					 "%s: Erasing at %d, ", __func__, i);
		/*sleep */
		usleep(1*1000);
	}

	dev_info(&wac_i2c->client->dev,
			"%s: Erasing done\n", __func__);
	return ret;
}

int wacom_i2c_flash_write(struct wacom_i2c *wac_i2c, unsigned long startAddr,
			  u8 size, unsigned long maxAddr)
{
	unsigned long ulAddr;
	int ret, len, i, j, k;
	char sum;
	u8 buf[WRITE_BUFF], bank;
	unsigned long swtich_slot_time;

	ret = len = i = 0;
	bank = BANK;

	for (ulAddr = startAddr; ulAddr <= maxAddr; ulAddr += BLOCK_SIZE_W) {

		sum = 0;
		buf[0] = FLASH_WRITE;
		buf[1] = (u8) (ulAddr & 0xff);
		buf[2] = (u8) ((ulAddr & 0xff00) >> 8);
		buf[3] = size;
		buf[4] = bank;
		/*Pass Garbage*/
		for (i = 0; i < BLOCK_SIZE_W; i++) {
			if (Binary[ulAddr+i] != 0xff)
				break;
		}
		if (i == BLOCK_SIZE_W) {
			dev_err(&wac_i2c->client->dev,
					 "%s: Pass ulAddr %u\n",
					 __func__, (unsigned int)ulAddr);
			continue;
		}

		for (i = 0; i < 5; i++)
			sum += buf[i];

		sum = ~sum + 1;
		buf[5] = sum;

		len = 6;

		/* 2012/07/18
		* if the returned data is not equal to the length of the bytes
		* that is supposed to send/receive, return it as fail
		*/
		for (k = 0; k < RETRY_TRANSFER; k++) {
			ret = wacom_i2c_send(wac_i2c, buf, len, true);
			if (ret == len)
				break;
			if (ret < 0 || k == (RETRY_TRANSFER - 1)) {
			dev_err(&wac_i2c->client->dev,
					 "%s: Write process aborted\n",
					 __func__);
				return ERR_FAILED_ENTER;
			}
		}
		/*2012/07/18*/

		usleep(10*1000);
		len = 1;
		j = 0;
		do {
			usleep(5*1000);
			ret = wacom_i2c_recv(wac_i2c,
						buf, len, true);
			j++;

			if (j > RETRY || buf[0] == 0x90) {
				/*0xff:No data 0x90:Checksum error */
				dev_err(&wac_i2c->client->dev,
						 "%s: Error: %x , 0x%lx(%d)\n",
						__func__, buf[0], ulAddr, __LINE__);
				return -ERR_RET;
			}

		/* 2012/07/04 Evaluation if 0x06 or not added by Wacom*/
		} while (buf[0] == 0xff || buf[0] != 0x06);
		/* 2012/07/04 Evaluation if 0x06 or not added by Wacom*/

		usleep(1*1000);

		sum = 0;
		for (i = 0; i < BLOCK_SIZE_W; i++) {
			buf[i] = Binary[ulAddr + i];
			sum += Binary[ulAddr + i];
		}
		sum = ~sum + 1;
		buf[BLOCK_SIZE_W] = sum;
		len = BLOCK_SIZE_W + 1;

		/* 2012/07/18
		* if the returned data is not equal to the length of the bytes
		* that is supposed to send/receive, return it as fail
		*/
		for (k = 0; k < RETRY_TRANSFER; k++) {
			ret = wacom_i2c_send(wac_i2c, buf, len, true);
			if (ret == len)
				break;
			if (ret < 0 || k == (RETRY_TRANSFER - 1)) {
				dev_err(&wac_i2c->client->dev,
						 "%s: Write process aborted\n",
						 __func__);
				return ERR_FAILED_ENTER;
			}
		}
		/*2012/07/18*/

		msleep(50);
		len = 1;
		j = 0;

		do {
			usleep(10*1000);
			ret = wacom_i2c_recv(wac_i2c,
						buf, len, true);
			j++;

			if (j > RETRY || buf[0] == 0x82 || buf[0] == 0x90) {
				/*
				   0xff:No data
				   0x82:Write error
				   0x90:Checksum error
				 */
				dev_err(&wac_i2c->client->dev,
						 "%s: Error: %x , 0x%lx(%d)\n",
						__func__, buf[0], ulAddr, __LINE__);
				return -ERR_RET;
			}

		/* 2012/07/04 Evaluation if 0x06 or not added by Wacom*/
		} while (buf[0] == 0xff || buf[0] != 0x06);
		/* 2012/07/04 Evaluation if 0x06 or not added by Wacom*/

		if (printk_timed_ratelimit(&swtich_slot_time, 5000))
			dev_info(&wac_i2c->client->dev,
					 "%s: Written on:0x%lx",
					 __func__, ulAddr);
		usleep(1*1000);
	}

	dev_info(&wac_i2c->client->dev,
			 "%s: Writing done\n", __func__);

	return 0;
}

int wacom_i2c_flash_verify(struct wacom_i2c *wac_i2c, unsigned long startAddr,
			   u8 size, unsigned long maxAddr)
{
	unsigned long ulAddr;
	int ret, len, i, j, k;
	char sum;
	u8 buf[WRITE_BUFF], bank;
	unsigned long swtich_slot_time;

	ret = len = i = 0;
	bank = BANK;

	for (ulAddr = startAddr; ulAddr <= maxAddr; ulAddr += BLOCK_SIZE_W) {

		sum = 0;
		buf[0] = FLASH_VERIFY;
		buf[1] = (u8) (ulAddr & 0xff);
		buf[2] = (u8) ((ulAddr & 0xff00) >> 8);
		buf[3] = size;
		buf[4] = bank;

		for (i = 0; i < 5; i++)
			sum += buf[i];
		sum = ~sum + 1;
		buf[5] = sum;

		len = 6;
		j = 0;
		/*sleep */

		/* 2012/07/18
		* if the returned data is not equal to the length of the bytes
		* that is supposed to send/receive, return it as fail
		*/
		for (k = 0; k < RETRY_TRANSFER; k++) {
			ret = wacom_i2c_send(wac_i2c, buf, len, true);
			if (ret == len)
				break;
			if (ret < 0 || k == (RETRY_TRANSFER - 1)) {
				dev_err(&wac_i2c->client->dev,
						 "%s: Write process aborted\n",
						 __func__);
				return ERR_FAILED_ENTER;
			}
		}
		/*2012/07/18*/

		len = 1;

		do {
			usleep(1*1000);
			ret = wacom_i2c_recv(wac_i2c,
						buf, len, true);
			j++;
			if (j > RETRY || buf[0] == 0x90) {
				/*0xff:No data 0x90:Checksum error */
				dev_err(&wac_i2c->client->dev,
						 "%s: Error: %x , 0x%lx(%d)\n",
						__func__, buf[0], ulAddr, __LINE__);
				return -ERR_RET;
			}
		/* 2012/07/04 Evaluation if 0x06 or not added by Wacom*/
		} while (buf[0] == 0xff || buf[0] != 0x06);
		/* 2012/07/04 Evaluation if 0x06 or not added by Wacom*/

		usleep(1*1000);
		sum = 0;
		for (i = 0; i < BLOCK_SIZE_W; i++) {
			buf[i] = Binary[ulAddr + i];
			sum += Binary[ulAddr + i];
		}
		sum = ~sum + 1;
		buf[BLOCK_SIZE_W] = sum;
		len = BLOCK_SIZE_W + 1;

		/* 2012/07/18
		* if the returned data is not equal to the length of the bytes
		* that is supposed to send/receive, return it as fail
		*/
		for (k = 0; k < RETRY_TRANSFER; k++) {
			ret = wacom_i2c_send(wac_i2c, buf, len, true);
			if (ret == len)
				break;
			if (ret < 0 || k == (RETRY_TRANSFER - 1)) {
			dev_err(&wac_i2c->client->dev,
					 "%s: Write process aborted\n",
					 __func__);
				return ERR_FAILED_ENTER;
			}
		}
		/*2012/07/18*/

		usleep(3*1000);
		/* 2013/04/30 margin for HLTE firmware verification */
		len = 1;
		j = 0;
		do {
			usleep(2*1000);
			ret = wacom_i2c_recv(wac_i2c,
						buf, len, true);
			j++;

			if (j > RETRY || buf[0] == 0x81 || buf[0] == 0x90) {
				/*
				   0xff:No data
				   0x82:Write error
				   0x90:Checksum error
				 */
				dev_err(&wac_i2c->client->dev,
						 "%s: Error: %x , 0x%lx(%d)\n",
						__func__, buf[0], ulAddr, __LINE__);
				return -ERR_RET;
			}

		/* 2012/07/04 Evaluation if 0x06 or not added by Wacom*/
		} while (buf[0] == 0xff || buf[0] != 0x06);
		/* 2012/07/04 Evaluation if 0x06 or not added by Wacom*/

		if (printk_timed_ratelimit(&swtich_slot_time, 5000))
			dev_info(&wac_i2c->client->dev,
					 "%s: Verified:0x%lx", __func__, ulAddr);
		usleep(1*1000);
	}

	dev_info(&wac_i2c->client->dev,
			 "%s: Verifying done\n", __func__);

	return 0;
}

int wacom_i2c_flash(struct wacom_i2c *wac_i2c)
{
	bool fw_update_enable = false;
	int ret = 0, blver = 0, mcu = 0;
	u32 max_addr = 0, cmd_addr = 0;

	if (system_rev >= WACOM_FW_UPDATE_REVISION) {
		dev_info(&wac_i2c->client->dev,
				"%s: run FW update..\n",
				__func__);
	} else {
		dev_info(&wac_i2c->client->dev,
				"%s: do not update..\n",
				__func__);
		return -ERR_RET;
	}

#ifdef WACOM_HAVE_FWE_PIN
	wac_i2c->wac_pdata->compulsory_flash_mode(wac_i2c, true);
	/*Reset*/
	wac_i2c->wac_pdata->reset_platform_hw(wac_i2c);
	msleep(200);
	dev_err(&wac_i2c->client->dev, "%s: Set FWE\n", __func__);
#endif

	ret = wacom_i2c_flash_cmd(wac_i2c);
	if (ret < 0)
		goto fw_update_error;
	usleep(10*1000);

	ret = wacom_i2c_flash_enter(wac_i2c);
	dev_info(&wac_i2c->client->dev,
			 "%s: flashEnter:%d\n", __func__, ret);
	usleep(10*1000);

	blver = wacom_i2c_flash_BLVer(wac_i2c);
	dev_info(&wac_i2c->client->dev,
			 "%s: blver:%d\n", __func__, blver);
	usleep(10*1000);

	mcu = wacom_i2c_flash_mcuId(wac_i2c);
	dev_info(&wac_i2c->client->dev,
			 "%s: mcu:%x\n", __func__, mcu);
	if (MPU_W9001 != mcu) {
		dev_info(&wac_i2c->client->dev,
				 "%s: mcu:%x\n", __func__, mcu);
		ret = -ENXIO;
		goto mcu_type_error;
	}
	usleep(1*1000);

	switch (mcu) {
	case MPUVER_W8501:
		dev_info(&wac_i2c->client->dev,
				 "%s: flashing for w8501 started\n",
				 __func__);
		max_addr = MAX_ADDR_W8501;
		cmd_addr = MAX_BLOCK_W8501;
		fw_update_enable = true;
		break;

	case MPUVER_514:
		dev_info(&wac_i2c->client->dev,
				 "%s: Flashing for 514 started\n",
				 __func__);
		max_addr = MAX_ADDR_514;
		cmd_addr = MAX_BLOCK_514;
		fw_update_enable = true;
		break;

	case MPUVER_505:
		dev_info(&wac_i2c->client->dev,
				 "%s: Flashing for 505 started\n",
				 __func__);
		max_addr = MAX_ADDR_505;
		cmd_addr = MAX_BLOCK_505;
		fw_update_enable = true;
		break;

	default:
		dev_info(&wac_i2c->client->dev,
				 "%s: default called\n",
				 __func__);
		break;
	}

	if (fw_update_enable) {
		bool valid_hex = false;
		int cnt = 0;
		/*2012/07/04: below modified by Wacom*/
		/*If wacom_i2c_flash_verify returns -ERR_HEX, */
		/*please redo whole process of flashing from  */
		/*wacom_i2c_flash_erase                       */
		do {
			ret = wacom_i2c_flash_erase(wac_i2c, FLASH_ERASE,
				    cmd_addr, END_BLOCK);
			if (ret < 0) {
				printk(KERN_ERR "[E-PEN] error - erase\n");
				continue;
			}
			msleep(20);

			ret = wacom_i2c_flash_write(wac_i2c, START_ADDR,
				    NUM_BLOCK_2WRITE, max_addr);
			if (ret < 0) {
				dev_err(&wac_i2c->client->dev,
						 "%s: error - writing\n",
						 __func__);
				continue;
			}
			msleep(20);

			ret = wacom_i2c_flash_verify(wac_i2c, START_ADDR,
				     NUM_BLOCK_2WRITE, max_addr);
			if (ret == -ERR_HEX)
				dev_err(&wac_i2c->client->dev,
						 "%s: firmware is not valied\n",
						 __func__);
			else if (ret < 0) {
				dev_info(&wac_i2c->client->dev,
						 "%s: error - verifying\n",
						 __func__);
				continue;
			} else
				valid_hex = true;
		} while ((!valid_hex) && (cnt < 10));
		/*2012/07/04: Wacom*/

	dev_info(&wac_i2c->client->dev,
			 "%s: Firmware successfully updated\n",
			 __func__);
	}
	usleep(1*1000);

mcu_type_error:
	wacom_i2c_flash_end(wac_i2c);
	if (ret < 0)
		dev_err(&wac_i2c->client->dev,
				 "%s: error - wacom_i2c_flash_end\n",
				 __func__);

fw_update_error:
#ifdef WACOM_HAVE_FWE_PIN
	wac_i2c->wac_pdata->compulsory_flash_mode(wac_i2c, false);
	/*Reset*/
	wac_i2c->wac_pdata->reset_platform_hw(wac_i2c);
	msleep(200);
	dev_info(&wac_i2c->client->dev,
			 "%s: Release FWE\n",
			 __func__);
#endif

	return ret;
}

static int wacom_enter_flash_mode(struct wacom_i2c *wac_i2c)
{
	int ret, len = 0;
	char buf[8];

	dev_info(&wac_i2c->client->dev, "%s\n", __func__);

	memset(buf, 0, 8);
	strncpy(buf, "\rflash\r", 7);

	len = sizeof(buf) / sizeof(buf[0]);

	ret = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (ret < 0) {
		dev_err(&wac_i2c->client->dev,
				"%s: Sending flash command failed, %d\n",
				__func__, ret);
		return ret;
	}

	msleep(500);

	return 0;
}

static int wacom_set_cmd_feature(struct wacom_i2c *wac_i2c)
{
	int ret, len = 0;
	u8 buf[4];

	buf[len++] = 4;
	buf[len++] = 0;
	buf[len++] = 0x37;
	buf[len++] = CMD_SET_FEATURE;

	ret = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (ret < 0)
		dev_err(&wac_i2c->client->dev,
				"%s: failed send CMD_SET_FEATURE, %d\n",
				__func__, ret);

	return ret;
}

static int wacom_get_cmd_feature(struct wacom_i2c *wac_i2c)
{
	int ret, len = 0;
	u8 buf[4];

	buf[len++] = 4;
	buf[len++] = 0;
	buf[len++] = 0x38;
	buf[len++] = CMD_GET_FEATURE;

	ret = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (ret < 0)
		dev_err(&wac_i2c->client->dev,
				"%s: failed send CMD_GET_FEATURE, ret: %d\n",
				__func__, ret);

	len = 0;
	buf[len++] = 5;
	buf[len++] = 0;

	ret = wacom_i2c_send(wac_i2c, buf, len, WACOM_I2C_MODE_BOOT);
	if (ret < 0)
		dev_err(&wac_i2c->client->dev,
				"%s: failed send command, ret: %d\n",
				__func__, ret);

	return ret;
}

/*
 * mode1. BOOT_QUERY: check enter boot mode for flash.
 * mode2. BOOT_BLVER : check bootloader version.
 * mode3. BOOT_MPU : check MPU version
 */
int wacom_check_flash_mode(struct wacom_i2c *wac_i2c, int mode)
{
	int ret, ECH;
	unsigned char response_cmd = 0;
	unsigned char command[CMD_SIZE];
	unsigned char response[RSP_SIZE];

	switch (mode) {
	case BOOT_QUERY:
		response_cmd = QUERY_CMD;
		break;
	case BOOT_BLVER:
		response_cmd = BOOT_CMD;
		break;
	case BOOT_MPU:
		response_cmd = MPU_CMD;
		break;
	default :
		break;
	}

	dev_info(&wac_i2c->client->dev, "%s, mode = %s\n", __func__,
			(mode == BOOT_QUERY) ? "BOOT_QUERY" :
			(mode == BOOT_BLVER) ? "BOOT_BLVER" :
			(mode == BOOT_MPU) ? "BOOT_MPU" : "Not Support");

	ret = wacom_set_cmd_feature(wac_i2c);
	if (ret < 0)
		return ret;

	command[0] = 5;
	command[1] = 0;
	command[2] = 5;
	command[3] = 0;
	command[4] = BOOT_CMD_REPORT_ID;
	command[5] = mode;
	command[6] = ECH = 7;

	ret = wacom_i2c_send(wac_i2c, command, 7, WACOM_I2C_MODE_BOOT);
	if (ret < 0) {
		dev_err(&wac_i2c->client->dev,
				"%s: failed send REPORT_ID, %d\n",
				__func__, ret);
		return ret;
	}

	ret = wacom_get_cmd_feature(wac_i2c);
	if (ret < 0)
		return ret;

	usleep_range(10000, 10000);

	ret = wacom_i2c_recv(wac_i2c, response, BOOT_RSP_SIZE,
			    WACOM_I2C_MODE_BOOT);
	if (ret < 0) {
		dev_err(&wac_i2c->client->dev,
				"%s: failed receive response, %d\n",
				__func__, ret);
		return ret;
	}

	if ((response[3] != response_cmd) || (response[4] != ECH)) {
		dev_err(&wac_i2c->client->dev,
				"%s: failed matching response3[%x], 4[%x]\n",
				__func__, response[3], response[4]);
		return -1;
	}

	return response[5];

}

int wacom_enter_bootloader(struct wacom_i2c *wac_i2c)
{
	int ret;
	int retry = 0;

	dev_info(&wac_i2c->client->dev, "%s\n", __func__);

	ret = wacom_enter_flash_mode(wac_i2c);
	if (ret < 0)
		msleep(500);

	do {
		msleep(100);
		ret = wacom_check_flash_mode(wac_i2c, BOOT_QUERY);
		retry++;
	} while (ret < 0 && retry < 10);

	if (ret < 0)
		return EXIT_FAIL_GET_BOOT_LOADER_VERSION;

	ret = wacom_check_flash_mode(wac_i2c, BOOT_BLVER);
	wac_i2c->boot_ver = ret;
	if (ret < 0)
		return EXIT_FAIL_GET_BOOT_LOADER_VERSION;

	return ret;
}

static int wacom_flash_end(struct wacom_i2c *wac_i2c)
{
	int ret, ECH;
	unsigned char command[CMD_SIZE];

	dev_info(&wac_i2c->client->dev, "%s\n", __func__);

	command[0] = 4;
	command[1] = 0;
	command[2] = 0x37;
	command[3] = CMD_SET_FEATURE;
	command[4] = 5;
	command[5] = 0;
	command[6] = 5;
	command[7] = 0;
	command[8] = BOOT_CMD_REPORT_ID;
	command[9] = BOOT_EXIT;
	command[10] = ECH = 7;

	ret = wacom_i2c_send(wac_i2c, command, 11, WACOM_I2C_MODE_BOOT);
	if (ret < 0)
		dev_err(&wac_i2c->client->dev,
				"%s failed, %d\n",
				__func__, ret);
	return ret;
}

static int wacom_flash_erase(struct wacom_i2c *wac_i2c,
			int *eraseBlock, int num)
{
	int ret = 0, ECH;
	unsigned char sum;
	unsigned char cmd_chksum;
	int i, j;
	unsigned char command[CMD_SIZE];
	unsigned char response[RSP_SIZE];

	dev_info(&wac_i2c->client->dev, "%s\n", __func__);

	for (i = 0; i < num; i++) {
		/*msleep(500);*/
retry:
		ret = wacom_set_cmd_feature(wac_i2c);
		if (ret < 0) {
			dev_info(&wac_i2c->client->dev,
				"%s: set command failed, retry= %d\n",
				__func__, i);
			return ret;
		}

		command[0] = 5;
		command[1] = 0;
		command[2] = 7;
		command[3] = 0;
		command[4] = BOOT_CMD_REPORT_ID;
		command[5] = BOOT_ERASE_FLASH;
		command[6] = ECH = i;
		command[7] = *eraseBlock;
		eraseBlock++;

		sum = 0;
		for (j = 0; j < 8; j++)
			sum += command[j];
		cmd_chksum = ~sum + 1;
		command[8] = cmd_chksum;

		ret = wacom_i2c_send(wac_i2c, command, 9, WACOM_I2C_MODE_BOOT);
		if (ret < 0) {
			dev_err(&wac_i2c->client->dev,
					"%s: failed send erase command, %d\n",
					__func__, ret);
			return ret;
		}

		msleep(300);

		ret = wacom_get_cmd_feature(wac_i2c);
		if (ret < 0) {
			dev_info(&wac_i2c->client->dev,
				"%s: get command failed, retry= %d\n",
				__func__, i);
			return ret;
		}

		ret = wacom_i2c_recv(wac_i2c, response, BOOT_RSP_SIZE,
				    WACOM_I2C_MODE_BOOT);
		if (ret < 0) {
			dev_err(&wac_i2c->client->dev,
					"%s: failed receive response, %d\n",
					__func__, ret);
			return ret;
		}

		if ((response[3] != ERS_CMD) || (response[4] != ECH)) {
			dev_err(&wac_i2c->client->dev,
					"%s: failed matching response3[%x], 4[%x]\n",
					__func__, response[3], response[4]);
			return -1;
		}

		if (response[5] == 0x80) {
			dev_err(&wac_i2c->client->dev,
					"%s: erase retry\n",
					__func__);
			goto retry;
		}

		if (response[5] != ACK) {
			dev_err(&wac_i2c->client->dev,
					"%s: failed erase. response[%x], retry[%d]\n",
					__func__, response[5], i);
			return -1;
		}
	}
	return ret;
}

static int wacom_flash_write_block(struct wacom_i2c *wac_i2c, const unsigned char *flash_data,
			      unsigned long ulAddress, u8 *pcommand_id)
{
	const int MAX_COM_SIZE = (12 + FLASH_BLOCK_SIZE + 2);
	int ECH;
	int ret;
	unsigned char sum;
	unsigned char command[MAX_COM_SIZE];
	unsigned char response[RSP_SIZE];
	unsigned int i;

	ret = wacom_set_cmd_feature(wac_i2c);
	if (ret < 0)
		return ret;

	command[0] = 5;
	command[1] = 0;
	command[2] = 76;
	command[3] = 0;
	command[4] = BOOT_CMD_REPORT_ID;
	command[5] = BOOT_WRITE_FLASH;
	command[6] = ECH = ++(*pcommand_id);
	command[7] = ulAddress & 0x000000ff;
	command[8] = (ulAddress & 0x0000ff00) >> 8;
	command[9] = (ulAddress & 0x00ff0000) >> 16;
	command[10] = (ulAddress & 0xff000000) >> 24;
	command[11] = 8;
	sum = 0;
	for (i = 0; i < 12; i++)
		sum += command[i];
	command[MAX_COM_SIZE - 2] = ~sum + 1;

	sum = 0;
	for (i = 12; i < (FLASH_BLOCK_SIZE + 12); i++) {
		command[i] = flash_data[ulAddress + (i - 12)];
		sum += flash_data[ulAddress + (i - 12)];
	}
	command[MAX_COM_SIZE - 1] = ~sum + 1;

	ret = wacom_i2c_send(wac_i2c, command, BOOT_CMD_SIZE,
			    WACOM_I2C_MODE_BOOT);
	if (ret < 0) {
		printk(KERN_DEBUG "epen:1 rv:%d\n", ret);
		return ret;
	}

	usleep_range(10000, 10000);

	ret = wacom_get_cmd_feature(wac_i2c);
	if (ret < 0)
		return ret;

	ret = wacom_i2c_recv(wac_i2c, response, BOOT_RSP_SIZE,
			    WACOM_I2C_MODE_BOOT);
	if (ret < 0) {
		dev_err(&wac_i2c->client->dev,
				"%s: failed receive response, %d\n",
				__func__, ret);
		return ret;
	}

	if ((response[3] != WRITE_CMD) ||
	    (response[4] != ECH) || response[5] != ACK)
		return -1;

	return 0;

}

static int wacom_flash_write(struct wacom_i2c *wac_i2c,
			const unsigned char *flash_data, size_t data_size,
			unsigned long start_address, unsigned long *max_address)
{
	unsigned long ulAddress;
	bool ret;
	int i;
	unsigned long pageNo = 0;
	u8 command_id = 0;

	dev_info(&wac_i2c->client->dev, "%s\n", __func__);

	for (ulAddress = start_address; ulAddress < *max_address;
	     ulAddress += FLASH_BLOCK_SIZE) {
		unsigned int j;
		bool bWrite = false;

		/* Wacom 2012/10/04: skip if all each data locating on
			from ulAddr to ulAddr+Block_SIZE_W are 0xff */
		for (i = 0; i < FLASH_BLOCK_SIZE; i++) {
			if (flash_data[ulAddress + i] != 0xFF)
				break;
		}
		if (i == (FLASH_BLOCK_SIZE)) {
			/*printk(KERN_DEBUG"epen:BLOCK PASSED\n"); */
			continue;
		}
		/* Wacom 2012/10/04 */

		for (j = 0; j < FLASH_BLOCK_SIZE; j++) {
			if (flash_data[ulAddress + j] == 0xFF)
				continue;
			else {
				bWrite = true;
				break;
			}
		}

		if (!bWrite) {
			pageNo++;
			continue;
		}

		ret = wacom_flash_write_block(wac_i2c, flash_data, ulAddress,
				       &command_id);
		if (ret < 0)
			return ret;

		pageNo++;
	}

	return 0;
}

int wacom_i2c_flash_9007(struct wacom_i2c *wac_i2c)
{
	unsigned long max_address = 0;
	unsigned long start_address = START_ADDR;
	int i, ret = 0;
	int eraseBlock[100], eraseBlockNum;
	unsigned long ulMaxRange;

	if (wac_i2c->ic_mpu_ver != MPU_W9007 && wac_i2c->ic_mpu_ver != MPU_W9010)
		return EXIT_FAIL_GET_MPU_TYPE;

	wac_i2c->wac_pdata->compulsory_flash_mode(wac_i2c, true);
	/*Reset*/
	wac_i2c->wac_pdata->reset_platform_hw(wac_i2c);
	msleep(200);
	dev_err(&wac_i2c->client->dev, "%s: Set FWE\n", __func__);

	/*Set start and end address and block numbers*/
	eraseBlockNum = 0;
	start_address = START_ADDR_W9007;
	max_address = MAX_ADDR_W9007;

	for (i = BLOCK_NUM_W9007; i >= 8; i--) {
		eraseBlock[eraseBlockNum] = i;
		eraseBlockNum++;
	}

	/*Set adress range */
	ulMaxRange = max_address;
	ulMaxRange -= start_address;
	ulMaxRange >>= 6;
	if (max_address > (ulMaxRange << 6))
		ulMaxRange++;

	/*Erase the old program */
	ret = wacom_flash_erase(wac_i2c, eraseBlock, eraseBlockNum);
	if (ret < 0) {
		dev_err(&wac_i2c->client->dev,
				"%s: failed erase old firmware, %d\n",
				__func__, ret);
		goto flashing_fw_err;
	}

	/*Write the new program */
	ret =
	    wacom_flash_write(wac_i2c, Binary, DATA_SIZE, start_address, &max_address);
	if (ret < 0) {
		dev_err(&wac_i2c->client->dev,
				"%s: failed writing new firmware, %d\n",
				__func__, ret);
		goto flashing_fw_err;
	}

	/*Enable */
	ret = wacom_flash_end(wac_i2c);
	if (ret < 0) {
		dev_err(&wac_i2c->client->dev,
				"%s: failed flash mode close, %d\n",
				__func__, ret);
		goto flashing_fw_err;
	}

	dev_err(&wac_i2c->client->dev,
				"%s: Successed new Firmware writing\n",
				__func__);

flashing_fw_err:
	wac_i2c->wac_pdata->compulsory_flash_mode(wac_i2c, false);
	/*Reset */
	wac_i2c->wac_pdata->reset_platform_hw(wac_i2c);
	msleep(200);

	return ret;
}

int wacom_fw_load_from_UMS(struct wacom_i2c *wac_i2c)
	{
	struct file *fp;
	mm_segment_t old_fs;
	long fsize, nread;
	int ret = 0;
	const struct firmware *firm_data = NULL;

	dev_info(&wac_i2c->client->dev,
			"%s: Start firmware flashing (UMS).\n", __func__);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(WACOM_FW_PATH, O_RDONLY, S_IRUSR);

	if (IS_ERR(fp)) {
		dev_err(&wac_i2c->client->dev,
			"%s: failed to open %s.\n",
			__func__, WACOM_FW_PATH);
		ret = -ENOENT;
		goto open_err;
	}

	fsize= fp->f_path.dentry->d_inode->i_size;
	dev_info(&wac_i2c->client->dev,
		"%s: start, file path %s, size %ld Bytes\n",
		__func__, WACOM_FW_PATH, fsize);

	firm_data = kzalloc(fsize, GFP_KERNEL);
	if (IS_ERR(firm_data)) {
		dev_err(&wac_i2c->client->dev,
			"%s, kmalloc failed\n", __func__);
			ret = -EFAULT;
		goto malloc_error;
	}

	nread = vfs_read(fp, (char __user *)firm_data,
		fsize, &fp->f_pos);

	dev_info(&wac_i2c->client->dev,
		"%s: nread %ld Bytes\n", __func__, nread);
	if (nread != fsize) {
		dev_err(&wac_i2c->client->dev,
			"%s: failed to read firmware file, nread %ld Bytes\n",
			__func__, nread);
		ret = -EIO;
		goto read_err;
	}

	filp_close(fp, current->files);
	set_fs(old_fs);
	/*start firm update*/
	wacom_i2c_set_firm_data((unsigned char *)firm_data);

	return 0;

	read_err:		
		kfree(firm_data);
	malloc_error:
		filp_close(fp, current->files);
	open_err:
		set_fs(old_fs);   
		return ret;
	}

int wacom_load_fw_from_req_fw(struct wacom_i2c *wac_i2c)
{
	int ret = 0;
	const struct firmware *firm_data = NULL;
	char fw_path[WACOM_MAX_FW_PATH];

	dev_info(&wac_i2c->client->dev, "%s\n", __func__);

	/*Obtain MPU type: this can be manually done in user space */
	dev_info(&wac_i2c->client->dev,
			"%s: MPU type: %x , BOOT ver: %x\n",
			__func__, wac_i2c->ic_mpu_ver, wac_i2c->boot_ver);

	memset(&fw_path, 0, WACOM_MAX_FW_PATH);
	if (wac_i2c->ic_mpu_ver == MPU_W9001) {
		snprintf(fw_path, WACOM_MAX_FW_PATH,
			"%s", WACOM_FW_NAME_W9001);
	} else if (wac_i2c->ic_mpu_ver == MPU_W9007) {
		if (wac_i2c->boot_ver == 0x91)
			snprintf(fw_path, WACOM_MAX_FW_PATH,
				"%s", WACOM_FW_NAME_W9007_BL91);
		else if (wac_i2c->boot_ver == 0x92)
			snprintf(fw_path, WACOM_MAX_FW_PATH,
				"%s", WACOM_FW_NAME_W9007_BL92);
	} else if (wac_i2c->ic_mpu_ver == MPU_W9010) {
		if (system_rev >= WACOM_FW_UPDATE_REVISION) {
			snprintf(fw_path, WACOM_MAX_FW_PATH,
				"%s", WACOM_FW_NAME_W9010);
		} else {
			dev_info(&wac_i2c->client->dev,
				"%s: B934 PANEL, firmware name is NULL. return -1\n",
				__func__);
			ret = -1;
			goto firm_name_null_err;
		}

	} else {
		dev_info(&wac_i2c->client->dev,
			"%s: Not apply WACOM IC, firmware name is NULL. return -1\n",
			__func__);
		ret = -1;
		goto firm_name_null_err;
	}

	ret = request_firmware(&firm_data, fw_path, &wac_i2c->client->dev);
	if (ret < 0) {
		dev_err(&wac_i2c->client->dev,
		       "%s: Unable to open firmware. ret %d\n",
				__func__, ret);
		goto request_firm_err;
	}

	dev_info(&wac_i2c->client->dev, "%s: firmware name: %s, size: %d\n",
			__func__, fw_path, firm_data->size);

	/* firmware version check */
	if (wac_i2c->ic_mpu_ver == MPU_W9010 || wac_i2c->ic_mpu_ver == MPU_W9007)
		wac_i2c->wac_feature->fw_version = 
			firm_data->data[FIRM_VER_LB_ADDR_W9007] |(firm_data->data[FIRM_VER_UB_ADDR_W9007] << 8);
			
	dev_info(&wac_i2c->client->dev, "%s: firmware version = %x\n",
			__func__, wac_i2c->wac_feature->fw_version);
	wacom_i2c_set_firm_data((unsigned char *)firm_data->data);
	release_firmware(firm_data);

	firm_name_null_err:
	request_firm_err:
		return ret;
}

