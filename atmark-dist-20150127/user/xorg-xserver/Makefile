#
# Makefile for xorg-xserver
#

CONF_BUILD = $(shell $(SRC_DIR)/config.guess)
CONF_HOST = $(shell $(SRC_DIR)/config.sub $(CROSS:-=))

SRC_DIR = xorg-xserver-1.4.2/
CONFIGURE_OPT =	--host=$(CONF_HOST) \
                --build=$(CONF_BUILD) \
                --prefix=/usr \
                --with-fontdir=/usr/share/fonts/X11 \
                --disable-builddocs \
                --disable-build-linuxdoc \
                --disable-aiglx \
                --disable-registry \
                --disable-composite \
                --disable-shm \
                --disable-xres \
                --disable-xv \
                --disable-xvmc \
                --disable-dga \
                --disable-screensaver \
                --disable-xdmcp \
                --disable-xdm-auth-1 \
                --disable-glx \
                --disable-dri \
                --disable-dri2 \
                --disable-xinerama \
                --disable-xf86vidmode \
                --disable-xace \
                --disable-xevie \
                --disable-dbe \
                --disable-dpms \
                --disable-config-hal \
                --disable-xfree86-utils \
                --disable-xorg \
                --disable-xvfb \
                --disable-xnest \
                --disable-xephyr \
                --disable-xsdl \
                --disable-kdrive-vesa \
                --disable-secure-rpc \
                --enable-tslib \
                --enable-kdrive


PROG = 

all: configure build

configure:
	(cd $(SRC_DIR); ./configure $(CONFIGURE_OPT) )

build: $(SRC_DIR)/Makefile
	make -C $(SRC_DIR) all

romfs: build
	 $(ROMFSINST) $(SRC_DIR)hw/kdrive/fbdev/Xfbdev /usr/bin/Xfbdev

clean:
	make -C $(SRC_DIR) clean


distclean: clean
	make -C $(SRC_DIR) distclean

