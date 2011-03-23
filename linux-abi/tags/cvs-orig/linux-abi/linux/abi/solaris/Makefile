#
# Makefile for the kernel ABI code for Solaris emulation
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

SOLARIS_OBJS := do_solaris.o lfs.o solarisx86.o

ifeq ($(CONFIG_ABI_SOLARIS),y)
  O_TARGET	:= abi_solaris.o
  O_OBJS	+= $(SOLARIS_OBJS)
else
  ifeq ($(CONFIG_ABI_SOLARIS),m)
  M_OBJS	:= abi_solaris.o
  MI_OBJS	:= $(SOLARIS_OBJS)

abi_solaris.o: $(SOLARIS_OBJS)
	$(LD) -r -o $@ $(SOLARIS_OBJS)

  endif
endif

include $(TOPDIR)/Rules.make

