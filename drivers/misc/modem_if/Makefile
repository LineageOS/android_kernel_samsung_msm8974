# Makefile of modem_if

EXTRA_CFLAGS += -Idrivers/misc/modem_if_v2

obj-y += sipc4_modem.o sipc4_io_device.o
# obj-y += sipc5_modem.o sipc5_io_device.o
obj-y += modem_net_flowcontrol_device.o modem_utils.o

obj-$(CONFIG_GSM_MODEM_SPRD6500) += modem_modemctl_device_sc6500.o

obj-$(CONFIG_LINK_DEVICE_SPI) += modem_link_device_spi.o

obj-$(CONFIG_SIM_SLOT_SWITCH) += modem_sim_slot_switch.o
