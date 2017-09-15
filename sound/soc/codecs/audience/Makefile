snd-soc-es325-objs := es325.o es325-slim.o es325-i2s.o es325-i2c.o
snd-soc-es705-objs := es705.o es705-platform.o es705-slim.o es705-i2s.o es705-i2c.o \
			es705-spi.o es705-cdev.o es705-uart.o es705-uart-common.o
snd-soc-es515-objs := es515.o escore.o es-a212.o escore-list.o es-d202.o
snd-soc-escore-slim-objs += escore-slim.o
snd-soc-escore-i2s-objs += escore-i2s.o
snd-soc-escore-i2c-objs += escore-i2c.o
snd-soc-escore-cdev-objs += escore-cdev.o
snd-soc-escore-uart-objs += escore-uart.o escore-uart-common.o

obj-$(CONFIG_SND_SOC_ES705)	+= snd-soc-es705.o
obj-$(CONFIG_SND_SOC_ES325)	+= snd-soc-es325.o
obj-$(CONFIG_SND_SOC_ES515)	+= snd-soc-es515.o
obj-$(CONFIG_SND_SOC_ES_SLIM)	+= snd-soc-escore-slim.o
obj-$(CONFIG_SND_SOC_ES_I2S)	+= snd-soc-escore-i2s.o
obj-$(CONFIG_SND_SOC_ES_I2C)	+= snd-soc-escore-i2c.o
obj-$(CONFIG_SND_SOC_ES_UART)	+= snd-soc-escore-uart.o
obj-$(CONFIG_SND_SOC_ES_CDEV)	+= snd-soc-escore-cdev.o
