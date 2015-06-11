#ifndef __ASM_ARCH_MXC_FLEXCAN_H__
#define __ASM_ARCH_MXC_FLEXCAN_H__

struct flexcan_platform_data {
	char *core_reg;
	char *io_reg;
	void (*xcvr_enable) (int id, int en);
	void (*active) (int id);
	void (*inactive) (int id);
};

#endif /* __ASM_ARCH_MXC_FLEXCAN_H__ */
