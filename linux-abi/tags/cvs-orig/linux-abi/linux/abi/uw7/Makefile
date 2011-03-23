#
# Makefile for the kernel ABI code for UnixWare 7.x emulation
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

UW7_OBJS := do_uw7.o funcs.o kernel.o ioctl.o context.o mac.o access.o \
		proc.o statvfs.o mmap.o

ifeq ($(CONFIG_ABI_UW7),y)
  O_TARGET	:= abi_uw7.o
  O_OBJS	+= $(UW7_OBJS)
else
  ifeq ($(CONFIG_ABI_UW7),m)
  M_OBJS	:= abi_uw7.o
  MI_OBJS	:= $(UW7_OBJS)

abi_uw7.o: $(UW7_OBJS)
	$(LD) -r -o $@ $(UW7_OBJS)

  endif
endif

include $(TOPDIR)/Rules.make

