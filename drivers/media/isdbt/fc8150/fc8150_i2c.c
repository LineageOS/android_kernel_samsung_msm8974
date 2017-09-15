/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fc8150_i2c.c

 Description : fc8150 host interface

*******************************************************************************/
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include<linux/slab.h>

#include "fci_types.h"
#include "fc8150_regs.h"
#include "fci_oal.h"
#include "fci_hal.h"

#define HPIC_READ           0x01 /* read command */
#define HPIC_WRITE          0x02 /* write command */
#define HPIC_AINC           0x04 /* address increment */
#define HPIC_BMODE          0x00 /* byte mode */
#define HPIC_WMODE          0x10 /* word mode */
#define HPIC_LMODE          0x20 /* long mode */
#define HPIC_ENDIAN         0x00 /* little endian */
#define HPIC_CLEAR          0x80 /* currently not used */

#define CHIP_ADDR           0x58

static DEFINE_MUTEX(fci_i2c_lock);

#define I2C_M_FCIRD 1
#define I2C_M_FCIWR 0
#define I2C_MAX_SEND_LENGTH 300

struct i2c_ts_driver{
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct work;
};

struct i2c_client *fc8150_i2c;

struct i2c_driver fc8150_i2c_driver;

int fc8150_tsif_setting(HANDLE hDevice)//shubham
{
	PRINTF(NULL, "fc8150_tsif_setting called \n");
	bbm_write(hDevice, BBM_TS_CLK_DIV, 0x04); //04 as original
	bbm_write(hDevice, BBM_TS_PAUSE, 0x80);

#ifdef MSMCHIP
	PRINTF(NULL, "TSIF enable...MSM mode\n");
	bbm_write(hDevice, BBM_TS_CTRL, 0x06);	//02 as original
	bbm_write(hDevice, BBM_TS_SEL, 0x84);
#else
	PRINTF(NULL, "TSIF enable...normal mode\n");
	bbm_write(hDevice, BBM_TS_CTRL, 0x00);
	bbm_write(hDevice, BBM_TS_SEL, 0x83);
#endif
PRINTF(NULL, "fc8150_tsif_setting completed \n");

return BBM_OK;
}


static int fc8150_i2c_probe(struct i2c_client *i2c_client,
	const struct i2c_device_id *id)
{
	PRINTF(NULL, "fc8150_i2c_probe called \n");
	fc8150_i2c = i2c_client;
	i2c_set_clientdata(i2c_client, NULL);
	

	PRINTF(NULL, "fc8150_i2c_probe OK \n");
	return 0;
}

static int fc8150_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id fc8150_id[] = {
	{ "fc8150_i2c", 0 },
	{ },
};

static struct of_device_id fc8150_match_table[] = {
        { .compatible = "isdb,isdb_fc8300",},
        { },
};
struct i2c_driver fc8150_i2c_driver = {
	.driver = {
			.name = "fc8150_i2c",
			.owner = THIS_MODULE,
			.of_match_table = fc8150_match_table,
		},
	.probe    = fc8150_i2c_probe,
	.remove   = fc8150_remove,
	.id_table = fc8150_id,
};

static int i2c_bulkread(HANDLE hDevice, u8 chip, u8 addr, u8 *data, u16 length)
{
	int res;
	struct i2c_msg rmsg[2];
	unsigned char i2c_data[1];

	if (fc8150_i2c == NULL)	{
		PRINTF(0, "[ERROR] FC8150_I2C Handle Fail...........\n");
		return BBM_NOK;
	}

	rmsg[0].addr = chip;
	rmsg[0].flags = I2C_M_FCIWR;
	rmsg[0].len = 1;
	rmsg[0].buf = i2c_data;
	i2c_data[0] = addr & 0xff;

	rmsg[1].addr = chip;
	rmsg[1].flags = I2C_M_FCIRD;
	rmsg[1].len = length;
	rmsg[1].buf = data;
	res = i2c_transfer(fc8150_i2c->adapter, &rmsg[0], 2);

	return res;
}


static int i2c_bulkwrite(HANDLE hDevice, u8 chip, u8 addr, u8 *data, u16 length)
{
	int res;
	struct i2c_msg wmsg;
	unsigned char i2c_data[I2C_MAX_SEND_LENGTH];

	if (fc8150_i2c == NULL)	{
		PRINTF(0, "[ERROR] FC8150_I2C Handle Fail...........\n");
		return BBM_NOK;
	}


	if (length+1 > I2C_MAX_SEND_LENGTH)	{
		PRINTF(0, ".......error");
		return -ENODEV;
	}
	wmsg.addr = chip;
	wmsg.flags = I2C_M_FCIWR;
	wmsg.len = length + 1;
	wmsg.buf = i2c_data;

	i2c_data[0] = addr & 0xff;
	memcpy(&i2c_data[1], data, length);

	res = i2c_transfer(fc8150_i2c->adapter, &wmsg, 1);

	return res;
}


static int i2c_dataread(HANDLE hDevice, u8 chip, u8 addr, u8 *data, u32 length)
{
	return i2c_bulkread(hDevice, chip, addr, data, length);
}

int fc8150_bypass_read(HANDLE hDevice, u8 chip, u8 addr, u8 *data, u16 length)
{
	int res;
	u8 bypass_addr = 0x03;
	u8 bypass_data = 1;
	u8 bypass_len  = 1;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(hDevice, CHIP_ADDR, bypass_addr
		, &bypass_data, bypass_len);
	res |= i2c_bulkread(hDevice, chip, addr, data, length);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

int fc8150_bypass_write(HANDLE hDevice, u8 chip, u8 addr, u8 *data, u16 length)
{
	int res;
	u8 bypass_addr = 0x03;
	u8 bypass_data = 1;
	u8 bypass_len  = 1;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(hDevice, CHIP_ADDR, bypass_addr
		, &bypass_data, bypass_len);
	res |= i2c_bulkwrite(hDevice, chip, addr, data, length);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

int fc8150_i2c_init(HANDLE hDevice, u16 param1, u16 param2)
{
	int res;
	PRINTF(NULL, "fc8150_i2c_init \n");
	fc8150_i2c = kzalloc(sizeof(struct i2c_ts_driver), GFP_KERNEL); //shubham

	if (fc8150_i2c == NULL)	//shubham
		return -ENOMEM;
		
	res = i2c_add_driver(&fc8150_i2c_driver);

	fc8150_tsif_setting(hDevice);

	PRINTF(NULL, "fc8150_i2c_init exit\n");
	return res;
}


int fc8150_i2c_byteread(HANDLE hDevice, u16 addr, u8 *data)
{
	int res;
	u8 command = HPIC_READ | HPIC_BMODE | HPIC_ENDIAN;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_ADDRESS_REG
		, (u8 *)&addr, 2);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkread(hDevice, CHIP_ADDR, BBM_DATA_REG, data, 1);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

int fc8150_i2c_wordread(HANDLE hDevice, u16 addr, u16 *data)
{
	int res;
	u8 command = HPIC_READ | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_ADDRESS_REG
		, (u8 *)&addr, 2);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkread(hDevice, CHIP_ADDR, BBM_DATA_REG, (u8 *)data, 2);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

int fc8150_i2c_longread(HANDLE hDevice, u16 addr, u32 *data)
{
	int res;
	u8 command = HPIC_READ | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_ADDRESS_REG
		, (u8 *)&addr, 2);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkread(hDevice, CHIP_ADDR, BBM_DATA_REG, (u8 *)data, 4);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

int fc8150_i2c_bulkread(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	int res;
	u8 command = HPIC_READ | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_ADDRESS_REG
		, (u8 *)&addr, 2);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkread(hDevice, CHIP_ADDR, BBM_DATA_REG, data, length);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

int fc8150_i2c_bytewrite(HANDLE hDevice, u16 addr, u8 data)
{
	int res;
	u8 command = HPIC_WRITE | HPIC_BMODE | HPIC_ENDIAN;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_ADDRESS_REG
		, (u8 *)&addr, 2);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_DATA_REG, (u8 *)&data, 1);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

int fc8150_i2c_wordwrite(HANDLE hDevice, u16 addr, u16 data)
{
	int res;
	u8 command = HPIC_WRITE | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_ADDRESS_REG
		, (u8 *)&addr, 2);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_DATA_REG, (u8 *)&data, 2);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

int fc8150_i2c_longwrite(HANDLE hDevice, u16 addr, u32 data)
{
	int res;
	u8 command = HPIC_WRITE | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_ADDRESS_REG
		, (u8 *)&addr, 2);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_DATA_REG, (u8 *)&data, 4);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

int fc8150_i2c_bulkwrite(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	int res;
	u8 command = HPIC_WRITE | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_ADDRESS_REG
		, (u8 *)&addr, 2);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_DATA_REG, data, length);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

int fc8150_i2c_dataread(HANDLE hDevice, u16 addr, u8 *data, u32 length)
{
	int res;
	u8 command = HPIC_READ | HPIC_BMODE | HPIC_ENDIAN;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_ADDRESS_REG
		, (u8 *)&addr, 2);
	res |= i2c_bulkwrite(hDevice, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_dataread(hDevice, CHIP_ADDR, BBM_DATA_REG, data, length);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

int fc8150_i2c_deinit(HANDLE hDevice)
{
	bbm_write(hDevice, BBM_TS_SEL, 0x00);
	PRINTF(NULL, "fc8150_i2c_deinit \n");
	i2c_del_driver(&fc8150_i2c_driver);

	return BBM_OK;
}
