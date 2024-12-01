# Makefile
obj-m += lightfs.o
lightfs-objs := main.o inode.o file.o bitmap.o dir.o
CFLAGS += -g

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

