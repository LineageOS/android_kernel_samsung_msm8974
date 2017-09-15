#
# Makefile for the sensor drivers.
#

# Each configuration option enables a list of files.
obj-$(CONFIG_SENSORS_SSP_ATMEL)	+= atmel/
ifdef CONFIG_SEC_PATEK_PROJECT
obj-$(CONFIG_SEC_PATEK_PROJECT)	+= stm_patek/
else
obj-$(CONFIG_SENSORS_SSP_STM)	+= stm/
endif
obj-$(CONFIG_SENSORS_SSP_STM_32F)	+= stm32f/
obj-$(CONFIG_SENSORS_SSP_STM_HESTIA)	+= stm_hestia/