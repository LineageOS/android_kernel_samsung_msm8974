#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/crypto.h>
#include <linux/firmware.h>


#include "../../iio.h"
#include "../../buffer.h"
#include "../../kfifo_buf.h"
#include "../../sysfs.h"

#include "inv_sensors_sm.h"
#include "inv_sensors_common.h"
#include "inv_dmp_base.h"
#include "inv_dmp_fw.h"


#define DMP_FW_DEFAULT_PATH  "/vendor/firmware/"
#define DMP_FW_BUFFER_LEN 32

#define DMP_BANK_SIZE (256)

struct dmp_fw_cfg {
	struct file *fp;
	loff_t pos;
	int ind;

	const struct firmware *fw;
	struct crypto_cipher *cipher;
	struct invsens_i2c_t *i2c_handle;


	char cipher_key[DMP_CIPHER_KEY_LEN];

	unsigned char buffer[DMP_FW_BUFFER_LEN];

};


static int dmp_fw_decrypt(struct dmp_fw_cfg *fcfg, unsigned char *buffer)
{
	unsigned char encrypted[DMP_FW_BUFFER_LEN];

	memcpy(encrypted, &fcfg->fw->data[fcfg->pos], DMP_FW_BUFFER_LEN);
	fcfg->pos += DMP_FW_BUFFER_LEN;

	crypto_cipher_decrypt_one(fcfg->cipher, buffer, encrypted);
	crypto_cipher_decrypt_one(fcfg->cipher, &buffer[16],
				  &encrypted[16]);

	return 0;
}



static int dmp_fw_open(struct dmp_fw_cfg *fcfg, const char *filename)
{
	struct crypto_cipher *cipher;
	int res = 0;
	char path[512];

	INV_DBG_FUNC_NAME;

	cipher = crypto_alloc_cipher("aes", 0, CRYPTO_ALG_ASYNC);

	if (!cipher)
		return -ENOMEM;

	res =
	    crypto_cipher_setkey(cipher, fcfg->cipher_key,
				 DMP_CIPHER_KEY_LEN);
	if (!res)
		fcfg->cipher = cipher;

	snprintf(path, sizeof(path), DMP_FW_DEFAULT_PATH "%s", filename);

	res = request_firmware(&fcfg->fw, filename,
		&fcfg->i2c_handle->client->dev);
	if (res)
		goto error_case;

	fcfg->pos = 0;

	dmp_fw_decrypt(fcfg, fcfg->buffer);
	fcfg->ind = 0;

	return 0;

error_case:
	return res;
}

static int dmp_fw_read_block(struct dmp_fw_cfg *fcfg, uint8_t *buffer,
				int length)
{
	int res = 0;
	int remains = length;
	int available = 0;
	int ind = 0;

	if (!buffer || length <= 0)
		return -EINVAL;

	do {
		if (remains + fcfg->ind >= DMP_FW_BUFFER_LEN)
			available = DMP_FW_BUFFER_LEN - fcfg->ind;
		else
			available = remains;

		memcpy(&buffer[ind], &fcfg->buffer[fcfg->ind], available);
		ind += available;
		fcfg->ind += available;

		if (fcfg->ind == DMP_FW_BUFFER_LEN) {
			dmp_fw_decrypt(fcfg, fcfg->buffer);
			fcfg->ind = 0;
		}

		remains -= available;
	} while (remains > 0);

	return res;
}


static int dmp_fw_read_int(struct dmp_fw_cfg *fcfg, int *value)
{
	int res = 0;
	u8 data[4];
	int v = 0;

	INV_DBG_FUNC_NAME;

	res = dmp_fw_read_block(fcfg, data, 4);
	if (res)
		return res;

	v = data[0] + (int) (data[1] << 8)
	    + (int) (data[2] << 16) + (int) (data[3] << 24);

	if (value)
		*value = v;

	return res;
}


static int dmp_fw_read_u16(struct dmp_fw_cfg *fcfg, u16 *value)
{
	int res = 0;
	u8 data[2];

	res = dmp_fw_read_block(fcfg, data, 2);
	if (res)
		return res;

	*value = (int) data[0] + (int) (data[1] << 8);

	return res;
}


static int dmp_fw_close(struct dmp_fw_cfg *fcfg)
{

	INV_DBG_FUNC_NAME;

	if (fcfg->cipher)
		crypto_free_cipher(fcfg->cipher);

	fcfg->cipher = NULL;

	if (fcfg->fw)
		release_firmware(fcfg->fw);

	fcfg->fw = NULL;

	return 0;
}

int invdmp_download_fw(const char *filename, int offset,
		       struct invsens_i2c_t *i2c_handle, u16 i2c_addr,
		       const char *key, struct invdmp_fw_data_t *fw_data)
{
	int res = 0;
	struct dmp_fw_cfg fcfg_data = { 0 };
	struct dmp_fw_cfg *fcfg = &fcfg_data;
	int size = 0, i = 0;
	int bank, write_size;
	u16 mem_addr;
	unsigned char data[DMP_BANK_SIZE] = { 0 };

	INV_DBG_FUNC_NAME;

	if (!filename || !fw_data || !key)
		return -EINVAL;

	memcpy(fcfg->cipher_key, key, DMP_CIPHER_KEY_LEN);

	fcfg->i2c_handle = i2c_handle;

	res = dmp_fw_open(fcfg, filename);
	if (res)
		goto error_case;

	res = dmp_fw_read_int(fcfg, &size);
	if (res)
		goto error_case;

	fw_data->key_map =
	    kzalloc(sizeof(struct invdmp_key_t) * size, GFP_KERNEL);
	if (!fw_data->key_map) {
		res = -ENOMEM;
		goto error_case;
	}
	fw_data->key_size = size;

	for (i = 0; i < fw_data->key_size; i++) {
		dmp_fw_read_u16(fcfg, &fw_data->key_map[i].addr);
		dmp_fw_read_u16(fcfg, &fw_data->key_map[i].key);
	}

	res = dmp_fw_read_int(fcfg, &size);
	if (res)
		goto error_case;

	/* Write and verify memory */
	for (bank = 0; size > 0; bank++, size -= write_size) {
		if (size > DMP_BANK_SIZE)
			write_size = DMP_BANK_SIZE;
		else
			write_size = size;

		mem_addr = ((bank << 8) | 0x00);

		if (bank == 0) {
			mem_addr += offset;
			write_size -= offset;
		}

		res = dmp_fw_read_block(fcfg, data, write_size);
		if (res)
			goto error_case;

		res =
		    inv_i2c_mem_write(i2c_handle, i2c_addr, mem_addr,
				      write_size, data);
	}

	INVSENS_LOGD("%s : firmware loading done(%d)!!!\n", __func__, res);

	res = dmp_fw_close(fcfg);
	if (res)
		goto error_case;

	return 0;

error_case:
	return res;
}
