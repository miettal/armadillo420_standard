SRC_DIR = curl-7.26.0

CONF_BUILD = $(shell $(SRC_DIR)/config.guess)
CONF_HOST = $(shell $(SRC_DIR)/config.sub $(CROSS:-=))

BUILD_DIR = builddir

CONFIGURE_OPT = --build=$(CONF_BUILD) \
		--host=$(CONF_HOST)   \
		--prefix=/usr

all: build

$(BUILD_DIR)/Makefile:
	mkdir -p $(BUILD_DIR)
	(cd $(BUILD_DIR); ../$(SRC_DIR)/configure $(CONFIGURE_OPT))

build: $(BUILD_DIR)/Makefile
	make -C $(BUILD_DIR)

romfs: build
	$(ROMFSINST) $(BUILD_DIR)/src/.libs/curl /usr/bin/

clean:
	make -C $(BUILD_DIR) clean

distclean: clean
	rm -rf $(BUILD_DIR)
