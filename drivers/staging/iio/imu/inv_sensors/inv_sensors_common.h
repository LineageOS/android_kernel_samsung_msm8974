#ifndef _INV_SENSORS_COMMON_H_
#define _INV_SENSORS_COMMON_H_

#include "inv_sensors_iio.h"
#include "inv_sensors_sm.h"



#define INV_MIN(a, b) ((a < b) ? a : b)
#define INV_MAX(a, b) ((a > b) ? a : b)

struct invsens_i2c_t {
	struct i2c_client *client;
};


#define i2c_write_single_base(dcfg, i2c_addr, reg, value, ret, goto_error) \
	if (unlikely((ret = inv_i2c_single_write(dcfg->i2c_handle,\
		i2c_addr, reg, value)))){\
		pr_info("[inv] i2c error(%s:%d) err = %d\n",\
			__func__, __LINE__, ret);\
		goto goto_error;\
	}

#define i2c_read_base(dcfg, i2c_addr, reg, length, data, ret, goto_error) \
	if (unlikely((ret = inv_i2c_read(dcfg->i2c_handle, i2c_addr,\
		reg, length, data)))){\
		pr_info("[inv] i2c error(%s:%d) err = %d\n",\
			__func__, __LINE__, ret);\
		goto goto_error;\
	}

#define i2c_write_single(dcfg, reg, value, ret, goto_error) \
	i2c_write_single_base(dcfg, dcfg->i2c_addr,\
		reg, value, ret, goto_error)

#define i2c_read(dcfg, reg, length, data, ret, goto_error) \
	i2c_read_base(dcfg, dcfg->i2c_addr,\
		reg, length, data, ret, goto_error)

#define i2c_write_single_reg(dcfg, reg, value) \
	inv_i2c_single_write(dcfg->i2c_handle, dcfg->i2c_addr, reg, value)

#define i2c_read_reg(dcfg, reg, length, data)  \
	inv_i2c_read(dcfg->i2c_handle, dcfg->i2c_addr, reg, length, data)



/*i2c apis*/
int inv_i2c_read(struct invsens_i2c_t *i2c_handle,
		 u16 i2c_addr, u8 reg, u16 length, u8 *data);
int inv_i2c_single_write(struct invsens_i2c_t *i2c_handle,
		 u16 i2c_addr, u8 reg, u8 data);
int inv_i2c_mem_write(struct invsens_i2c_t *i2c_handle,
	      u16 i2c_addr, u16 mem_addr, u16 length, u8 *data);
int inv_i2c_mem_read(struct invsens_i2c_t *i2c_handle,
	     u16 i2c_addr, u16 mem_addr, u16 length, u8 *data);


/*utils*/
int invsens_swap_array(u8 *src, u8 *dst, int len);
int invsens_set_log_mask(u32 mask);
u32 invsens_get_log_mask(void);


#define INVSENS_LOG_HIGH	0x01
#define INVSENS_LOG_MID		0x02
#define INVSENS_LOG_LOW	0x04
#define INVSENS_LOG_DEBUG	0x08
#define INVSENS_LOG_ERR		0x10
#define INVSENS_LOG_ENTRY	0x20
#define INVSENS_LOG_INFO	0x40
#define INVSENS_LOG_ALL		0xff


#ifdef CONFIG_INV_SENSORS_DBG

/*log definition*/
#define INVSENS_LOGD(...) \
	if (unlikely(invsens_get_log_mask() & INVSENS_LOG_DEBUG))\
		pr_info("[INV]D:"__VA_ARGS__)
#define INVSENS_LOGI(...) \
	if (unlikely(invsens_get_log_mask() & INVSENS_LOG_INFO))\
		pr_info("[INV]I:"__VA_ARGS__)
#define INVSENS_LOGE(...) \
	if(unlikely(invsens_get_log_mask() & INVSENS_LOG_ERR))\
		pr_info("[INV]E:"__VA_ARGS__)

#define INV_DBG_FUNC_NAME \
	INVSENS_LOGI("%s\n", __func__);

#else /*CONFIG_INV_SENSORS_DBG*/


#define INVSENS_LOGD(...)
#define INVSENS_LOGI(...)
#define INVSENS_LOGE(...)

#define INV_DBG_FUNC_NAME
#endif


#endif /*_INV_SENSORS_COMMON_H_*/
