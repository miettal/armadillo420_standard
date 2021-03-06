
EXEC = netflash
OBJS = tftpmain.o tftp.o tftpsubs.o http.o
COBJS = netflash.o
CFLAGS += -I../mtd-utils/include

ifdef CONFIG_USER_NETFLASH_WITH_FTP
CFLAGS += -DFTP
OBJS += cmds.o cmdtab.o domacro.o ftp.o glob.o ftpmain.o ruserpass.o
endif

ifdef CONFIG_USER_NETFLASH_WITH_CGI
OBJS += cgi.o cgiparse.o kmp.o
endif

ifdef CONFIG_USER_NETFLASH_HMACMD5
EXEC += hmacmd5
OBJS += md5.o hmacmd5.o
endif

ifdef HW_VENDOR
CFLAGS += -DVENDOR=\"$(HW_VENDOR)\"
else
CFLAGS += -DVENDOR=\"$(CONFIG_VENDOR)\"
endif

ifdef HW_PRODUCT
CFLAGS += -DPRODUCT=\"$(HW_PRODUCT)\"
else
CFLAGS += -DPRODUCT=\"$(CONFIG_PRODUCT)\"
endif

CFLAGS += -DVERSION=\"$(VERSIONPKG)\"
COBJS += versioning.o

ifdef CONFIG_USER_NETFLASH_DECOMPRESS
ADD_LIBZ  = -lz
endif

ifdef CONFIG_USER_NETFLASH_SETSRC
CFLAGS += -DSETSRC
endif

ifdef CONFIG_USER_NETFLASH_CRYPTO
ADD_LIBZ += $(LIBAES) $(LIBCRYPTO)
endif

CFLAGS += -Dnetflashmain=main
# CFLAGS += -DDEBUG_CGI


all: $(EXEC)

netflash: $(OBJS) $(COBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(COBJS) $(ADD_LIBZ) $(LDLIBS$(LDLIBS_$@))

netflash.o: netflash.c $(ROOTDIR)/config.arch
	echo $(COBJS)
	$(CC) -c -o $@ netflash.c -I$(ROOTDIR)/$(LINUXDIR)/include $(CFLAGS)

versioning.o: versioning.c $(ROOTDIR)/config.arch
	$(CC) -c -o $@ versioning.c $(CFLAGS)

hmacmd5: hmacmd5main.c hmacmd5.c md5.c
	gcc -o $@ $^

versiontest: versioning.c
	gcc -DVERSIONTEST -o $@ $^

romfs:
	$(ROMFSINST) /bin/netflash

clean:
	-rm -f netflash hmacmd5 *.elf *.gdb *.o

