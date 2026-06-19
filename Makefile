obj-m 	:= esccp.o
KDIR	:= /lib/modules/$(shell uname -r)/build
PWD	:= $(shell pwd)
IGNORE	:=

ccflags-y := -DESCC_DEBUG


default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

esccp-objs := esccdrv.o open.o close.o read.o write.o ioctl.o sync.o poll.o seek.o support.o isr.o

.PHONY:clean
clean:
	@find . $(IGNORE) \
	\( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' \
		-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \) \
		-type f -print | xargs rm -f

.PHONY:tools
tools:
	$(MAKE) -C utils/
	$(MAKE) -C utils/ install

.PHONY:tools-clean
tools-clean:
	$(MAKE) -C utils/ clean

