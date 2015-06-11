#ifndef _LINUX_RTC_S35390A_H_
#define _LINUX_RTC_S35390A_H_

struct s35390a_platform_data {
	unsigned int alarm_irq;
	void (*alarm_irq_init)(void);
	int alarm_is_wakeup_src;
};

#endif /* _LINUX_RTC_S35390A_H_ */
