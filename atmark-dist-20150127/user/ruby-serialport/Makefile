RUBY_SERIALPORT_VERSION = 1.3.1
RUBY_CC_VERSION_$(CONFIG_USER_RUBY_RUBY_2_0_0_P451) = 2.0.0
RUBY_CC_VERSION_$(CONFIG_USER_RUBY_RUBY_2_1_2) = 2.1.2
RUBY_CC_VERSION = $(RUBY_CC_VERSION_y)

all: build

build: 
	rake RUBY_CC_VERSION=$(RUBY_CC_VERSION) CROSS_PLATFORM=$(CROSS_COMPILE:-=) cross clean native gem
	[ ! -d build ] && mkdir build
	gem install --local $$(ls pkg/serialport-$(RUBY_SERIALPORT_VERSION)-*.gem) --install-dir=./build --no-document

romfs:
	$(ROMFSINST) $$(ls -d build/gems/*/lib/serialport) /usr/lib/ruby/vendor_ruby/serialport
	$(ROMFSINST) $$(ls -d build/gems/*/lib/serialport.rb) /usr/lib/ruby/vendor_ruby/serialport.rb
	$(ROMFSINST) $$(ls -d build/gems/*/lib/serialport.so) /usr/lib/ruby/vendor_ruby/serialport.so

clean:
	rm -rf build
	rm -rf pkg/*
