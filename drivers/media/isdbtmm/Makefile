#
# Makefile for the ISDB-Tmm tuner device drivers.
#

## Compile only the target include ISDB-Tmm configure ##
ifeq ($(CONFIG_ISDBTMM),y)

DRV_NAME = mm_tuner

#EXTRA_CFLAGS += -DDLOG


obj-$(CONFIG_ISDBTMM) += tuner_drv.o 		\
						 tuner_drv_hw.o 	\
						 tuner_drv_wrap.o 	\
						 tuner_spi.o		\
						 tuner.o

endif
