#
# Makefile for the kernel ABI code which is shared by iBCS, Solaris and UW7
# Loosely speaking, this is the SVR4 code - so that's the name it's got.
# If you can think of a better name, feel free to change it.
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

SVR4_UNEXP_OBJS := isc_funcs.o sysisc.o vtkd.o xnx.o
SVR4_EXPORT_OBJS := ipc.o open.o svr4_funcs.o stream.o \
	utsname.o sysfs.o xstat.o sysinfo.o ulimit.o poll.o \
	hrtsys.o svr4.o ptrace.o mmap.o syslocal.o sysconf.o \
	sysi86.o xnx_funcs.o ioctl.o timod.o

SVR4_ALL_OBJS := $(SVR4_UNEXP_OBJS) $(SVR4_EXPORT_OBJS)

ifeq ($(CONFIG_ABI_SVR4_COMMON),y)
  O_TARGET	:= svr4_common.o
  O_OBJS	+= $(SVR4_UNEXP_OBJS)
  OX_OBJS	+= $(SVR4_EXPORT_OBJS)
else
  ifeq ($(CONFIG_ABI_SVR4_COMMON),m)
  M_OBJS	:= svr4_common.o
  MI_OBJS	:= $(SVR4_UNEXP_OBJS)
  MIX_OBJS	:= $(SVR4_EXPORT_OBJS)

svr4_common.o: $(SVR4_ALL_OBJS)
	$(LD) -r -o $@ $(SVR4_ALL_OBJS)

  endif
endif

include $(TOPDIR)/Rules.make
