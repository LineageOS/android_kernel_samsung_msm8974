/*
 * drivers/input/touchscreen/mms100_isc.c - ISC(In-system programming via I2C) enalbes MMS-100 Series sensor to be programmed while installed in a complete system.
 *
 * Copyright (C) 2012 Melfas, Inc.
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
#include <asm/unaligned.h>
#include "mms100_cfg_update.h"

/*
 *      Default configuration of ISC mode
 */
#define DEFAULT_SLAVE_ADDR                      	0x48

#define SECTION_NUM                           		3
#define SECTION_NAME_LEN                	        5

#define PAGE_HEADER                         		3
#define PAGE_DATA                              		1024
#define PAGE_TAIL                            		2
#define PACKET_SIZE                           		(PAGE_HEADER + PAGE_DATA + PAGE_TAIL)

#define TIMEOUT_CNT                          		10

#define TS_WRITE_REGS_LEN	PACKET_SIZE
/*
 * State Registers
 */

#define MIP_ADDR_INPUT_INFORMATION           		 0x01

#define ISC_ADDR_VERSION							 0xE1
#define ISC_ADDR_SECTION_PAGE_INFO					 0xE5

/*
 * Config Update Commands
 */
#define ISC_CMD_ENTER_ISC							0x5F
#define ISC_CMD_ENTER_ISC_PARA1					0x01
#define ISC_CMD_UPDATE_MODE						0xAE
#define ISC_SUBCMD_ENTER_UPDATE					0x55
#define ISC_SUBCMD_DATA_WRITE						0XF1
#define ISC_SUBCMD_LEAVE_UPDATE_PARA1			0x0F
#define ISC_SUBCMD_LEAVE_UPDATE_PARA2			0xF0
#define ISC_CMD_CONFIRM_STATUS					0xAF

#define ISC_STATUS_UPDATE_MODE					0x01
#define ISC_STATUS_CRC_CHECK_SUCCESS				0x03

#define ISC_CHAR_2_BCD(num)	\
	(((num/10)<<4) + (num%10))
#define ISC_MAX(x, y)		( ((x) > (y))? (x) : (y) )

#define MMS_BUILTIN_FW_NAME				"tsp_melfas/victor/victor.fw"
#define MMS_SDCARD_FW_NAME				"/sdcard/melfas.fw"

typedef enum {
	EC_NONE = -1,
	EC_DEPRECATED = 0,
	EC_BOOTLOADER_RUNNING = 1,
	EC_BOOT_ON_SUCCEEDED = 2,
	EC_ERASE_END_MARKER_ON_SLAVE_FINISHED = 3,
	EC_SLAVE_DOWNLOAD_STARTS = 4,
	EC_SLAVE_DOWNLOAD_FINISHED = 5,
	EC_2CHIP_HANDSHAKE_FAILED = 0x0E,
	EC_ESD_PATTERN_CHECKED = 0x0F,
	EC_LIMIT
} eErrCode_t;

typedef enum {
	SEC_NONE = -1, SEC_BOOTLOADER = 0, SEC_CORE, SEC_CONFIG, SEC_LIMIT
} eSectionType_t;

typedef struct {
	unsigned char version;
	unsigned char compatible_version;
	unsigned char start_addr;
	unsigned char end_addr;
	int bin_offset;
	u32 crc;
} tISCFWInfo_t;


static const char section_name[SECTION_NUM][SECTION_NAME_LEN] = { "BOOT",
		"CORE", "CONF" };

static const unsigned char crc0_buf[31] = { 0x1D, 0x2C, 0x05, 0x34, 0x95, 0xA4,
		0x8D, 0xBC, 0x59, 0x68, 0x41, 0x70, 0xD1, 0xE0, 0xC9, 0xF8, 0x3F, 0x0E,
		0x27, 0x16, 0xB7, 0x86, 0xAF, 0x9E, 0x7B, 0x4A, 0x63, 0x52, 0xF3, 0xC2,
		0xEB };

static const unsigned char crc1_buf[31] = { 0x1E, 0x9C, 0xDF, 0x5D, 0x76, 0xF4,
		0xB7, 0x35, 0x2A, 0xA8, 0xEB, 0x69, 0x42, 0xC0, 0x83, 0x01, 0x04, 0x86,
		0xC5, 0x47, 0x6C, 0xEE, 0xAD, 0x2F, 0x30, 0xB2, 0xF1, 0x73, 0x58, 0xDA,
		0x99 };

struct mms_bin_hdr {
	char	tag[8];
	u16	core_version;
	u16	section_num;
	u16	contains_full_binary;
	u16	reserved0;

	u32	binary_offset;
	u32	binary_length;

	u32	extention_offset;	
	u32	reserved1;
	
} __attribute__ ((packed));

struct mms_fw_img {
	u16	type;
	u16	version;

	u16	start_page;
	u16	end_page;

	u32	offset;
	u32	length;

} __attribute__ ((packed));

static tISCFWInfo_t mfsb_info[SECTION_NUM];
static tISCFWInfo_t ts_info[SECTION_NUM];
static bool section_update_flag[SECTION_NUM];

//const struct firmware *fw_mfsb[SECTION_NUM];

static struct firmware *fw_mfsb;
static struct mms_bin_hdr *fw_hdr;
static struct mms_fw_img **img;

static unsigned char g_wr_buf[1024 + 3 + 2];

static int mms100_i2c_read(struct i2c_client *client, u16 addr, u16 length,
		u8 *value) {
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg;
	int ret = -1;

	msg.addr = client->addr;
	msg.flags = 0x00;
	msg.len = 1;
	msg.buf = (u8 *) &addr;

	ret = i2c_transfer(adapter, &msg, 1);

	if (ret >= 0) {
		msg.addr = client->addr;
		msg.flags = I2C_M_RD;
		msg.len = length;
		msg.buf = (u8 *) value;

		ret = i2c_transfer(adapter, &msg, 1);
	}

	if (ret < 0) {
		pr_err("[TSP] : read error : [%d]", ret);
	}

	return ret;
}
/*
static int mms100_i2c_write(struct i2c_client *client, char *buf, int length) {
	int i;
	char data[TS_WRITE_REGS_LEN];

	if (length > TS_WRITE_REGS_LEN) {
		pr_err("[TSP] %s :size error \n", __FUNCTION__);
		return -EINVAL;
	}

	for (i = 0; i < length; i++)
		data[i] = *buf++;

	i = i2c_master_send(client, (char *) data, length);

	if (i == length)
		return length;
	else {
		pr_err("[TSP] :write error : [%d]", i);
		return -EIO;
	}
}
*/

extern int melfas_power(struct i2c_client *client,bool onoff);

static eISCRet_t mms100_reset(struct i2c_client *_client) {
	pr_info("[TSP ISC] %s\n", __func__);

	melfas_power(_client,0);
	mms100_msdelay(20);
	melfas_power(_client,1);
	mms100_msdelay(100);

	return ISC_SUCCESS;
}

static eISCRet_t mms100_check_operating_mode(struct i2c_client *_client,
		const eErrCode_t _error_code) {
	int ret;
	unsigned char rd_buf = 0x00;

	pr_info("[TSP ISC] %s\n", __func__);

	// Config version을 읽어서 booting 확인...
	ret = mms100_i2c_read(_client, ISC_ADDR_VERSION, 1, &rd_buf);

	if (ret < 0) {
		pr_info("[TSP ISC] %s,%d: i2c read fail[%d] \n", __FUNCTION__, __LINE__,
				ret);
		return _error_code;
	}

	pr_info("End mms100_check_operating_mode()\n");

	return ISC_SUCCESS;
}

static eISCRet_t mms100_get_version_info(struct i2c_client *_client) {
	int i, ret;
	unsigned char rd_buf[8];

	pr_info("[TSP ISC] %s\n", __func__);

	// config version brust read (core, private, public)
	ret = mms100_i2c_read(_client, ISC_ADDR_VERSION, SECTION_NUM, rd_buf);

	if (ret < 0) {
		pr_info("[TSP ISC] %s,%d: i2c read fail[%d] \n", __FUNCTION__, __LINE__,
				ret);
		return ISC_I2C_ERROR;
	}

	for (i = 0; i < SECTION_NUM; i++)
		ts_info[i].version = rd_buf[i];

	ts_info[SEC_CORE].compatible_version = ts_info[SEC_BOOTLOADER].version;
	ts_info[SEC_CONFIG].compatible_version = ts_info[SEC_CORE].version;

	ret = mms100_i2c_read(_client, ISC_ADDR_SECTION_PAGE_INFO, 8, rd_buf);

	if (ret < 0) {
		pr_info("[TSP ISC] %s,%d: i2c read fail[%d] \n", __FUNCTION__, __LINE__,
				ret);
		return ISC_I2C_ERROR;
	}

	for (i = 0; i < SECTION_NUM; i++) {
		ts_info[i].start_addr = rd_buf[i];

		/* 
		 *  previous core binary had 4 sections while current version contains 3 of them
		 * for compatibleness, register address was not modified so we get 1,2,3,5,6,7th
		 * data of read buffer
		 */
		ts_info[i].end_addr = rd_buf[i + SECTION_NUM + 1];
	}

	for (i = 0; i < SECTION_NUM; i++) {
		pr_info("\tTS : Section(%d) version: 0x%02X\n", i, ts_info[i].version);
		pr_info("\tTS : Section(%d) Start Address: 0x%02X\n", i,
				ts_info[i].start_addr);
		pr_info("\tTS : Section(%d) End Address: 0x%02X\n", i,
				ts_info[i].end_addr);
		pr_info("\tTS : Section(%d) Compatibility: 0x%02X\n", i,
				ts_info[i].compatible_version);
	}

	pr_info("End mms100_get_version_info()\n");

	return ISC_SUCCESS;
}

static eISCRet_t mms100_seek_section_info(void) {
#define STRING_BUF_LEN		100

	int i;
	int offset = sizeof(struct mms_bin_hdr);

	pr_info("[TSP ISC] %s\n", __func__);

	fw_hdr = (struct mms_bin_hdr *)fw_mfsb->data;
	img = kzalloc(sizeof(*img) * fw_hdr->section_num, GFP_KERNEL);
	for (i = 0; i < fw_hdr->section_num ;i++, offset += sizeof(struct mms_fw_img)){
		img[i] = (struct mms_fw_img *)(fw_mfsb->data + offset);
		mfsb_info[i].version = img[i]->version;
		mfsb_info[i].start_addr = img[i]->start_page;
		mfsb_info[i].end_addr = img[i]->end_page;
		if(i==0){
			mfsb_info[i].compatible_version = img[i]->version;
		}else{
			mfsb_info[i].compatible_version = img[i-1]->version;
		}
	}

	for (i = 0; i < SECTION_NUM; i++) {
		pr_info("\tMFSB : Section(%d) Version: 0x%02X\n", i,
				mfsb_info[i].version);
		pr_info("\tMFSB : Section(%d) Start Address: 0x%02X\n", i,
				mfsb_info[i].start_addr);
		pr_info("\tMFSB : Section(%d) End Address: 0x%02X\n", i,
				mfsb_info[i].end_addr);
		pr_info("\tMFSB : Section(%d) Compatibility: 0x%02X\n", i,
				mfsb_info[i].compatible_version);
	}

	pr_info("End mms100_seek_section_info()\n");

	return ISC_SUCCESS;
}


static eISCRet_t mms100_compare_version_info(struct i2c_client *_client) {
	int i;
	//unsigned char expected_compatibility[SECTION_NUM];
	int target_ver[SECTION_NUM];
	int fw_up_to_date = true;
	
	pr_info("[TSP ISC] %s\n", __func__);
	
	if (mms100_get_version_info(_client) != ISC_SUCCESS)
		return ISC_I2C_ERROR;

	mms100_seek_section_info();

	target_ver[0] = mfsb_info[0].version;
	section_update_flag[0] = false;
	for (i = SEC_CORE; i < SECTION_NUM; i++) {
		
		if (mfsb_info[i].version != ts_info[i].version) {
			fw_up_to_date = false;
			section_update_flag[i] = true;
			target_ver[i] = mfsb_info[i].version;
		/*	
			if(mfsb_info[0].version != ts_info[0].version){
				section_update_flag[0]=true;
				section_update_flag[1]=true;
				section_update_flag[2]=true;
			}
		*/
			if(mfsb_info[1].version != ts_info[1].version){
				section_update_flag[0]=false;
				section_update_flag[1]=true;
				section_update_flag[2]=true;
			}

		} else {
			target_ver[i] = ts_info[i].version;
		}
	}

	if (fw_up_to_date) {
		pr_info("mms_ts firmware version is up to date\n");
		return ISC_NO_NEED_UPDATE_ERROR;
	}

#if 0
	if (section_update_flag[SEC_BOOTLOADER])
		expected_compatibility[SEC_CORE] = mfsb_info[SEC_BOOTLOADER].version;
	else
		expected_compatibility[SEC_CORE] = ts_info[SEC_BOOTLOADER].version;

	if (section_update_flag[SEC_CORE])
		expected_compatibility[SEC_CONFIG] = mfsb_info[SEC_CORE].version;
	else
		expected_compatibility[SEC_CONFIG] = ts_info[SEC_CORE].version;

	/* ?? what is this for ?? */
	//for (i = SEC_CORE; i < SEC_CONFIG; i++) {
	for (i = 0; i < SECTION_NUM; i++) {
		if (section_update_flag[i]) {
			pr_info("section_update_flag(%d), 0x%02x, 0x%02x\n", i,
					expected_compatibility[i], mfsb_info[i].compatible_version);

			if (expected_compatibility[i] != mfsb_info[i].compatible_version)
				return ISC_COMPATIVILITY_ERROR;
		} else {
			pr_info("!section_update_flag(%d), 0x%02x, 0x%02x\n", i,
					expected_compatibility[i], ts_info[i].compatible_version);
			if (expected_compatibility[i] != ts_info[i].compatible_version)
				return ISC_COMPATIVILITY_ERROR;
		}
	}
#else

	for (i = 1; i < SECTION_NUM; i++) {
		if (target_ver[i - 1] != mfsb_info[i].compatible_version) {
			pr_info("compatibility version mismatch(%d), 0x%02x, 0x%02x\n", i,
					target_ver[i - 1], mfsb_info[i].compatible_version);
			return ISC_COMPATIVILITY_ERROR;
		}
	}

#endif

	pr_info("End mms100_compare_version_info()\n");

	return ISC_SUCCESS;
}

static eISCRet_t mms100_enter_ISC_mode(struct i2c_client *_client) {
	int ret;
	unsigned char wr_buf[2];

	pr_info("[TSP ISC] %s\n", __func__);

	wr_buf[0] = ISC_CMD_ENTER_ISC;          // command
	wr_buf[1] = ISC_CMD_ENTER_ISC_PARA1;    // sub_command

	ret = i2c_master_send(_client, wr_buf, 2);

	if (ret < 0) {
		pr_info("[TSP ISC] %s,%d: i2c write fail[%d] \n", __FUNCTION__,
				__LINE__, ret);
		return ISC_I2C_ERROR;
	}

	mms100_msdelay(50);

	pr_info("End mms100_enter_ISC_mode()\n");

	return ISC_SUCCESS;
}

static eISCRet_t mms100_enter_config_update(struct i2c_client *_client) {
	int ret;
	unsigned char wr_buf[10] = { 0, };
	unsigned char rd_buf;

	pr_info("[TSP ISC] %s\n", __func__);

	wr_buf[0] = ISC_CMD_UPDATE_MODE;
	wr_buf[1] = ISC_SUBCMD_ENTER_UPDATE;

	ret = i2c_master_send(_client, wr_buf, 10);

	if (ret < 0) {
		pr_info("[TSP ISC] %s,%d: i2c write fail[%d] \n", __FUNCTION__,
				__LINE__, ret);
		return ISC_I2C_ERROR;
	}

	ret = mms100_i2c_read(_client, ISC_CMD_CONFIRM_STATUS, 1, &rd_buf);

	if (ret < 0) {
		pr_info("[TSP ISC] %s,%d: i2c read fail[%d] \n", __FUNCTION__, __LINE__,
				ret);
		return ISC_I2C_ERROR;
	}

	if (rd_buf != ISC_STATUS_UPDATE_MODE)
		return ISC_UPDATE_MODE_ENTER_ERROR;

	pr_info("End mms100_enter_config_update()\n");

	return ISC_SUCCESS;
}

static eISCRet_t mms100_ISC_clear_page(struct i2c_client *_client,
		unsigned char _page_addr) {
	int ret;
	unsigned char rd_buf;

	pr_info("[TSP ISC] %s\n", __func__);

	//_buf = (unsigned char*) kmalloc(sizeof(unsigned char) * PACKET_SIZE);
	memset(&g_wr_buf[3], 0xFF, PAGE_DATA);

	g_wr_buf[0] = ISC_CMD_UPDATE_MODE;        // command
	g_wr_buf[1] = ISC_SUBCMD_DATA_WRITE;       // sub_command
	g_wr_buf[2] = _page_addr;

	g_wr_buf[PAGE_HEADER + PAGE_DATA] = crc0_buf[_page_addr];
	g_wr_buf[PAGE_HEADER + PAGE_DATA + 1] = crc1_buf[_page_addr];

	ret = i2c_master_send(_client, g_wr_buf, PACKET_SIZE);

	if (ret < 0) {
		pr_info("[TSP ISC] %s,%d: i2c write fail[%d] \n", __FUNCTION__,
				__LINE__, ret);
		return ISC_I2C_ERROR;
	}

	ret = mms100_i2c_read(_client, ISC_CMD_CONFIRM_STATUS, 1, &rd_buf);

	if (ret < 0) {
		pr_info("[TSP ISC] %s,%d: i2c read fail[%d] \n", __FUNCTION__, __LINE__,
				ret);
		return ISC_I2C_ERROR;
	}

	if (rd_buf != ISC_STATUS_CRC_CHECK_SUCCESS)
		return ISC_UPDATE_MODE_ENTER_ERROR;

	pr_info("End mms100_ISC_clear_page()\n");

	return ISC_SUCCESS;

}

static eISCRet_t mms100_ISC_clear_validate_markers(struct i2c_client *_client) {
	eISCRet_t ret_msg;
	int i, j;
	bool is_matched_address;

	pr_info("[TSP ISC] %s\n", __func__);

	for (i = SEC_CORE; i <= SEC_CONFIG; i++) {
		if (section_update_flag[i]) {
			if (ts_info[i].end_addr <= 30 && ts_info[i].end_addr > 0) {
				ret_msg = mms100_ISC_clear_page(_client, ts_info[i].end_addr);

				if (ret_msg != ISC_SUCCESS)
					return ret_msg;
			}
		}
	}

	for (i = SEC_CORE; i <= SEC_CONFIG; i++) {
		if (section_update_flag[i]) {
			is_matched_address = false;
			for (j = SEC_CORE; j <= SEC_CONFIG; j++) {
				if (mfsb_info[i].end_addr == ts_info[i].end_addr) {
					is_matched_address = true;
					break;
				}
			}

			if (!is_matched_address) {
				if (mfsb_info[i].end_addr <= 30 && mfsb_info[i].end_addr > 0) {
					ret_msg = mms100_ISC_clear_page(_client,
							mfsb_info[i].end_addr);

					if (ret_msg != ISC_SUCCESS)
						return ret_msg;
				}
			}
		}
	}

	pr_info("End mms100_ISC_clear_validate_markers()\n");

	return ISC_SUCCESS;
}

static void mms100_calc_crc(unsigned char *crc, int page_addr,
		unsigned char* ptr_fw)	// 2012.08.30
		{
	int i, j;

	unsigned char ucData;

	unsigned short SeedValue;
	unsigned short CRC_check_buf;
	unsigned short CRC_send_buf;
	unsigned short IN_data;
	unsigned short XOR_bit_1;
	unsigned short XOR_bit_2;
	unsigned short XOR_bit_3;

	// Seed

	CRC_check_buf = 0xFFFF;
	SeedValue = (unsigned short) page_addr;

	for (i = 7; i >= 0; i--) {
		IN_data = (SeedValue >> i) & 0x01;
		XOR_bit_1 = (CRC_check_buf & 0x0001) ^ IN_data;
		XOR_bit_2 = XOR_bit_1 ^ (CRC_check_buf >> 11 & 0x01);
		XOR_bit_3 = XOR_bit_1 ^ (CRC_check_buf >> 4 & 0x01);
		CRC_send_buf = (XOR_bit_1 << 4) | (CRC_check_buf >> 12 & 0x0F);
		CRC_send_buf = (CRC_send_buf << 7) | (XOR_bit_2 << 6)
				| (CRC_check_buf >> 5 & 0x3F);
		CRC_send_buf = (CRC_send_buf << 4) | (XOR_bit_3 << 3)
				| (CRC_check_buf >> 1 & 0x0007);
		CRC_check_buf = CRC_send_buf;
	}

	for (i = 0; i < 1024; i++) {
		ucData = ptr_fw[i];

		for (j = 7; j >= 0; j--) {
			IN_data = (ucData >> j) & 0x0001;
			XOR_bit_1 = (CRC_check_buf & 0x0001) ^ IN_data;
			XOR_bit_2 = XOR_bit_1 ^ (CRC_check_buf >> 11 & 0x01);
			XOR_bit_3 = XOR_bit_1 ^ (CRC_check_buf >> 4 & 0x01);
			CRC_send_buf = (XOR_bit_1 << 4) | (CRC_check_buf >> 12 & 0x0F);
			CRC_send_buf = (CRC_send_buf << 7) | (XOR_bit_2 << 6)
					| (CRC_check_buf >> 5 & 0x3F);
			CRC_send_buf = (CRC_send_buf << 4) | (XOR_bit_3 << 3)
					| (CRC_check_buf >> 1 & 0x0007);
			CRC_check_buf = CRC_send_buf;
		}
	}

	crc[0] = (unsigned char) ((CRC_check_buf >> 8) & 0xFF);
	crc[1] = (unsigned char) ((CRC_check_buf >> 0) & 0xFF);
}

static eISCRet_t mms100_update_section_data(struct i2c_client *_client) {
#define STRING_BUF_LEN		100

	int i, j, ret;					// 2012.08.30
	unsigned char rd_buf;
	unsigned char crc[2];			// 2012.08.30
	int ptr;
	const u8 *ptr_fw;
	int page_addr;
	const u8 *fw_data;
	pr_info("[TSP ISC] %s\n", __func__);
	fw_data = fw_mfsb->data + fw_hdr->binary_offset;
	for (i = 0; i < fw_hdr->section_num ; i++){
		pr_info("update flag (%d)\n", section_update_flag[i]);
		ptr = img[i]->offset;
		//fw_data[i] = fw_mfsb->data + fw_hdr->binary_offset + img[i]->offset;
		if (section_update_flag[i]) {
			ptr_fw = fw_data+ptr;
			pr_info("binary found\n");

			for (page_addr = mfsb_info[i].start_addr;
					page_addr <= mfsb_info[i].end_addr; page_addr++) {
				if (page_addr - mfsb_info[i].start_addr > 0)
					//ptr_fw += PACKET_SIZE;						// 2012.08.30
					ptr_fw += 1024;								// 2012.08.30

				//if ((ptr_fw[0] != ISC_CMD_UPDATE_MODE) || (ptr_fw[1] != ISC_SUBCMD_DATA_WRITE) || (ptr_fw[2] != page_addr))	// 2012.08.30
				//    return ISC_WRITE_BUFFER_ERROR;																			

				g_wr_buf[0] = ISC_CMD_UPDATE_MODE;				// 2012.08.30
				g_wr_buf[1] = ISC_SUBCMD_DATA_WRITE;
				g_wr_buf[2] = (unsigned char) page_addr;

				for (j = 0; j < 1024; j += 4) {
					g_wr_buf[3 + j] = ptr_fw[j + 3];
					g_wr_buf[3 + j + 1] = ptr_fw[j + 2];
					g_wr_buf[3 + j + 2] = ptr_fw[j + 1];
					g_wr_buf[3 + j + 3] = ptr_fw[j + 0];
				}

				mms100_calc_crc(crc, page_addr, &g_wr_buf[3]);

				g_wr_buf[1027] = crc[0];
				g_wr_buf[1028] = crc[1];

				//ret = mms100_i2c_write(_client, ptr_fw, PACKET_SIZE);		// 2012.08.30
				pr_info("crc val : %X%X\n", crc[0], crc[1]);
				ret = i2c_master_send(_client, g_wr_buf, PACKET_SIZE);	// 2012.08.30

				if (ret < 0) {
					pr_info("[TSP ISC] %s,%d: i2c write fail[%d] \n",
							__FUNCTION__, __LINE__, ret);
					return ISC_I2C_ERROR;
				}

				ret = mms100_i2c_read(_client, ISC_CMD_CONFIRM_STATUS, 1,
						&rd_buf);

				if (ret < 0) {
					pr_info("[TSP ISC] %s,%d: i2c read fail[%d] \n",
							__FUNCTION__, __LINE__, ret);
					return ISC_I2C_ERROR;
				}

				if (rd_buf != ISC_STATUS_CRC_CHECK_SUCCESS)
					return ISC_CRC_ERROR;

				section_update_flag[i] = false;
				pr_info("section(%d) updated.\n", i);
			}
		}	
	}

	pr_info("End mms100_update_section_data()\n");

	return ISC_SUCCESS;
}

static eISCRet_t mms100_open_kernel_mbinary(struct i2c_client *_client) {
	const char *fw_name = MMS_BUILTIN_FW_NAME;
	const struct firmware *fw = NULL;
	int ret;
	dev_info(&_client->dev, "[TSP ISC] %s\n", __func__);

	fw_name = kstrdup(fw_name, GFP_KERNEL);
	ret=request_firmware(&fw, fw_name, &_client->dev);
	fw_mfsb = kzalloc(sizeof(*fw_mfsb), GFP_KERNEL);
	fw_mfsb->data = fw->data;
	if (ret) {
		dev_info(&_client->dev, "failed to schedule firmware update\n");
		return ISC_FILE_OPEN_ERROR;
	}
	dev_info(&_client->dev, "[TSP ISC] End %s\n", __func__);

	return ISC_SUCCESS;
}

static eISCRet_t mms100_open_sdcard_mbinary(struct i2c_client *_client) {
	mm_segment_t old_fs = {0};
	struct file *fp;
	int nread = 0;
	unsigned char *fw_data;
	int ret = ISC_FILE_OPEN_ERROR;

	dev_info(&_client->dev, "[TSP ISC] %s\n", __func__);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(MMS_SDCARD_FW_NAME, O_RDONLY, S_IRUSR);
	if (IS_ERR(fp)) {
		dev_err(&_client->dev,
			"file %s open error:%d\n", MMS_SDCARD_FW_NAME, (s32)fp);
		goto open_err;
	}

	fw_mfsb = kzalloc(sizeof(*fw_mfsb), GFP_KERNEL);
	fw_mfsb->size= fp->f_path.dentry->d_inode->i_size;
	if(fw_mfsb->size >0 ){
		fw_data = kzalloc(fw_mfsb->size, GFP_KERNEL);
		nread = vfs_read(fp, (char __user *)fw_data, fw_mfsb->size, &fp->f_pos);
		dev_info(&_client->dev,
				"%s: start, fw path(%s), fw size %u Bytes\n", __func__,
				MMS_SDCARD_FW_NAME, fw_mfsb->size);

		if (nread != fw_mfsb->size) {
			dev_err(&_client->dev, "%s : fail to read fw (nread = %d)\n",
					  __func__, nread);
			goto open_err;
		}
		fw_mfsb->data = fw_data;
		dev_info(&_client->dev, "ums fw is opened successfully!!\n");
		ret = ISC_SUCCESS;
	}

	filp_close(fp, current->files);
open_err:
	set_fs(old_fs);
	return ret;
}
/*
static eISCRet_t mms100_close_mbinary(void) {
	pr_info("[TSP ISC] %s\n", __func__);

	release_firmware(fw_mfsb);

	pr_info("End mms100_close_mbinary()\n");

	return ISC_SUCCESS;
}
*/
/*
 *       본 함수는 TSP device driver의 i2c_set_clientdata가 완료된 이후에 호출되어야 합니다.
 */
eISCRet_t mms100_ISC_download_mbinary(struct i2c_client *_client, bool force_update, int fw_location) {
	eISCRet_t ret_msg = ISC_NONE;

	pr_info("[TSP ISC] %s\n", __func__);

	//함수내 Todo 확인 해 주세요..!!!
	//mms100_reset(_client);

	//mms100_seek_section_info();

	/*IC의 부팅 여부 판단*/
	ret_msg = mms100_check_operating_mode(_client, EC_BOOT_ON_SUCCEEDED);
	if (ret_msg != ISC_SUCCESS)
		goto ISC_ERROR_HANDLE;

	/*FW BIN(.mfsb)을 open 함.*/
	//함수내 mfsb 경로 확인 해 주세요..!!!
	if (fw_location == BUILT_IN){
		ret_msg = mms100_open_kernel_mbinary(_client);
		if (ret_msg != ISC_SUCCESS)
			goto ISC_ERROR_HANDLE;
	}
	else if (fw_location == UMS){
		ret_msg = mms100_open_sdcard_mbinary(_client);
		if (ret_msg != ISC_SUCCESS)
			goto ISC_ERROR_HANDLE;
	}

	/*Config version Check*/
	if (force_update) {
		int i;
		mms100_seek_section_info();
		section_update_flag[0] = false; //don't update boot area
		for (i = SEC_CORE; i < SECTION_NUM; i++)
			section_update_flag[i] = true;
	} else {
		ret_msg = mms100_compare_version_info(_client);
		if (ret_msg != ISC_SUCCESS)
			goto ISC_ERROR_HANDLE;
	}

	ret_msg = mms100_enter_ISC_mode(_client);
	if (ret_msg != ISC_SUCCESS)
		goto ISC_ERROR_HANDLE;

	ret_msg = mms100_enter_config_update(_client);
	if (ret_msg != ISC_SUCCESS)
		goto ISC_ERROR_HANDLE;

	
	ret_msg = mms100_ISC_clear_validate_markers(_client);
	if (ret_msg != ISC_SUCCESS)
		goto ISC_ERROR_HANDLE;

	ret_msg = mms100_update_section_data(_client);
	if (ret_msg != ISC_SUCCESS)
		goto ISC_ERROR_HANDLE;
	kfree(img);
	//mms100_reset(_client); 

	pr_info("FIRMWARE_UPDATE_FINISHED!!!\n");

	ret_msg = ISC_SUCCESS;

ISC_ERROR_HANDLE: 
	if (ret_msg != ISC_SUCCESS)
		pr_info("ISC_ERROR_CODE: %d\n", ret_msg);

	mms100_reset(_client);
	//mms100_close_mbinary();

	return ret_msg;
}

