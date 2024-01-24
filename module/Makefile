obj-m += src/networkfs.o 

PWD := $(CURDIR) 
KDIR = /lib/modules/`uname -r`/build
EXTRA_CFLAGS = -Wall -g -std=gnu11

all:
	make -C $(KDIR) M=$(PWD) modules 

clean:
	make -C $(KDIR) M=$(PWD) clean
