#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/file.h>

#define GPIO_PATH	"/sys/class/gpio/VIN_ALERT_N/"
#define ADC_PATH	"/sys/devices/platform/i2c-gpio.3/i2c-adapter/i2c-3/3-0054/"

#define ADC_VOLTAGE_MIN	0
#define ADC_VOLTAGE_MAX	3300	/* reference voltage */

#define VOLTAGE_MIN	0	/* vout_to_vin(ADC_VOLTAGE_MIN) */
#define VOLTAGE_MAX	20223	/* vout_to_vin(ADC_VOLTAGE_MAX) */

#define LOCKFILE 	"/var/run/vintrigger.lock"
static int lock = -1;

static int terminated;

#define __stringify_1(x)	#x
#define stringify(x)		__stringify_1(x)

#define UL_BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:-!!(e); }))
#define __must_be_array(a) \
	UL_BUILD_BUG_ON_ZERO(__builtin_types_compatible_p(__typeof__(a), __typeof__(&a[0])))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))

#define DIV_ROUND_CLOSEST(x, divisor)(			\
{							\
	typeof(divisor) __divisor = divisor;		\
	(((x) + ((__divisor) / 2)) / (__divisor));	\
}							\
)

#define __unused __attribute__((unused))

#define logging_init()							\
({									\
	openlog("vintrigger", LOG_PID | LOG_NOWAIT, LOG_DAEMON);	\
})

#define logging_close()	\
({			\
	closelog();	\
})

#define log_error(doexit, ...)		\
({					\
	syslog(LOG_ERR, __VA_ARGS__);	\
	logging_close();		\
	if (doexit)			\
		exit(EXIT_FAILURE);	\
})

#define log_message( ...)		\
({					\
	syslog(LOG_INFO, __VA_ARGS__);	\
})


#define ALT_OVER	(1<<0)
#define ALT_UNDER	(1<<1)
#define ALT_BOTH	(ALT_OVER | ALT_UNDER)

static int alteration;
static char *high_limit;
static char *low_limit;

static char **params;

static ssize_t _xwrite(int fd, const void *buf, size_t count)
{
	ssize_t ret;

	do {
		ret = write(fd, buf, count);
	} while (ret < 0 && errno == EINTR);

	return ret;
}

ssize_t xwrite(int fd, const void *buf, size_t count)
{
	ssize_t cc;
	ssize_t total;

	total = 0;

	while (count) {
		cc = _xwrite(fd, buf, count);

		if (cc < 0) {
			if (total)
				return total;
			return cc;
		}

		total += cc;
		buf = ((const char *)buf) + cc;
		count -= cc;
	}

	return total;
}

static ssize_t _xread(int fd, void *buf, size_t count)
{
	ssize_t ret;

	do {
		ret = read(fd, buf, count);
	} while (ret < 0 && errno == EINTR);

	return ret;
}

static ssize_t xread(int fd, void *buf, size_t count)
{
	ssize_t cc;
	ssize_t total;

	total = 0;

	while (count) {
		cc = _xread(fd, buf, count);

		if (cc < 0) {
			if (total)
				return total;
			return cc;
		}
		if (cc == 0)
			break;
		buf = ((char *)buf) + cc;
		total += cc;
		count -= cc;
	}

	return total;
}


static int xpoll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	int ret;

	do {
		if (terminated)
			return -1;
		ret = poll(fds, nfds, timeout);
	} while (ret < 0 && errno == EINTR);

	return ret;
}

static int xflock(int fd, int operation)
{
	int ret;

	do {
		ret = flock(fd, operation);
	} while (ret < 0 && errno == EINTR);

	return ret;
}

static int xstrtol(const char *str, long *val)
{
	const char *ptr;
	char *endptr;

	ptr = str;

	while (*ptr != '\0') {
		if (isspace((int)*ptr)) {
			ptr++;
			continue;
		}
		break;
	}

	if (*ptr == '\0')
		return -1;

	errno = 0;
	*val = strtol(ptr, &endptr, 0);

	if (errno || (*endptr == *ptr))
		return -1;

	return 0;
}

static void set_signal_edge(void)
{
        int fd;
	int ret;

        fd = open(GPIO_PATH "edge", O_RDWR);
	if (fd < 0)
		log_error(1, "cannot open %s", GPIO_PATH "edge");

	if (alteration == ALT_OVER)
		ret = xwrite(fd, "falling", 7);
	else
		ret = xwrite(fd, "rising", 6);
	if (ret < 0)
		log_error(1, "cannot setting signal edge");

        close(fd);
}

static void wait_interrupt(void)
{
        int fd;
	struct pollfd pfd;
	char val;
	int ret;

	fd = open(GPIO_PATH "value", O_RDONLY);
	if (fd < 0)
		log_error(1, "cannot open %s", GPIO_PATH "value");

	ret = xread(fd, &val, 1);
	if (ret < 0)
		log_error(1, "cannot read %s", GPIO_PATH "value");

	if (val == '0')
		goto out;

	set_signal_edge();

	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	ret = xpoll(&pfd, 1, -1);
	if (ret < 0)
		log_error(1, "cannot poll %s", GPIO_PATH "value");

out:
	close(fd);
}

/*
 *  VIN
 *   |
 *   R1(200k)
 *   |
 *   |--VOUT(to ADC081C)
 *   |
 *   R2(39k)
 *   |
 *  GND
 */
#define R1	200
#define R2	39

static int vin_to_vout(int vin)
{
	return DIV_ROUND_CLOSEST(R2 * vin, R1 + R2);
}

static int __unused vout_to_vin(int vout)
{
	return DIV_ROUND_CLOSEST((R1 + R2) * vout, R2);
}

static int is_valid_voltage(char *arg)
{
	long volt;
	int ret;

	ret = xstrtol(arg, &volt);
	if (ret)
		return ret;

	if ((volt < VOLTAGE_MIN) || (VOLTAGE_MAX < volt))
		return -1;

	return 0;
}

#define ADC_VOLT_LEN_MAX	(4) /* strlen(stringify(ADC_VOLTAGE_MAX)) + 1 */
static char adc_high_limit[ADC_VOLT_LEN_MAX];
static char adc_low_limit[ADC_VOLT_LEN_MAX];

static void write_adc_file(const char *file, const char *val, size_t count, int exit_on_err)
{
	char path[MAXPATHLEN];
        int fd;
	int ret;

	snprintf(path, sizeof(path), "%s%s", ADC_PATH, file);
	fd = open(path, O_RDWR);
	if (fd < 0) {
		log_error(exit_on_err, "cannot open %s", path);
		return;
	}

	ret = xwrite(fd, val, count);
	if (ret < 0) {
		log_error(exit_on_err, "cannot write %s", path);
		return;
	}

	close(fd);
}

static void read_adc_file(const char *file, char *val, size_t count, int exit_on_err)
{
	char path[MAXPATHLEN];
        int fd;
	int ret;

	snprintf(path, sizeof(path), "%s%s", ADC_PATH, file);
	fd = open(path, O_RDWR);
	if (fd < 0) {
		log_error(exit_on_err, "cannot open %s", path);
		return;
	}

	ret = xread(fd, val, count);
	if (ret < 0)
		log_error(exit_on_err, "cannot read %s", path);

	close(fd);
}

static void restore_adc(void)
{
	if (strlen(adc_high_limit))
		write_adc_file("high_limit", adc_high_limit, strlen(adc_high_limit), 0);

	if (strlen(adc_low_limit))
		write_adc_file("low_limit", adc_low_limit, strlen(adc_low_limit), 0);
}

static void backup_adc(void)
{
	read_adc_file("high_limit", adc_high_limit, ADC_VOLT_LEN_MAX, 1);
	read_adc_file("low_limit", adc_low_limit, ADC_VOLT_LEN_MAX, 1);
}

#define VOLT_LEN_MAX	(6) /* strlen(stringify(VOLTAGE_MAX)) + 1 */

static void setup_adc(void)
{
	long lim;
	char hlim[VOLT_LEN_MAX];
	char llim[VOLT_LEN_MAX];

	if (alteration & ALT_OVER) {
		xstrtol(high_limit, &lim);
		snprintf(hlim, sizeof(hlim), "%d", vin_to_vout(lim));
	}
	if (alteration & ALT_UNDER) {
		xstrtol(low_limit, &lim);
		snprintf(llim, sizeof(llim), "%d", vin_to_vout(lim));
	}

	switch (alteration) {
	case ALT_OVER:
		write_adc_file("high_limit", hlim, strlen(hlim), 1);
		write_adc_file("low_limit", stringify(ADC_VOLTAGE_MIN), ADC_VOLT_LEN_MAX, 1);
		break;
	case ALT_UNDER:
		write_adc_file("high_limit", stringify(ADC_VOLTAGE_MAX), ADC_VOLT_LEN_MAX, 1);
		write_adc_file("low_limit", llim, strlen(llim), 1);
		break;
	case ALT_BOTH:
		write_adc_file("high_limit", hlim, strlen(hlim), 1);
		write_adc_file("low_limit", llim, strlen(llim), 1);
		break;
	default:
		log_error(1, "unsupported alteration 0x%x", alteration);
	}
}

static void term_handler(__unused int signum)
{
	terminated = 1;
}

static int setup_signal(void)
{
	int term_signals[] = {
		SIGHUP, SIGINT, SIGQUIT, SIGPIPE, SIGTERM
	};
	struct sigaction sa;
	unsigned int i;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = term_handler;

	for(i = 0; i < ARRAY_SIZE(term_signals); i++) {
		if (sigaction(term_signals[i], &sa, NULL) < 0) {
			fprintf(stderr, "cannot set signal handler\n");
			return -1;
		}
	}

	return 0;
}

static void exec_cmd(void)
{
	int pid;
	int status;

	if (!params)
		return;

	pid = fork();
	if (pid < 0)
		log_error(1, "cannot fork");

	if (!pid) {
		execvp(params[0], params);
		log_error(1, "execvp failure");
	} else
		waitpid(pid, &status, 0);
}

static void usage(void)
{
	fprintf(stderr, "Usage: vintrigger -o|-u VOLTAGE COMMAND [ARGS]\n"
		"Options:\n"
		"  -o, --over=VOLTAGE\n"
		"      Execute the program COMMAND when the detected voltage is equal\n"
		"      to or over the VOLTAGE.\n"
		"  -u, --under=VOLTAGE\n"
		"      Execute the program COMMAND when the detected voltage is equal\n"
		"      to or unver the VOLTAGE.\n"
		"VOLTAGE: Range: %d - %d\n", VOLTAGE_MIN, VOLTAGE_MAX);
}

static int parse_args(int argc, char *argv[])
{
	struct option longopts[] = {
		{"over",required_argument, NULL, 'o' },
		{"under",required_argument, NULL, 'u' },
		{0,0,0,0},
	};
	int c;
	int ret;

	while ((c = getopt_long(argc, argv, "+o:u:",longopts, NULL)) != EOF) {
		switch (c) {
		case 'o':
			alteration |= ALT_OVER;
			ret = is_valid_voltage(optarg);
			if (ret)
				return -1;
			high_limit = optarg;
			break;
		case 'u':
			alteration |= ALT_UNDER;
			ret = is_valid_voltage(optarg);
			if (ret)
				return -1;
			low_limit = optarg;
			break;
		default:
			return -1;
		}
	}

	if (!alteration)
		return -1;

	if (optind < argc)
		params = &argv[optind];

	return 0;
}

static int place_exclusive_lock(void)
{
	int ret;

	lock = open(LOCKFILE, O_RDONLY|O_CREAT|O_CLOEXEC,
		  S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH);
	if (lock < 0)
		fprintf(stderr, "cannot open %s\n", LOCKFILE);

	ret = xflock(lock, LOCK_EX | LOCK_NB);
	if (ret < 0) {
		if (errno == EWOULDBLOCK)
			fprintf(stderr, "vintrigger is already running\n");
		else
			fprintf(stderr, "cannot flock %s\n", LOCKFILE);
	}

	return ret;
}

static void remove_exclusive_lock(void)
{
	if (lock < 0)
		return;

	xflock(lock, LOCK_UN | LOCK_NB);
	close(lock);
	unlink(LOCKFILE);
}

int main(int argc, char *argv[])
{
	int ret;

	ret = parse_args(argc, argv);
	if (ret) {
		usage();
		return EXIT_FAILURE;
	}

	atexit(remove_exclusive_lock);
	ret = place_exclusive_lock();
	if (ret)
		return EXIT_FAILURE;

	ret = setup_signal();
	if (ret)
		return EXIT_FAILURE;

	logging_init();

	backup_adc();

	atexit(restore_adc);
	setup_adc();

	switch (alteration) {
	case ALT_OVER:
		log_message("waiting for an over range alert (%s mV).", high_limit);
		break;
	case ALT_UNDER:
		log_message("waiting for an under range alert (%s mV).", low_limit);
		break;
	case ALT_BOTH:
		log_message("waiting for an under range alert or a over range alert (%s-%s mV).",
			    low_limit, high_limit);
		break;
	default:
		log_error(1, "unsupported alteration 0x%x", alteration);
	}
	wait_interrupt();
	log_message("exceeded the limit. executing command.");

	exec_cmd();

	restore_adc();
	remove_exclusive_lock();
	logging_close();

	return EXIT_SUCCESS;
}
