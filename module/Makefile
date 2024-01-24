obj-m += src/linufs.o
obj-m += src/networkfs.o
ccflags-y := -std=gnu11 -g -Wall -Wno-declaration-after-statement

PWD := $(CURDIR)
KDIR = /lib/modules/`uname -r`/build

all:
	make -C $(KDIR) M=$(PWD) modules 

clean:
	make -C $(KDIR) M=$(PWD) clean
