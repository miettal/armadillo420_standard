#
# Makefile for regulator drivers.
#

obj-$(CONFIG_REGULATOR) += reg-core.o
obj-$(CONFIG_REGULATOR_FIXED_VOLTAGE) += fixed.o

ifeq ($(CONFIG_REGULATOR_DEBUG),y)
	EXTRA_CFLAGS		+= -DDEBUG
endif

obj-$(CONFIG_REGULATOR_MC34704) += mc34704/
