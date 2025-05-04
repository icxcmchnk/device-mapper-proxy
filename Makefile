.PHONY: all clean

CURRENT = $(shell uname -r)
KDIR = /lib/modules/$(CURRENT)/build
KCOMPILER = $(shell grep CONFIG_CC_VERSION /boot/config-$(CURRENT) \
			| cut -d'"' -f2 | awk '{print $$1}')
PWD = $(shell pwd)
TARGET = dmp

obj-m := $(TARGET).o

all:
	$(MAKE) CC=$(KCOMPILER) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) CC=$(KCOMPILER) -C $(KDIR) M=$(PWD) clean