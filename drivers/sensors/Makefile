#
# Makefile for the sensors drivers.
#

# Each configuration option enables a list of files.

obj-$(CONFIG_SENSORS)		+= sensors_core.o

# combo chipset
obj-$(CONFIG_SENSORS_BMI055)	+= bma255.o bmg160.o
obj-$(CONFIG_SENSORS_BMI058)	+= bma280.o bmg160.o
obj-$(CONFIG_SENSORS_K303C)	+= k303c_acc.o k303c_mag.o
# magnetic_sensor
obj-$(CONFIG_SENSORS_AK8963C)	+= ak8963c.o
obj-$(CONFIG_SENSORS_AK09911C)	+= ak09911c.o
obj-$(CONFIG_SENSORS_HSCDTD008A)	+= hscdtd008a_i2c.o alps-input.o
# optical_sensor
obj-$(CONFIG_SENSORS_CM3323)	+= cm3323.o
obj-$(CONFIG_SENSORS_CM36651)	+= cm36651.o
obj-$(CONFIG_SENSORS_CM36653)	+= cm36653.o
obj-$(CONFIG_SENSORS_CM36686)	+= cm36686.o
obj-$(CONFIG_SENSORS_GP2A030A)	+= gp2ap030.o
obj-$(CONFIG_SENSORS_TMD3782S)	+= taos_tmd3782.o
obj-$(CONFIG_SENSORS_GP2A002)	+= gp2a002.o
obj-$(CONFIG_SENSORS_TMD27723)	+= tmd27723.o
# grip_sensor
obj-$(CONFIG_SENSORS_SX9500)	+= sx9500.o
# accelerometer_sensor
obj-$(CONFIG_SENSORS_BMA255)	+= bma255.o
obj-$(CONFIG_SENSORS_K3DH)	+= k3dh.o
obj-$(CONFIG_SENSORS_K2HH)	+= k2hh.o
obj-$(CONFIG_SENSORS_BMA254)	+= bma254.o
obj-$(CONFIG_INPUT_MPU6500)	+= mpu6500_input.o mpu6500_selftest.o
obj-$(CONFIG_SENSORS_YAS532)	+= yas532/
obj-$(CONFIG_SENSORS_ADPD142)	+= adpd142.o
# hrm_sensor
obj-$(CONFIG_SENSORS_MAX86900)	+= max86900.o
