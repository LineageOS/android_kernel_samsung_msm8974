TARGET_NAME := mwlan_aarp
TARGET_KO := $(TARGET_NAME).ko

CLIB := .

obj-m += $(TARGET_NAME).o
$(TARGET_NAME)-objs := \
	a.o\

EXTRA_CFLAGS += -Wall -I $(PWD)/$(CLIB)
EXTRA_CFLAGS += $(_CFLAGS)
# only over the Android 5.0
EXTRA_CFLAGS += -DPLAT_VER=$(shell printf 0x50000)
EXTRA_CFLAGS += -DMODEL_$(subst -,_,$(PRODUCT_MODEL))
# end

all:
	make -C $(SRCPATH) M=$(PWD) modules

clean:
	make -C $(SRCPATH) M=$(PWD) clean
