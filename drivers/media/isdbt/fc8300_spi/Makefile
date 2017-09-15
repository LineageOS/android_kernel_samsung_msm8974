ifeq ($(CONFIG_ISDBT_FC8300_SPI),y)

DRV_NAME = isdbt_fc8300

#EXTRA_CFLAGS += -DDLOG


obj-$(CONFIG_ISDBT_FC8300_SPI) += bbm.o \
							fc8300.o \
							fc8300_bb.o \
							fc8300_cs_tun.o \
							fc8300_es1_tun.o \
							fc8300_es2_tun.o \
							fc8300_i2c.o \
							fc8300_isr.o \
							fc8300_ppi.o \
							fc8300_spi.o \
							fc8300_tun.o \
							fc8300_tun_table.o \
							fci_hal.o \
							fci_hpi.o \
							fci_oal.o \
							fci_ringbuffer.o \
							fci_tun.o

endif

#ccflags-y += -Idrivers/media/isdbt
