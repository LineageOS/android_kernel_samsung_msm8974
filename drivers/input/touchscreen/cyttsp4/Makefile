### CYTTSP
#obj-m	+= cyttsp4.o
obj-$(CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4) += cyttsp4_core.o
obj-$(CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4) += cyttsp4_i2c.o
obj-$(CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4) += cyttsp4_platform.o
obj-$(CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4) += cyttsp4_devtree.o
obj-$(CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4) += cyttsp4_mt_common.o
obj-$(CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4) += cyttsp4_mtb.o
obj-$(CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4) += cyttsp4_btn.o
obj-$(CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4) += cyttsp4_debug.o
obj-$(CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4) += cyttsp4_device_access.o
obj-$(CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4) += cyttsp4_loader.o
obj-$(CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4) += cyttsp4_samsung_sysfs.o

CY_CFLAG_ADD =\
	-DCONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4\
	-DCONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_DEVICETREE_SUPPORT\
	-DCONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_BUTTON\
	-DCONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_DEVICE_ACCESS\
	-DCONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_LOADER\
	-DCONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_PLATFORM_FW_UPGRADE\
	-DCONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_BINARY_FW_UPGRADE

CY_CFLAG_REMOVE =\
	-DCYTTSP4_PDATA_IN_PLATFORM_C\
	-DCONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_PROXIMITY\
	-DCONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_PLATFORM_TTCONFIG_UPGRADE\
	-DCONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_DEVICE_ACCESS_API\
	-DCONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_DEBUG\
	-DCONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_MANUAL_TTCONFIG_UPGRADE
	
CFLAGS_cyttsp4_core.o 			+= $(CY_CFLAG_ADD) -DDEBUG
CFLAGS_cyttsp4_i2c.o 			+= $(CY_CFLAG_ADD) -DDEBUG
CFLAGS_cyttsp4_spi.o 			+= $(CY_CFLAG_ADD)
CFLAGS_cyttsp4_mta.o 			+= $(CY_CFLAG_ADD)
CFLAGS_cyttsp4_mtb.o 			+= $(CY_CFLAG_ADD)
CFLAGS_cyttsp4_mt_a.o 			+= $(CY_CFLAG_ADD)
CFLAGS_cyttsp4_mt_b.o 			+= $(CY_CFLAG_ADD)
CFLAGS_cyttsp4_mt_common.o 		+= $(CY_CFLAG_ADD)
CFLAGS_cyttsp4_btn.o 			+= $(CY_CFLAG_ADD)
CFLAGS_cyttsp4_proximity.o 		+= $(CY_CFLAG_ADD)
CFLAGS_cyttsp4_samsung_sysfs.o  += $(CY_CFLAG_ADD)
CFLAGS_cyttsp4_device_access.o 	+= $(CY_CFLAG_ADD)
CFLAGS_cyttsp4_test_device_access_api.o += $(CY_CFLAG_ADD)
CFLAGS_cyttsp4_loader.o 		+= $(CY_CFLAG_ADD)
CFLAGS_cyttsp4_debug.o 			+= $(CY_CFLAG_ADD)
CFLAGS_cyttsp4_devtree.o 		+= $(CY_CFLAG_ADD) -DDEBUG
CFLAGS_cyttsp4_platform.o 		+= $(CY_CFLAG_ADD) -DDEBUG

CFLAGS_cyttsp4_core.o 			:= $(filter-out $(CY_CFLAG_REMOVE), $(CFLAGS_cyttsp4_core.o))
CFLAGS_cyttsp4_i2c.o 			:= $(filter-out $(CY_CFLAG_REMOVE), $(CFLAGS_cyttsp4_i2c.o))
CFLAGS_cyttsp4_spi.o 			:= $(filter-out $(CY_CFLAG_REMOVE), $(CFLAGS_cyttsp4_spi.o))
CFLAGS_cyttsp4_mta.o 			:= $(filter-out $(CY_CFLAG_REMOVE), $(CFLAGS_cyttsp4_mta.o))
CFLAGS_cyttsp4_mtb.o 			:= $(filter-out $(CY_CFLAG_REMOVE), $(CFLAGS_cyttsp4_mtb.o))
CFLAGS_cyttsp4_mt_a.o 			:= $(filter-out $(CY_CFLAG_REMOVE), $(CFLAGS_cyttsp4_mt_a.o))
CFLAGS_cyttsp4_mt_b.o 			:= $(filter-out $(CY_CFLAG_REMOVE), $(CFLAGS_cyttsp4_mt_b.o))
CFLAGS_cyttsp4_mt_common.o 		:= $(filter-out $(CY_CFLAG_REMOVE), $(CFLAGS_cyttsp4_mt_common.o))
CFLAGS_cyttsp4_btn.o 			:= $(filter-out $(CY_CFLAG_REMOVE), $(CFLAGS_cyttsp4_btn.o))
CFLAGS_cyttsp4_proximity.o 		:= $(filter-out $(CY_CFLAG_REMOVE), $(CFLAGS_cyttsp4_proximity.o))
CFLAGS_cyttsp4_samsung_sysfs.o		:= $(filter-out $(CY_CFLAG_REMOVE), $(CFLAGS_cyttsp4_samsung_sysfs.o))
CFLAGS_cyttsp4_device_access.o 		:= $(filter-out $(CY_CFLAG_REMOVE), $(CFLAGS_cyttsp4_device_access.o))
CFLAGS_cyttsp4_test_device_access_api.o := $(filter-out $(CY_CFLAG_REMOVE), $(CFLAGS_cyttsp4_test_device_access_api.o))
CFLAGS_cyttsp4_loader.o 		:= $(filter-out $(CY_CFLAG_REMOVE), $(CFLAGS_cyttsp4_loader.o))
CFLAGS_cyttsp4_debug.o			:= $(filter-out $(CY_CFLAG_REMOVE), $(CFLAGS_cyttsp4_debug.o))
CFLAGS_cyttsp4_devtree.o 		:= $(filter-out $(CY_CFLAG_REMOVE), $(CFLAGS_cyttsp4_devtree.o))
CFLAGS_cyttsp4_platform.o 		:= $(filter-out $(CY_CFLAG_REMOVE), $(CFLAGS_cyttsp4_platform.o))
