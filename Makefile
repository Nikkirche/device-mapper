MOD = dmp
obj-m += lib/$(MOD).o
KDIR = $(shell uname -r)/build
MDIR = $(PWD)
all:
	make -C /lib/modules/$(KDIR) M=$(MDIR) modules

clean:
	make -C /lib/modules/$(KDIR) M=$(MDIR) clean

insmod: all
	sudo rmmod lib/$(MOD).ko; true
	sudo insmod lib/$(MOD).ko dyndbg