#
# Makefile for MMC/SD card drivers
#

ifeq ($(CONFIG_MMC_DEBUG),y)
	EXTRA_CFLAGS		+= -DDEBUG
endif

obj-$(CONFIG_MMC_BLOCK)		+= mmc_block.o

ifeq ($(CONFIG_ARCH_MX25),y)
mmc_block-objs			:= block_mx25.o queue.o
else
mmc_block-objs			:= block.o queue.o
endif

obj-$(CONFIG_MMC_TEST)		+= mmc_test.o

obj-$(CONFIG_SDIO_UART)		+= sdio_uart.o

