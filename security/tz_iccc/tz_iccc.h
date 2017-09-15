/*
 * @file tz_iccc.h
 * @brief header file for kernel tz_iccc
 * Copyright (c) 2015, Samsung Electronics Corporation. All rights reserved.
 */
#ifndef TZ_ICCC_H
#define TZ_ICCC_H

/* ICCC Implementation in Kernel */



#define MIN_PADDING_NEEDED 9

/* ICCC TZApp name */
#define ICCC_TZAPP_NAME	"tz_iccc"

/* Defining ICCC related CMD IDs for TZ communication*/
#define CMD_ICCC_INIT           0x00000001
#define CMD_ICCC_SAVEDATA_KERN  0x00000002
#define CMD_ICCC_READDATA_KERN  0x00000003

/* Buffer length to be used for ICCC TZ communication*/
#define ICCC_QSEE_BUFFER_LENGTH		1024

/* QSEECOM driver related MACROS*/
#define QSEECOM_ALIGN_SIZE  0x40
#define QSEECOM_ALIGN_MASK  (QSEECOM_ALIGN_SIZE - 1)
#define QSEECOM_ALIGN(x)    \
    ((x + QSEECOM_ALIGN_SIZE) & (~QSEECOM_ALIGN_MASK))

/* structures for ICCC QSEE communication*/

typedef struct tz_msg_header {
	/* * First 4 bytes should always be id: either cmd_id or resp_id */
	uint32_t id;
	uint32_t content_id;
	uint32_t len;
	uint32_t status;
} __attribute__ ((packed)) tz_msg_header_t;

typedef struct iccc_req_s {
	uint32_t cmd_id;
	uint32_t type;
	uint32_t value;
    uint32_t padding[MIN_PADDING_NEEDED]; //only padding , just to make tciMessage_t's size >= 64
} __attribute__ ((packed)) iccc_req_t;

typedef struct iccc_rsp_s {
	uint32_t cmd_id;
	uint32_t type;
	uint32_t value;
	int ret;
} __attribute__ ((packed)) iccc_rsp_t;

typedef struct {
	union content_u {
        iccc_req_t iccc_req;
        iccc_rsp_t iccc_rsp;
    } __attribute__ ((packed)) content;
} __attribute__ ((packed)) tima_iccc_generic_payload_t;

typedef struct {
	tz_msg_header_t header;

    union payload_u {
        tima_iccc_generic_payload_t generic;
    } __attribute__ ((packed)) payload;
} __attribute__ ((packed)) tima_iccc_message_t;

typedef tima_iccc_message_t tciMessage_t;

/* structures for QSEECOM driver interaction*/
struct qseecom_handle {
    void *dev; /* in/out */
    unsigned char *sbuf; /* in/out */
    uint32_t sbuf_len; /* in/out */
};

/* declaring QSEECOM driver related functions*/

extern int qseecom_start_app(struct qseecom_handle **handle, char *app_name, uint32_t size);
extern int qseecom_shutdown_app(struct qseecom_handle **handle);
extern int qseecom_send_command(struct qseecom_handle *handle, void *send_buf, uint32_t sbuf_len, void *resp_buf, uint32_t rbuf_len);
extern int qseecom_set_bandwidth(struct qseecom_handle *handle, bool high);

#endif	/* TIMA_UEVENT_H */
