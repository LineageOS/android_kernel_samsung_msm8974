
#include "inv_sensors_common.h"


#define REG_BANK_SEL            0x6D
#define REG_MEM_START_ADDR      0x6E
#define REG_MEM_RW              0x6F

static u32 invsens_log_level;


#ifdef CONFIG_INV_SENSORS_DBG
/**
 * i2c write/read hook to allocate and construct the string of the bytes
 * transferred over the wire. Supports minimal overhead pr_debug logs.
 */
static char *inv_i2c_debug_begin(u8 const *data, u32 len, char *string)
{
	int ii;
	string = kmalloc(len * 2 + 1, GFP_KERNEL);
	for (ii = 0; ii < len; ii++)
		sprintf(&string[ii * 2], "%02X", data[ii]);
	string[len * 2] = 0;
	return string;
}

/**
 * i2c write/read hook to free the string of the bytes allocated and
 * constructed by inv_i2c_debug_begin. Supports minimal overhead pr_debug logs.
 */
static char *inv_i2c_debug_end(char *string)
{
	kfree(string);
	return "";
}
#endif

int inv_i2c_single_write(struct invsens_i2c_t *i2c_handle, u16 i2c_addr,
			 u8 reg, u8 data)
{
	int result = 0;
	u8 tmp[2];
	struct i2c_msg msg;
	tmp[0] = reg;
	tmp[1] = data;

	msg.addr = i2c_addr;
	msg.flags = 0;		/* write */
	msg.buf = tmp;
	msg.len = 2;

#ifdef CONFIG_INV_SENSORS_DBG
	INVSENS_LOGD("WR%02X%02X%02X\n", i2c_addr, reg, data);
#endif

	result = i2c_transfer(i2c_handle->client->adapter, &msg, 1);
	if (result < 1) {
		if (result == 0)
			result = -EIO;
	} else
		result = 0;
	return result;
}

int inv_i2c_read(struct invsens_i2c_t *i2c_handle, u16 i2c_addr, u8 reg,
	u16 length, u8 *data)
{
	struct i2c_msg msgs[2];
	int result;

	if (!data)
		return -EINVAL;

	msgs[0].addr = i2c_addr;
	msgs[0].flags = 0;	/* write */
	msgs[0].buf = &reg;
	msgs[0].len = 1;

	msgs[1].addr = i2c_addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].buf = data;
	msgs[1].len = length;

	result = i2c_transfer(i2c_handle->client->adapter, msgs, 2);

	if (result < 2) {
		if (result >= 0)
			result = -EIO;
	} else
		result = 0;

#ifdef CONFIG_INV_SENSORS_DBG
	{
		char *read = 0;
		INVSENS_LOGD("RD%02X%02X%02X -> %s%s\n",
			     i2c_addr, reg, length,
			     inv_i2c_debug_begin(data, length, read),
			     inv_i2c_debug_end(read));
	}
#endif
	return result;

}


int inv_i2c_mem_write(struct invsens_i2c_t *i2c_handle, u16 i2c_addr,
	u16 mem_addr, u16 length, u8 *data)
{
	u8 bank[2];
	u8 addr[2];
	u8 buf[513];

	struct i2c_msg msgs[3];
	int result;

	if (!data || !i2c_handle)
		return -EINVAL;

	if (length >= (sizeof(buf) - 1))
		return -ENOMEM;

	bank[0] = REG_BANK_SEL;
	bank[1] = mem_addr >> 8;

	addr[0] = REG_MEM_START_ADDR;
	addr[1] = mem_addr & 0xFF;

	buf[0] = REG_MEM_RW;
	memcpy(buf + 1, data, length);

	/* write message */
	msgs[0].addr = i2c_addr;
	msgs[0].flags = 0;
	msgs[0].buf = bank;
	msgs[0].len = sizeof(bank);

	msgs[1].addr = i2c_addr;
	msgs[1].flags = 0;
	msgs[1].buf = addr;
	msgs[1].len = sizeof(addr);

	msgs[2].addr = i2c_addr;
	msgs[2].flags = 0;
	msgs[2].buf = (u8 *) buf;
	msgs[2].len = length + 1;

#ifdef CONFIG_INV_SENSORS_DBG
	{
		char *write = 0;
		INVSENS_LOGD("WM%02X%02X%02X - %s%s : %d\n",
			     i2c_addr,
			     bank[1], addr[1],
			     inv_i2c_debug_begin(data, length, write),
			     inv_i2c_debug_end(write), length);
	}
#endif

	result = i2c_transfer(i2c_handle->client->adapter, msgs, 3);
	if (result != 3) {
		if (result >= 0)
			result = -EIO;
	} else
		result = 0;
	return result;
}


int inv_i2c_mem_read(struct invsens_i2c_t *i2c_handle, u16 i2c_addr,
	u16 mem_addr, u16 length, u8 *data)
{
	u8 bank[2];
	u8 addr[2];
	u8 buf;

	struct i2c_msg msgs[4];
	int result;

	if (!data || !i2c_handle)
		return -EINVAL;

	bank[0] = REG_BANK_SEL;
	bank[1] = mem_addr >> 8;

	addr[0] = REG_MEM_START_ADDR;
	addr[1] = mem_addr & 0xFF;

	buf = REG_MEM_RW;

	/* write message */
	msgs[0].addr = i2c_addr;
	msgs[0].flags = 0;
	msgs[0].buf = bank;
	msgs[0].len = sizeof(bank);

	msgs[1].addr = i2c_addr;
	msgs[1].flags = 0;
	msgs[1].buf = addr;
	msgs[1].len = sizeof(addr);

	msgs[2].addr = i2c_addr;
	msgs[2].flags = 0;
	msgs[2].buf = &buf;
	msgs[2].len = 1;

	msgs[3].addr = i2c_addr;
	msgs[3].flags = I2C_M_RD;
	msgs[3].buf = data;
	msgs[3].len = length;

	result = i2c_transfer(i2c_handle->client->adapter, msgs, 4);
	if (result != 4) {
		if (result >= 0)
			result = -EIO;
	} else
		result = 0;

#ifdef CONFIG_INV_SENSORS_DBG
	{
		char *read = 0;
		INVSENS_LOGD("RM%02X%02X%02X%02X - %s%s\n",
			     i2c_addr, bank[1], addr[1], length,
			     inv_i2c_debug_begin(data, length, read),
			     inv_i2c_debug_end(read));
	}
#endif

	return result;

}



int invsens_swap_array(u8 *src, u8 *dst, int len)
{
	int res = SM_SUCCESS;
	int i = 0;

	for (; i < len; i++)
		dst[i] = src[len - (i + 1)];

	return res;
}

int invsens_set_log_mask(u32 mask)
{
	invsens_log_level = mask;
	return 0;
}

u32 invsens_get_log_mask()
{
	return invsens_log_level;
}
