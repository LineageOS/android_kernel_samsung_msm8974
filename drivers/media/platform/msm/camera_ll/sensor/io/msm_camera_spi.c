/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <mach/camera2.h>
#include "msm_camera_spi.h"

#undef SPIDBG
#ifdef CONFIG_MSMB_CAMERA_DEBUG
#define SPIDBG(fmt, args...) pr_err(fmt, ##args)
#define S_I2C_DBG(fmt, args...) pr_debug(fmt, ##args)
#else
#define SPIDBG(fmt, args...) do { } while (0)
#define S_I2C_DBG(fmt, args...) do { } while (0)
#endif

static int msm_camera_spi_txfr(struct spi_device *spi, char *txbuf,
			       char *rxbuf, int num_byte)
{
	struct spi_transfer t;
	struct spi_message m;

	memset(&t, 0, sizeof(t));
	t.tx_buf = txbuf;
	t.rx_buf = rxbuf;
	t.len = num_byte;
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	return spi_sync(spi, &m);
}

/**
  * msm_camera_set_addr() - helper function to set transfer address
  * @addr:	device address
  * @addr_len:	the addr field length of an instruction
  * @type:	type (i.e. byte-length) of @addr
  * @str:	shifted address output, must be zeroed when passed in
  *
  * This helper function sets @str based on the addr field length of an
  * instruction and the data length.
  */
static void msm_camera_set_addr(uint32_t addr, uint8_t addr_len,
				enum msm_camera_i2c_reg_addr_type type,
				char *str)
{
	int i, len;

	if (!addr_len)
		return;
	if (addr_len < type)
		SPIDBG("%s: omitting higher bits in address\n", __func__);

	/* only support transfer MSB first for now */
	len = addr_len - type;
	for (i = len; i < addr_len; i++) {
		if (i >= 0)
			str[i] = (addr >> (BITS_PER_BYTE * (addr_len - i - 1)))
				& 0xFF;
	}

}

/**
  * msm_camera_spi_tx_helper() - wrapper for SPI transaction
  * @client:	io client
  * @inst:	inst of this transaction
  * @addr:	device addr following the inst
  * @data:	output byte array (could be NULL)
  * @num_byte:	size of @data
  * @tx, rx:	optional transfer buffer.  It must be at least header
  *		+ @num_byte long.
  *
  * This is the core function for SPI transaction, except for writes.  It first
  * checks address type, then allocates required memory for tx/rx buffers.
  * It sends out <opcode><addr>, and optionally receives @num_byte of response,
  * if @data is not NULL.  This function does not check for wait conditions,
  * and will return immediately once bus transaction finishes.
  *
  * This function will allocate buffers of header + @num_byte long.  For
  * large transfers, the allocation could fail.  External buffer @tx, @rx
  * should be passed in to bypass allocation.  The size of buffer should be
  * at least header + num_byte long.  Since buffer is managed externally,
  * @data will be ignored, and read results will be in @rx.
  * @tx, @rx also can be used for repeated transfers to improve performance.
  */
int32_t msm_camera_spi_tx_helper(struct msm_camera_i2c_client *client,
		struct msm_camera_spi_inst *inst, uint32_t addr, uint8_t *data,
	uint32_t num_byte, char *tx, char *rx)
{
	int32_t rc = -EINVAL;
	struct spi_device *spi = client->spi_client->spi_master;
	char *ctx = NULL, *crx = NULL;
	uint32_t len, hlen;
	uint8_t retries = client->spi_client->retries;

	if ((client->addr_type != MSM_CAMERA_I2C_BYTE_ADDR)
	    && (client->addr_type != MSM_CAMERA_I2C_WORD_ADDR)
	    && (client->addr_type != MSM_CAMERA_I2C_3B_ADDR))
		return rc;

	hlen = msm_camera_spi_get_hlen(inst);
	len = hlen + num_byte;

	if (tx)
		ctx = tx;
	else
		ctx = kzalloc(len, GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;
	if (num_byte) {
		if (rx)
			crx = rx;
		else
			crx = kzalloc(len, GFP_KERNEL);
		if (!crx) {
			if (!tx)
				kfree(ctx);
		return -ENOMEM;
		}
	} else {
		crx = NULL;
	}

	ctx[0] = inst->opcode;
	msm_camera_set_addr(addr, inst->addr_len, client->addr_type, ctx + 1);
	SPIDBG("%s: tx(%u): %02x %02x %02x %02x\n", __func__,
	       len, ctx[0], ctx[1], ctx[2], ctx[3]);
	while ((rc = msm_camera_spi_txfr(spi, ctx, crx, len)) && retries) {
		retries--;
		msleep(client->spi_client->retry_delay);
	}
	if (rc < 0) {
		SPIDBG("%s: failed %d\n", __func__, rc);
		goto out;
	}
	if (data && num_byte && !rx)
		memcpy(data, crx + hlen, num_byte);
out:
	if (!tx)
		kfree(ctx);
	if (!rx)
		kfree(crx);
	return rc;
}

int32_t msm_camera_spi_read(struct msm_camera_i2c_client *client,
	uint32_t addr, uint16_t *data,
	enum msm_camera_i2c_data_type data_type)
{
	int32_t rc = -EINVAL;
	uint8_t temp[2];

	if ((data_type != MSM_CAMERA_I2C_BYTE_DATA)
	    && (data_type != MSM_CAMERA_I2C_WORD_DATA))
		return rc;

	rc = msm_camera_spi_tx_helper(client,
			&client->spi_client->cmd_tbl.read, addr, &temp[0],
			data_type, NULL, NULL);
	if (rc < 0)
		return rc;

	if (data_type == MSM_CAMERA_I2C_BYTE_DATA)
		*data = temp[0];
	else
		*data = (temp[0] << BITS_PER_BYTE) | temp[1];

	SPIDBG("%s: addr 0x%x, data %u\n", __func__, addr, *data);
	return rc;
}

int32_t msm_camera_spi_read_seq(struct msm_camera_i2c_client *client,
	uint32_t addr, uint8_t *data, uint32_t num_byte)
{
	return msm_camera_spi_tx_helper(client,
		&client->spi_client->cmd_tbl.read_seq, addr, data, num_byte,
		NULL, NULL);
}

/**
  * msm_camera_spi_read_seq_l()- function for large SPI reads
  * @client:	io client
  * @addr:	device address to read
  * @num_byte:	read length
  * @tx,rx:	pre-allocated SPI buffer.  Its size must be at least
  *		header + num_byte
  *
  * This function is used for large transactions.  Instead of allocating SPI
  * buffer each time, caller is responsible for pre-allocating memory buffers.
  * Memory buffer must be at least header + num_byte.  Header length can be
  * obtained by msm_camera_spi_get_hlen().
  */
int32_t msm_camera_spi_read_seq_l(struct msm_camera_i2c_client *client,
	uint32_t addr, uint32_t num_byte, char *tx, char *rx)
{
	return msm_camera_spi_tx_helper(client,
		&client->spi_client->cmd_tbl.read_seq, addr, NULL, num_byte,
		tx, rx);
}
int32_t msm_camera_spi_query_id(struct msm_camera_i2c_client *client,
	uint32_t addr, uint8_t *data, uint32_t num_byte)
{
	return msm_camera_spi_tx_helper(client,
		&client->spi_client->cmd_tbl.query_id, addr, data, num_byte,
		NULL, NULL);
}
static int32_t msm_camera_spi_read_status_reg(
		struct msm_camera_i2c_client *client, uint8_t *status)
{
	struct msm_camera_spi_inst *rs =
		&client->spi_client->cmd_tbl.read_status;
	if (rs->addr_len != 0) {
		pr_err("%s: not implemented yet\n", __func__);
		return -EINVAL;
	}
	return msm_camera_spi_tx_helper(client, rs, 0, status, 1, NULL, NULL);
}
static int32_t msm_camera_spi_device_busy(struct msm_camera_i2c_client *client,
		uint8_t *busy)
{
	int rc;
	uint8_t st = 0;
	rc = msm_camera_spi_read_status_reg(client,  &st);
	if (rc < 0) {
		SPIDBG("%s: failed to read status reg\n", __func__);
		return rc;
	}
	*busy = st & client->spi_client->busy_mask;
	return 0;
}
static int32_t msm_camera_spi_wait(struct msm_camera_i2c_client *client,
		struct msm_camera_spi_inst *inst)
{
	uint8_t busy;
	int i, rc;
	SPIDBG("%s: op 0x%x wait start\n", __func__, inst->opcode);
	for (i = 0; i <= inst->delay_count; i++) {
		rc = msm_camera_spi_device_busy(client, &busy);
		if (rc < 0)
			return rc;
		if (!busy)
			break;
		else
			msleep(inst->delay_intv);
		SPIDBG("%s: op 0x%x wait\n", __func__, inst->opcode);
	}
	if (i > inst->delay_count) {
		SPIDBG("%s: op %x timed out\n", __func__, inst->opcode);
		return -ETIMEDOUT;
	}
	SPIDBG("%s: op %x finished\n", __func__, inst->opcode);
	return 0;
}
static int32_t msm_camera_spi_write_enable(
		struct msm_camera_i2c_client *client)
{
	struct msm_camera_spi_inst *we =
		&client->spi_client->cmd_tbl.write_enable;
	int rc;
	if (we->addr_len != 0) {
		pr_err("%s: not implemented yet\n", __func__);
		return -EINVAL;
	}
	rc = msm_camera_spi_tx_helper(client, we, 0, NULL, 0, NULL, NULL);
	if (rc < 0)
		SPIDBG("%s: write enable failed\n", __func__);
	return rc;
}
int32_t msm_camera_spi_erase(struct msm_camera_i2c_client *client,
			     uint32_t addr, uint32_t size)
{
	struct msm_camera_spi_inst *se = &client->spi_client->cmd_tbl.erase;
	int rc = 0;
	uint32_t cur;
	uint32_t end = addr + size;
	uint32_t erase_size = client->spi_client->erase_size;
	end = addr + size;
	for (cur = rounddown(addr, erase_size); cur < end; cur += erase_size) {
		SPIDBG("%s: erasing 0x%x\n", __func__, cur);
		rc = msm_camera_spi_write_enable(client);
		if (rc < 0)
			return rc;
		rc = msm_camera_spi_tx_helper(client, se, cur, NULL, 0,
					      NULL, NULL);
		if (rc < 0) {
			SPIDBG("%s: erase failed\n", __func__);
			return rc;
		}
		rc = msm_camera_spi_wait(client, se);
		if (rc < 0) {
			SPIDBG("%s: erase timedout\n", __func__);
			return rc;
		}
	}
	return rc;
}
/**
  * msm_camera_spi_page_program() - core function to perform write
  * @client:	need for obtaining SPI device
  * @addr:	address to program on device
  * @data:	data to write
  * @len:	size of data
  * @tx:	tx buffer, size >= header + len
  *
  * This function performs SPI write, and has no boundary check.  Writing range
  * should not cross page boundary, or data will be corrupted.  Transaction is
  * guaranteed to be finished when it returns.  This function should never be
  * used outside msm_camera_spi_write_seq().
  */
static int32_t msm_camera_spi_page_program(struct msm_camera_i2c_client *client,
		uint32_t addr, uint8_t *data, uint16_t len, uint8_t *tx)
{
	int rc;
	struct msm_camera_spi_inst *pg =
		&client->spi_client->cmd_tbl.page_program;
	struct spi_device *spi = client->spi_client->spi_master;
	uint8_t retries = client->spi_client->retries;
	uint8_t header_len = sizeof(pg->opcode) + pg->addr_len + pg->dummy_len;
	SPIDBG("%s: addr 0x%x, size 0x%x\n", __func__, addr, len);
	rc = msm_camera_spi_write_enable(client);
	if (rc < 0)
		return rc;
	memset(tx, 0, header_len);
	tx[0] = pg->opcode;
	msm_camera_set_addr(addr, pg->addr_len, client->addr_type, tx + 1);
	memcpy(tx + header_len, data, len);
	SPIDBG("%s: tx(%u): %02x %02x %02x %02x\n", __func__,
	       len, tx[0], tx[1], tx[2], tx[3]);
	while ((rc = spi_write(spi, tx, len + header_len)) && retries) {
		rc = msm_camera_spi_wait(client, pg);
		msleep(client->spi_client->retry_delay);
		retries--;
	}
	if (rc < 0) {
		SPIDBG("%s: failed %d\n", __func__, rc);
		return rc;
	}
	rc = msm_camera_spi_wait(client, pg);
	return rc;
}
int32_t msm_camera_spi_write_seq(struct msm_camera_i2c_client *client,
	uint32_t addr, uint8_t *data, uint32_t num_byte)
{
	struct msm_camera_spi_inst *pg =
		&client->spi_client->cmd_tbl.page_program;
	const uint32_t page_size = client->spi_client->page_size;
	uint8_t header_len = sizeof(pg->opcode) + pg->addr_len + pg->dummy_len;
	uint16_t len;
	uint32_t cur_len, end;
	char *tx, *pdata = data;
	int rc = -EINVAL;
	if ((client->addr_type != MSM_CAMERA_I2C_BYTE_ADDR)
	    && (client->addr_type != MSM_CAMERA_I2C_WORD_ADDR)
	    && (client->addr_type != MSM_CAMERA_I2C_3B_ADDR))
		return rc;
    /* single page write */
	if ((addr % page_size) + num_byte <= page_size) {
		len = header_len + num_byte;
		tx = kmalloc(len, GFP_KERNEL);
		if (!tx)
			goto NOMEM;
		rc = msm_camera_spi_page_program(client, addr, data,
						 num_byte, tx);
		if (rc < 0)
			goto ERROR;
		goto OUT;
	}
	/* multi page write */
	len = header_len + page_size;
	tx = kmalloc(len, GFP_KERNEL);
	if (!tx)
		goto NOMEM;
	while (num_byte) {
		end = min(page_size, (addr % page_size) + num_byte);
		cur_len = end - (addr % page_size);
		rc = msm_camera_spi_page_program(client, addr, pdata,
						 cur_len, tx);
		if (rc < 0)
			goto ERROR;
		addr += cur_len;
		pdata += cur_len;
		num_byte -= cur_len;
	}
	goto OUT;
NOMEM:
	SPIDBG("%s: memory allocation failed\n", __func__);
	return -ENOMEM;
ERROR:
	SPIDBG("%s: error write\n", __func__);
OUT:
	kfree(tx);
	return rc;
}
