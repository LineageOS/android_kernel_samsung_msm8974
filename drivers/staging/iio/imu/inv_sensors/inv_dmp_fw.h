#ifndef _INV_DMP_FW_H_
#define _INV_DMP_FW_H_

#define DMP_CIPHER_KEY_LEN (32)


int invdmp_download_fw(const char *filename, int offset,
		       struct invsens_i2c_t *i2c_handle, u16 i2c_addr,
		       const char *key, struct invdmp_fw_data_t *fw_data);


#endif
