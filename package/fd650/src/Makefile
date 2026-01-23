obj-m += fd650.o

ARCH ?= csky
CROSS_COMPILE ?= /root/buildroot/output/host/bin/csky-linux-

PWD := $(shell pwd)

all:
	$(MAKE) -C '$(LINUX_DIR)' M='$(PWD)' modules

clean:
	$(MAKE) -C '$(LINUX_DIR)' M='$(PWD)' clean


