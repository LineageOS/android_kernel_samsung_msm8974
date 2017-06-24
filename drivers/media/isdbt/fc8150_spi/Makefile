ifneq (,$(filter y, $(CONFIG_ISDBT_FC8150_SPI) $(CONFIG_ISDBT_FC8150_HKDI)))

DRV_NAME = isdbt_fc8150

#EXTRA_CFLAGS += -DDLOG


obj-y += bbm.o \
							fc8150.o \
							fc8150_bb.o \
							fc8150_hpi.o \
							fc8150_i2c.o \
							fc8150_isr.o \
							fc8150_ppi.o \
							fc8150_spi.o \
							fc8150_spib.o \
							fc8150_tun.o \
							fci_bypass.o \
							fci_hal.o \
							fci_i2c.o \
							fci_oal.o \
							fci_ringbuffer.o \
							fci_tun.o

endif

#ccflags-y += -Idrivers/media/isdbt
