#
# Makefile for Inertial Measurement Units
#

adis16400-y             := adis16400_core.o
adis16400-$(CONFIG_IIO_BUFFER) += adis16400_ring.o adis16400_trigger.o
obj-$(CONFIG_ADIS16400) += adis16400.o

obj-y += mpu/
obj-y += mpu_primary/
obj-y += inv_mpu/
obj-y += inv_sensors/
