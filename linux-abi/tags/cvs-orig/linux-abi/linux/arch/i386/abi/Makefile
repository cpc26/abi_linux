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

ABI_ARCH_OBJS := dispatch_i386.o maps_i386.o

ifeq ($(CONFIG_ABI),y)
  O_TARGET     := abi_dispatch.o
  OX_OBJS      += $(ABI_ARCH_OBJS)
else
  ifeq ($(CONFIG_ABI),m)
  M_OBJS       += abi_dispatch.o
  MIX_OBJS     += $(ABI_ARCH_OBJS)

abi_dispatch.o: $(ABI_ARCH_OBJS)
	$(LD) -r -o $@ $(ABI_ARCH_OBJS)

  endif
endif

include $(TOPDIR)/Rules.make
