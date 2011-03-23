#
# Makefile for the kernel ABI code for BSD emulation
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now inherited from the
# parent makes..
#

SUB_DIRS        := 
MOD_SUB_DIRS    := $(SUB_DIRS)
ALL_SUB_DIRS    :=

BSD_OBJS := bsd.o bsdioctl.o bsdsignal.o bsdsocket.o bsdstat.o do_bsd.o

ifeq ($(CONFIG_ABI_BSD),y)
  O_TARGET	:= abi_bsd.o
  O_OBJS	+= $(BSD_OBJS)
else
  ifeq ($(CONFIG_ABI_BSD),m)
  M_OBJS	:= abi_bsd.o
  MI_OBJS	:= $(BSD_OBJS)

abi_bsd.o: $(BSD_OBJS)
	$(LD) -r -o $@ $(BSD_OBJS)

  endif
endif

include $(TOPDIR)/Rules.make

