#
# Makefile for the kernel ABI code which is shared by all emulations
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

# All of these export symbols - otherwise they wouldn't be here.

ABI_COMMON_OBJS := core.o map.o stat.o wysev386.o signal.o socket.o \
	ioctl.o socksys.o proc.o

ifeq ($(CONFIG_ABI),y)
  O_TARGET	:= abi_common.o
  OX_OBJS	+= $(ABI_COMMON_OBJS)
else
  ifeq ($(CONFIG_ABI),m)
  M_OBJS	+= abi_common.o
  MIX_OBJS	+= $(ABI_COMMON_OBJS)

abi_common.o: $(ABI_COMMON_OBJS)
	$(LD) -r -o $@ $(ABI_COMMON_OBJS) $(TOPDIR)/arch/$(ARCH)/abi/abi_dispatch.o

  endif
endif

include $(TOPDIR)/Rules.make

