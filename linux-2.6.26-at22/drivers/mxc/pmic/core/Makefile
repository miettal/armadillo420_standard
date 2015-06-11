#
# Makefile for the PMIC core drivers.
#
obj-$(CONFIG_MXC_SPI_PMIC_CORE) += pmic_core_spi_mod.o
obj-$(CONFIG_MXC_PMIC_CHARDEV)	+= pmic-dev.o

pmic_core_spi_mod-objs := pmic_external.o pmic_event.o pmic_core_spi.o 

obj-$(CONFIG_MXC_PMIC_MC34704) += pmic_mc34704_mod.o
pmic_mc34704_mod-objs := pmic_external.o pmic_event.o mc34704.o
