#
# Makefile for i.MX pwm devices.
#
obj-$(CONFIG_MXC_PWM)		+= mxc_pwm.o
obj-$(CONFIG_MXC_PWM_CLASS)	+= mxc_pwm_class.o

ifeq ($(CONFIG_MXC_PWM_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif
