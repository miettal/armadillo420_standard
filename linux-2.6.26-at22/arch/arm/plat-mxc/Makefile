#
# Makefile for the linux kernel.
#

# Common support
obj-y := gpio.o clock.o wdog.o snoop.o irq.o io.o

ifneq ($(CONFIG_ARCH_MX27),y)
obj-y += time.o cpu_common.o  spba.o sdma/
endif

obj-$(CONFIG_ARCH_MX25) += cpu_common.o devices.o iomux-v3.o
obj-$(CONFIG_ARCH_MX27) += cpu_common.o dma_mx2.o usb_common.o
obj-$(CONFIG_ARCH_MX3) += dptc.o usb_common.o


# LEDs support
obj-$(CONFIG_LEDS) += leds.o

# USB support
obj-$(CONFIG_ISP1504_MXC) += isp1504xc.o

obj-$(CONFIG_ISP1301_MXC) += isp1301xc.o

obj-$(CONFIG_MC13783_MXC) += mc13783_xc.o

obj-$(CONFIG_USB_EHCI_FSL_SERIAL) += serialxc.o

ifneq ($(strip $(CONFIG_USB_EHCI_FSL_UTMI) $(CONFIG_USB_GADGET_FSL_UTMI)),)
obj-y += utmixc.o
endif

ifneq ($(CONFIG_ARCH_MX25)$(CONFIG_USB),)
obj-y += usb_common.o
endif
