#
# Makefile for the linux kernel.
#

# Object file lists.

obj-y				:= system.o cpu.o mm.o clock.o devices.o serial.o
obj-$(CONFIG_GENERIC_GPIO)	+= gpio.o
obj-$(CONFIG_MXC_SDMA_API) 	+= dma.o
obj-$(CONFIG_PM) 		+= pm.o

armadillo400-objs-y	:= armadillo400.o armadillo400_gpio.o
armadillo460-objs-y	:= $(armadillo400-objs-y) armadillo460.o

ifeq ($(CONFIG_ARMADILLO410_CON2_AIOTG_STD),y)
obj-y				+= armadillo_iotg_std.o armadillo_iotg_std_gpio.o
else
obj-$(CONFIG_MACH_ARMADILLO410) += $(armadillo400-objs-y)
obj-$(CONFIG_MACH_ARMADILLO420) += $(armadillo400-objs-y)
obj-$(CONFIG_MACH_ARMADILLO440) += $(armadillo400-objs-y)
obj-$(CONFIG_MACH_ARMADILLO460) += $(armadillo460-objs-y)
endif

obj-$(CONFIG_AIOTG_STD_ADDON_AUTO_DETECT) += armadillo_iotg_std_addon.o

obj-$(CONFIG_USB_EHCI_ARC_H2)	+= usb_h2.o

ifneq ($(strip $(CONFIG_USB_GADGET_ARC) $(CONFIG_USB_EHCI_ARC_OTG)),)
	obj-y			+= usb_dr.o
endif
