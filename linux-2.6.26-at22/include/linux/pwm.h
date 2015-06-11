#ifndef __LINUX_PWM_H
#define __LINUX_PWM_H

struct pwm_device;

struct mxc_pwm_classdev
{
	const char *name;
	struct device *dev;
	struct pwm_device *pwm;
	unsigned long period_ns;
        unsigned long duty_ns;
	int invert;
	int enable;
};

struct platform_pwm_data {
	const char *name;
	struct mxc_pwm_classdev cdev;
	int invert;
	int export;
};

/*
 * pwm_request - request a PWM device
 */
struct pwm_device *pwm_request(int pwm_id, const char *label);

/*
 * pwm_free - free a PWM device
 */
void pwm_free(struct pwm_device *pwm);

/*
 * pwm_config - change a PWM device configuration
 */
int pwm_config(struct pwm_device *pwm, int duty_ns, int period_ns);

/*
 * pwm_set_invert - set a PWM device's output is inverted or not
 */
int pwm_set_invert(struct pwm_device *pwm, int invert);

/*
 * pwm_enable - start a PWM output toggling
 */
int pwm_enable(struct pwm_device *pwm);

/*
 * pwm_disable - stop a PWM output toggling
 */
void pwm_disable(struct pwm_device *pwm);

int mxc_pwm_class_init(void);
void mxc_pwm_class_exit(void);
int mxc_pwm_classdev_register(struct device *parent, int id, struct mxc_pwm_classdev *pwm_cdev);
void mxc_pwm_classdev_unregister(struct mxc_pwm_classdev *pwm_cdev);
int mxc_pwm_class_suspend(struct mxc_pwm_classdev *pwm_cdev);
int mxc_pwm_class_resume(struct mxc_pwm_classdev *pwm_cdev);

#endif /* __ASM_ARCH_PWM_H */
