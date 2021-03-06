ifndef CONFIG_USER_FNORD_HTTPD
# Not running under the uClinux build environment,
# so build a host version for debugging.
# This one runs in server mode so that we don't need tcpserver
CFLAGS += -g -DEMBED -I../.. -DDEBUG -DLOG_TO_STDERR -DSERVER_MODE #-DCHECK_STR_COPY
LDLIBS += -lcrypt
endif

#CFLAGS=-Os -fomit-frame-pointer
VERSION=$(shell head -1 CHANGES|sed 's/://')
CFLAGS += -Wall
CFLAGS += -DFNORD=\"fnord/$(VERSION)\"
CFLAGS += -DCGI
FLTFLAGS += -s 15000

# For debugging...
#LDLIBS=$(SLIBC)
#CFLAGS += -g -DDEBUG

OBJS += httpd.o

ifndef CONFIG_USER_FNORD_NOAUTH
# These are for auth
CFLAGS += -DUSE_AUTH
OBJS += auth.o base64.o
endif

all: httpd

httpd: $(OBJS) libowfat.a
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBCRYPT) $(LDLIBS)

libowfat.a: httpd.o buffer_1.o buffer_puts.o buffer_flush.o buffer_put.o \
buffer_putulong.o buffer_2.o buffer_putspace.o buffer_stubborn.o \
buffer_putflush.o str_copy.o fmt_ulong.o byte_diff.o byte_copy.o \
str_len.o str_diff.o str_chr.o str_diffn.o str_start.o scan_ulong.o
	ar cru $@ $^
	-ranlib $@

.PHONY: rename clean install server
server:
	./httpd -p 8000 -l default

clean:
	-rm -f *.[oa] httpd *.gdb *.elf

romfs:
	$(ROMFSINST) /bin/httpd
ifndef CONFIG_USER_CGI_HTTPS_ONLY
	$(ROMFSINST) -a "80 stream tcp nowait root /bin/httpd /home/httpd" -A "80 stream" /etc/default/inetd.conf
endif
ifdef CONFIG_USER_SSLWRAP_SSLWRAP
	$(ROMFSINST) -a "443 stream tcp nowait root /bin/sslwrap -cert /etc/config/ssl_cert.pem -key /etc/config/ssl_key.pem -exec /bin/httpd /home/httpd" \
		-A "443 stream" /etc/default/inetd.conf
endif
