INCLUDES = 					\
	@KDRIVE_INCS@				\
	@KDRIVE_CFLAGS@

noinst_LIBRARIES = libfbdev.a

libfbdev_a_SOURCES =	\
	fbdev.c		\
	fbdev.h

if KDRIVEFBDEV
bin_PROGRAMS = Xfbdev

Xfbdev_SOURCES = \
	fbinit.c

Xfbdev_LDADD = 						\
	libfbdev.a					\
	@KDRIVE_LIBS@					\
	@XSERVER_LIBS@

Xfbdev_DEPENDENCIES =	\
	libfbdev.a					\
	@KDRIVE_LOCAL_LIBS@

relink:
	rm -f $(bin_PROGRAMS) && make $(bin_PROGRAMS)
endif
