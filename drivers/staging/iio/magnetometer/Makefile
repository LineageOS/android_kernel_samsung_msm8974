#
# Makefile for industrial I/O Magnetometer sensors
#

obj-$(CONFIG_SENSORS_AK8975)	+= ak8975.o
obj-$(CONFIG_SENSORS_HMC5843)	+= hmc5843.o
obj-$(CONFIG_SENSORS_YAS_MAGNETOMETER)	+= yas_mag_drv-yas532.o yas_mag_kernel.o
CFLAGS_yas_mag_kernel.o += -Idrivers/staging/iio