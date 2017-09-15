##########################################
# Makefile for MTV Device Drivers.
##########################################

ccflags-$(CONFIG_MTV)  += -Idrivers/media/mtv
ccflags-y += -v

obj-$(CONFIG_MTV) := isdbt.o
#obj-$(CONFIG_MTV) := isdbt.o isdbt_data.o

###########
# FCI
###########

#FC8150
ccflags-$(CONFIG_MTV_FC8150) += -Idrivers/media/mtv/fc8150
obj-$(CONFIG_MTV_FC8150) += isdbt_port_fc8150.o
obj-$(CONFIG_MTV_FC8150) += fc8150/

#FC8300
ccflags-$(CONFIG_MTV_FC8300) += -Idrivers/media/mtv/fc8300
obj-$(CONFIG_MTV_FC8300) += isdbt_port_fc8300.o
obj-$(CONFIG_MTV_FC8300) += fc8300/

###########
# Raontech 
###########

# MTV222
ccflags-$(CONFIG_MTV_MTV222) += -Idrivers/media/mtv/mtv222
obj-$(CONFIG_MTV_MTV222) += isdbt_port_mtv222.o
obj-$(CONFIG_MTV_MTV222) += mtv222/

# MTV230
ccflags-$(CONFIG_MTV_MTV23x) += -Idrivers/media/mtv/mtv23x
obj-$(CONFIG_MTV_MTV23x) += isdbt_port_mtv23x.o
obj-$(CONFIG_MTV_MTV23x) += mtv23x/

###########
# I/F
###########

obj-$(CONFIG_MTV_SPI) += isdbt_spi.o
obj-$(CONFIG_MTV_EBI) += isdbt_ebi.o
obj-$(CONFIG_MTV_I2C) += isdbt_i2c.o
obj-$(CONFIG_MTV_TSIF) += isdbt_tsi.o

