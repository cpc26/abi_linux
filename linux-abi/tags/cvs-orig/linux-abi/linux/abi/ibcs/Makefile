#
# Makefile for the kernel ABI code for iBCS emulation
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

IBCS_OBJS := do_ibcs.o secureware.o wyse_nfs.o wyse_tcp.o

ifeq ($(CONFIG_ABI_IBCS),y)
  O_TARGET	:= abi_ibcs.o
  O_OBJS	+= $(IBCS_OBJS)
else
  ifeq ($(CONFIG_ABI_IBCS),m)
  M_OBJS	:= abi_ibcs.o
  MI_OBJS	:= $(IBCS_OBJS)

abi_ibcs.o: $(IBCS_OBJS)
	$(LD) -r -o $@ $(IBCS_OBJS)

  endif
endif

include $(TOPDIR)/Rules.make

