#Android makefile to build kernel as a part of Android Build
PERL		= perl

KERNEL_TARGET := $(strip $(INSTALLED_KERNEL_TARGET))
ifeq ($(KERNEL_TARGET),)
INSTALLED_KERNEL_TARGET := $(PRODUCT_OUT)/kernel
endif

ifeq ($(TARGET_PREBUILT_KERNEL),)

KERNEL_OUT := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ
KERNEL_CONFIG := $(KERNEL_OUT)/.config
ifeq ($(TARGET_KERNEL_APPEND_DTB), true)
TARGET_PREBUILT_INT_KERNEL := $(KERNEL_OUT)/arch/arm/boot/zImage-dtb
else
TARGET_PREBUILT_INT_KERNEL := $(KERNEL_OUT)/arch/arm/boot/zImage
endif
KERNEL_HEADERS_INSTALL := $(KERNEL_OUT)/usr
KERNEL_MODULES_INSTALL := system
KERNEL_MODULES_OUT := $(TARGET_OUT)/lib/modules
KERNEL_IMG=$(KERNEL_OUT)/arch/arm/boot/Image

USE_MODULE ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MODULES=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))

DTS_NAMES ?= $(shell $(PERL) -e 'while (<>) {$$a = $$1 if /CONFIG_ARCH_((?:MSM|QSD|MPQ)[a-zA-Z0-9]+)=y/; $$r = $$1 if /CONFIG_MSM_SOC_REV_(?!NONE)(\w+)=y/; $$arch = $$arch.lc("$$a$$r ") if /CONFIG_ARCH_((?:MSM|QSD|MPQ)[a-zA-Z0-9]+)=y/} print $$arch;' $(KERNEL_CONFIG))
KERNEL_USE_OF ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_USE_OF=y/) { $$of = "y"; break; } } print $$of;' kernel/arch/arm/configs/$(KERNEL_DEFCONFIG))
LOCALE_KOR ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_LOCALE_KOR=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
LOCALE_JPN ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_LOCALE_JPN=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
JS_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_JS_PROJECT=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
JS_TW_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_JS01LTEZT=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
LOCALE_CHN_DUOS ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_H3GDUOS=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
LOCALE_CHN_DUOS_CU ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_H3GDUOS_CU=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
K_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_K_PROJECT=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
S_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_S_PROJECT=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
PATEK_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_PATEK_PROJECT=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
LOCALE_JSGLTE_CHN_CMCC ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_JSGLTE_CHN_CMCC=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
LOCALE_MACH_H3G_CHN_CMCC ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_H3G_CHN_CMCC=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
LOCALE_MACH_H3G_CHN_OPEN ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_H3G_CHN_OPEN=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
LOCALE_MACH_HLTE_CHN_CMCC ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_HLTE_CHN_CMCC=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
H_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_H_PROJECT=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
FRESCO_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_FRESCO_PROJECT=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
F_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_F_PROJECT=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
MONDRIAN_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_MONDRIAN_PROJECT=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
VIENNA_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_VIENNA_PROJECT=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
LT03_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_LT03_PROJECT=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
MILLET3G_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_MILLET3G_EUR=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
MILLET3G_CHN_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_MILLET3G_CHN_OPEN=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
MILLETLTE_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_MILLETLTE_OPEN=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
MILLETLTE_ATT_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_MILLETLTE_ATT=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
MILLETLTE_CAN_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_MILLETLTE_CAN=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
MILLETLTE_TMO_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_MILLETLTE_TMO=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
MILLETLTE_VZW_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_MILLETLTE_VZW=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
MILLETLTE_KOR_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_MILLETLTE_KOR=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
MILLETWIFI_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_MILLETWIFI_COMMON=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
MATISSE3G_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_MATISSE3G_OPEN=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
MATISSEWIFI_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_MATISSEWIFI_COMMON=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
MATISSELTE_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_MATISSELTE_OPEN=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
MATISSELTE_ATT_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_MATISSELTE_ATT=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
MATISSELTE_VZW_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_MATISSELTE_VZW=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
MATISSELTE_USC_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_MATISSELTE_USC=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
AFYONLTE_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_AFYONLTE_COMMON=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
ATLANTICLTE_ATT_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_ATLANTICLTE_ATT=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
ATLANTICLTE_VZW_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_ATLANTICLTE_VZW=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
ATLANTIC3G_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_ATLANTIC3G_COMMON=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
ATLANTICLTE_USC_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_ATLANTICLTE_USC=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
PICASSO_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_PICASSO_PROJECT=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
CHAGALL_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_CHAGALL_PROJECT=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
KLIMT_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_KLIMT_PROJECT=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
V2_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_V2_PROJECT=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
KS01_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_KS01_PROJECT=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
JACTIVE_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_JACTIVE_PROJECT=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
KACTIVE_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_KACTIVE_PROJECT=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
HEAT_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_HEAT_PROJECT=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
BERLUTI3G_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_BERLUTI3G_EUR=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
BERLUTILTE_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_BERLUTILTE_EUR=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
GNOTEDS_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_BERLUTILTE_EUR=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
HEAT_DYN_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_HEAT_DYN=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
HEAT_AIO_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_HEAT_AIO=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
VICTORLTE_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_VICTORLTE=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
VASTALTE_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_VASTALTE=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
FRESCONEOLTE_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_FRESCONEOLTE_CTC=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
KANAS3G_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_KANAS3G=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
KANAS3G_CMCC_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_KANAS3G_CMCC=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
KANAS3G_CTC_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_KANAS3G_CTC=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
DEGASLTE_SPR_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_DEGASLTE_SPR=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
DEGASLTE_VZW_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_DEGASLTE_VZW=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
MEGA23G_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_MEGA23G_COMMON=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
MEGA2LTE_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_SEC_MEGA2LTE_COMMON=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
GNOTELTEDS_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_GNOTELTEDS_OPEN=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
T10_3G_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_T10_3G_OPEN=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
T8_3G_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_T8_3G_OPEN=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
T10_WIFI_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_T10_WIFI_OPEN=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
RUBENSLTE_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_RUBENSLTE_OPEN=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
RUBENSWIFI_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_RUBENSWIFI_OPEN=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
VASTALTE_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_VASTALTE_CHN_CMCC_DUOS=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
CHAGALL_DCM_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_CHAGALL_DCM=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
KACTIVE_SKT_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_KACTIVELTE_SKT=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
KACTIVE_DCM_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_KACTIVELTE_DCM=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))
KACTIVE_KOR_PROJECT ?= $(shell $(PERL) -e '$$of = "n"; while (<>) { if (/CONFIG_MACH_KACTIVELTE_KOR=y/) { $$of = "y"; break; } } print $$of;' $(KERNEL_CONFIG))


ifeq "$(KERNEL_USE_OF)" "y"
ifeq "$(K_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974pro/$(DTS_NAME)pro-sec*.dts)
endif
ifeq "$(S_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974pro/$(DTS_NAME)pro-sec*.dts)
endif
ifeq "$(PATEK_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974pro/$(DTS_NAME)pro-sec*.dts)
endif
ifeq "$(VIENNA_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-vienna*.dts)
endif
ifeq "$(LT03_PROJECT)" "y"
    ifeq "$(LOCALE_KOR)" "y"
        DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-lt03kor*.dts)
    else
        DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-lt03*.dts)
    endif
endif
ifeq "$(MILLET3G_PROJECT)" "y"
    ifeq "$(MILLET3G_CHN_PROJECT)" "y"
        DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/$(DTS_NAME)-sec-millet3g-chn-open*.dts)
    else
        DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/$(DTS_NAME)-sec-millet3geur*.dts)
    endif
endif
ifeq "$(MILLETLTE_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/msm8926-sec-milletlte-*.dts)
endif
ifeq "$(MILLETLTE_ATT_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/msm8926-sec-milletlteatt*.dts)
endif
ifeq "$(MILLETLTE_CAN_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/msm8926-sec-milletltecan*.dts)
endif
ifeq "$(MILLETLTE_TMO_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/msm8926-sec-milletlte_tmo*.dts)
endif
ifeq "$(MILLETLTE_VZW_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/msm8926-sec-milletltevzw*.dts)
endif
ifeq "$(MILLETLTE_KOR_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/msm8926-sec-milletltekor*.dts)
endif
ifeq "$(MILLETWIFI_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/$(DTS_NAME)-sec-milletwifieur*.dts)
endif
ifeq "$(RUBENSWIFI_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/$(DTS_NAME)-sec-rubenswifieur*.dts)
endif
ifeq "$(MATISSE3G_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/$(DTS_NAME)-sec-matisse3g-*.dts)
endif
ifeq "$(MATISSELTE_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/msm8926-sec-matisselte-*.dts)
endif
ifeq "$(MATISSELTE_ATT_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/msm8926-sec-matisselteatt*.dts)
endif
ifeq "$(MATISSELTE_VZW_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/msm8926-sec-matisseltevzw*.dts)
endif
ifeq "$(MATISSELTE_USC_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/msm8926-sec-matisselteusc*.dts)
endif
ifeq "$(AFYONLTE_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/msm8926-sec-afyonlte*.dts)
endif
ifeq "$(ATLANTICLTE_ATT_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/msm8928-sec-atlanticlteatt*.dts)
endif
ifeq "$(ATLANTICLTE_VZW_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/msm8928-sec-atlanticltevzw*.dts)
endif
ifeq "$(ATLANTIC3G_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/msm8228-sec-atlantic3g*.dts)
endif
ifeq "$(ATLANTICLTE_USC_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/msm8928-sec-atlanticlteusc*.dts)
endif
ifeq "$(BERLUTI3G_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/$(DTS_NAME)-sec-berluti3geur*.dts)
endif
ifeq "$(BERLUTILTE_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/msm8926-sec-berlutilte*.dts)
endif
ifeq "$(GNOTEDS_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/msm8926-sec-gnoteds*.dts)
endif
ifeq "$(MATISSEWIFI_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/msm8226-sec-matissewifi*.dts)
endif
ifeq "$(KANAS3G_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8610/$(DTS_NAME)-sec-kanas3g*.dts)
endif
ifeq "$(KANAS3G_CMCC_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8610/$(DTS_NAME)-sec-kanas3g-chn-cmcc*.dts)
endif
ifeq "$(KANAS3G_CTC_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8610/$(DTS_NAME)-sec-kanas3g-chn-ctc*.dts)
endif
ifeq "$(VICTORLTE_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/$(DTS_NAME)-sec-victorlte*.dts)
endif
ifeq "$(VASTALTE_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/$(DTS_NAME)-sec-vastalteduos*.dts)
endif
ifeq "$(DEGASLTE_SPR_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/msm8926-sec-degasltespr*.dts)
endif
ifeq "$(DEGASLTE_VZW_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/msm8926-sec-degasltevzw*.dts)
endif
ifeq "$(RUBENSLTE_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/msm8926-sec-rubenslteeur*.dts)
endif
ifeq "$(T10_3G_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/$(DTS_NAME)-sec-t10_3g-*.dts)
endif
ifeq "$(T8_3G_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/$(DTS_NAME)-sec-t8_3g-*.dts)
endif
ifeq "$(T10_WIFI_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/$(DTS_NAME)-sec-t10_wifi-*.dts)
endif
ifeq "$(HEAT_PROJECT)" "y"
	ifeq "$(HEAT_DYN_PROJECT)" "y"
		DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8610/$(DTS_NAME)-sec-heat-dyn*.dts)
	else
		ifeq "$(HEAT_AIO_PROJECT)" "y"
			DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8610/$(DTS_NAME)-sec-heat-aio*.dts)
		else
			DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8610/$(DTS_NAME)-sec-heat-tfnvzw*.dts)
		endif
	endif
endif
ifeq "$(MEGA23G_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/msm8228-sec-mega23g*.dts)
endif
ifeq "$(MEGA2LTE_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8226/msm8928-sec-mega2lte*.dts)
endif
$(info printing $(DTS_NAMES))
$(info printing $(DTS_FILES))

ifeq "$(FRESCO_PROJECT)" "y"
	ifeq "$(LOCALE_KOR)" "y"
		DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-frescolltekor*.dts)
	else
		DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-frescolltekor*.dts)
	endif
endif
ifeq "$(H_PROJECT)" "y"
	ifeq "$(LOCALE_JPN)" "y"
		DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-hltejpn*.dts)
	else
		ifeq "$(LOCALE_KOR)" "y"
			DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-hltekor*.dts)
		else
			ifeq "$(LOCALE_CHN_DUOS)" "y"
				ifeq "$(LOCALE_CHN_DUOS_CU)" "y"
					DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-h3gchncu*.dts)
				else
					DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-h3gchnduos*.dts)
				endif
			else
				ifeq "$(LOCALE_JSGLTE_CHN_CMCC)" "y" 
					DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-jsglte-*.dts)
				else
					ifeq "$(LOCALE_MACH_H3G_CHN_CMCC)" "y" 
						DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-hlte-chn-r06.dts)
						DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-hlte-chn-r07.dts)
						DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-hlte-chn-r09.dts)
					else 
						ifeq "$(LOCALE_MACH_H3G_CHN_OPEN)" "y" 
							DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-hlte-chn-r03.dts)
							DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-hlte-chn-r07.dts)
							DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-hlte-chn-r09.dts)
						else
							ifeq "$(LOCALE_MACH_HLTE_CHN_CMCC)" "y" 
								DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-hlte-chn-r07.dts)
								DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-hlte-chn-r09.dts)
							else
								DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-hlte-r*.dts)
							endif
						endif
					endif
				endif
			endif
		endif
	endif
endif
ifeq "$(F_PROJECT)" "y"
		DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-fltekor*.dts)
endif
ifeq "$(PICASSO_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-picasso*.dts)
endif
ifeq "$(CHAGALL_PROJECT)" "y"
	ifeq "$(CONFIG_MACH_CHAGALL_KDI)" "y"
		DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974pro/$(DTS_NAME)pro-ac-sec-chagall*.dts)
	else ifeq "$(CHAGALL_DCM_PROJECT)" "y"
		DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-chagalljpn*.dts)
	else
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-chagall-r*.dts)
endif
endif
ifeq "$(KLIMT_PROJECT)" "y"
	ifeq "$(LOCALE_JPN)" "y"
		DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974pro/$(DTS_NAME)-sec-klimtjpn*.dts)
	else
		DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-klimt*.dts)
	endif
endif
ifeq "$(V2_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-v2*.dts)
endif
ifeq "$(KS01_PROJECT)" "y"
	ifeq "$(LOCALE_KOR)" "y"
		DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-ks01lte*.dts)
	endif
endif
ifeq "$(JACTIVE_PROJECT)" "y"
DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/$(DTS_NAME)-sec-jactive*.dts)
endif
ifeq "$(JS_PROJECT)" "y"
	ifeq "$(LOCALE_JPN)" "y"
		ifeq "$(JS_TW_PROJECT)" "y"
                       DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-js01ltetw*.dts)
	        else
		       DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974/$(DTS_NAME)-sec-js01ltejpn*.dts)
                endif
	endif
endif
ifeq "$(KACTIVE_PROJECT)" "y"
	ifeq "$(KACTIVE_SKT_PROJECT)" "y"
		DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974pro/$(DTS_NAME)pro-ac-sec-kactivelteskt*.dts)
	else ifeq "$(KACTIVE_DCM_PROJECT)" "y"
		DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974pro/$(DTS_NAME)pro-ac-sec-kactiveltedcm*.dts)
	else ifeq "$(KACTIVE_KOR_PROJECT)" "y"
		DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974pro/$(DTS_NAME)pro-ac-sec-kactiveltekor*.dts)
	else
		DTS_FILES = $(wildcard $(TOP)/kernel/arch/arm/boot/dts/msm8974pro/$(DTS_NAME)pro-ac-sec-kactivelte-*.dts)
	endif
endif
DTS_FILE = $(lastword $(subst /, ,$(1)))
DTB_FILE = $(addprefix $(KERNEL_OUT)/arch/arm/boot/,$(patsubst %.dts,%.dtb,$(call DTS_FILE,$(1))))
ZIMG_FILE = $(addprefix $(KERNEL_OUT)/arch/arm/boot/,$(patsubst %.dts,%-zImage,$(call DTS_FILE,$(1))))
KERNEL_ZIMG = $(KERNEL_OUT)/arch/arm/boot/zImage
DTC = $(KERNEL_OUT)/scripts/dtc/dtc

define append-dtb
mkdir -p $(KERNEL_OUT)/arch/arm/boot;\
$(foreach DTS_NAME, $(DTS_NAMES), \
   $(foreach d, $(DTS_FILES), \
      $(DTC) -p 1024 -O dtb -o $(call DTB_FILE,$(d)) $(d); \
      cat $(KERNEL_ZIMG) $(call DTB_FILE,$(d)) > $(call ZIMG_FILE,$(d));))
endef
else

define append-dtb
endef
endif

ifeq ($(TARGET_USES_UNCOMPRESSED_KERNEL),true)
$(info Using uncompressed kernel)
TARGET_PREBUILT_KERNEL := $(KERNEL_OUT)/piggy
else
TARGET_PREBUILT_KERNEL := $(TARGET_PREBUILT_INT_KERNEL)
endif

define mv-modules
mdpath=`find $(KERNEL_MODULES_OUT) -type f -name modules.dep`;\
if [ "$$mdpath" != "" ];then\
mpath=`dirname $$mdpath`;\
ko=`find $$mpath/kernel -type f -name *.ko`;\
for i in $$ko; do mv $$i $(KERNEL_MODULES_OUT)/; done;\
fi
endef

define clean-module-folder
mdpath=`find $(KERNEL_MODULES_OUT) -type f -name modules.dep`;\
if [ "$$mdpath" != "" ];then\
mpath=`dirname $$mdpath`; rm -rf $$mpath;\
fi
endef

#Tweak defconfig for FACTORY KERNEL without additional fac_defcofig
define modi-facdefconfig
cp kernel/arch/arm/configs/$(KERNEL_DEFCONFIG) kernel/arch/arm/configs/factory_defconfig
echo -e "\nCONFIG_SEC_FACTORY=y" >> kernel/arch/arm/configs/factory_defconfig
endef

$(KERNEL_OUT):
	mkdir -p $(KERNEL_OUT)

$(KERNEL_CONFIG): $(KERNEL_OUT)
ifeq ($(SEC_FACTORY_BUILD),true)
	$(modi-facdefconfig)
	$(MAKE) -C kernel O=../$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=arm-linux-androideabi- VARIANT_DEFCONFIG=$(VARIANT_DEFCONFIG) DEBUG_DEFCONFIG=$(DEBUG_DEFCONFIG) SELINUX_DEFCONFIG=$(SELINUX_DEFCONFIG) SELINUX_LOG_DEFCONFIG=$(SELINUX_LOG_DEFCONFIG) factory_defconfig TIMA_DEFCONFIG=$(TIMA_DEFCONFIG)
else
	$(MAKE) -C kernel O=../$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=arm-linux-androideabi- VARIANT_DEFCONFIG=$(VARIANT_DEFCONFIG) DEBUG_DEFCONFIG=$(DEBUG_DEFCONFIG) SELINUX_DEFCONFIG=$(SELINUX_DEFCONFIG) SELINUX_LOG_DEFCONFIG=$(SELINUX_LOG_DEFCONFIG) $(KERNEL_DEFCONFIG) TIMA_DEFCONFIG=$(TIMA_DEFCONFIG)
endif
$(KERNEL_OUT)/piggy : $(TARGET_PREBUILT_INT_KERNEL)
	$(hide) gunzip -c $(KERNEL_OUT)/arch/arm/boot/compressed/piggy.gzip > $(KERNEL_OUT)/piggy

$(TARGET_PREBUILT_INT_KERNEL): $(KERNEL_OUT) $(KERNEL_CONFIG) $(KERNEL_HEADERS_INSTALL)
	$(MAKE) -C kernel O=../$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=arm-linux-androideabi-
ifeq "$(USE_MODULE)" "y"
	$(MAKE) -C kernel O=../$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=arm-linux-androideabi- modules
	$(MAKE) -C kernel O=../$(KERNEL_OUT) INSTALL_MOD_PATH=../../$(KERNEL_MODULES_INSTALL) INSTALL_MOD_STRIP=1 ARCH=arm CROSS_COMPILE=arm-linux-androideabi- modules_install
	$(mv-modules)
	$(clean-module-folder)
endif
	$(append-dtb)

$(KERNEL_HEADERS_INSTALL): $(KERNEL_OUT) $(KERNEL_CONFIG)
	$(MAKE) -C kernel O=../$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=arm-linux-androideabi- headers_install

kerneltags: $(KERNEL_OUT) $(KERNEL_CONFIG)
	$(MAKE) -C kernel O=../$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=arm-linux-androideabi- tags

kernelconfig: $(KERNEL_OUT) $(KERNEL_CONFIG)
	env KCONFIG_NOTIMESTAMP=true \
	     $(MAKE) -C kernel O=../$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=arm-linux-androideabi- menuconfig
	env KCONFIG_NOTIMESTAMP=true \
	     $(MAKE) -C kernel O=../$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=arm-linux-androideabi- savedefconfig
	cp $(KERNEL_OUT)/defconfig kernel/arch/arm/configs/$(KERNEL_DEFCONFIG)

endif
