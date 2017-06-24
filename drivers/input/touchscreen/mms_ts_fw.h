/* Melfas MMS-100 seies firmware list */

#if defined(CONFIG_TOUCHSCREEN_MMS144)

/* 4.8" OCTA LCD */
#define FW_VERSION 0xC7
#include "d2_fw.h"
/* 4.65" OCTA LCD */
#define FW_465_VERSION 0xA8
#include "d2_465_fw.h"
#if defined(CONFIG_MIPI_SAMSUNG_ESD_REFRESH)
extern void set_esd_enable(void);
extern void set_esd_disable(void);
#endif

#endif
