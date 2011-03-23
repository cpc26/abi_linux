#
# Makefile for the kernel ABI code
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now inherited from the
# parent makes..
#

O_TARGET        := abi.o
SUB_DIRS        := binfmts
MOD_IN_SUB_DIRS	:= $(SUB_DIRS)
ALL_SUB_DIRS    := common bsd ibcs solaris sysv uw7
MOD_LIST_NAME	:= ABI_MODULES

ifeq ($(CONFIG_ABI),y)
  SUB_DIRS += common
  O_OBJS += common/abi_common.o
else
  ifeq ($(CONFIG_ABI),m)
  MOD_IN_SUB_DIRS += abi_common
  MOD_TO_LIST += abi_common.o
  endif
endif

ifeq ($(CONFIG_ABI_SVR4_COMMON),y) 
  SUB_DIRS += sysv
  O_OBJS += sysv/svr4_common.o
else
  ifeq ($(CONFIG_ABI_SVR4_COMMON),m)
  MOD_IN_SUB_DIRS += svr4_common
  MOD_TO_LIST += svr4_common.o
  endif
endif

ifeq ($(CONFIG_ABI_BSD),y) 
  SUB_DIRS += bsd
  O_OBJS += bsd/abi_bsd.o
else
  ifeq ($(CONFIG_ABI_BSD),m)
  MOD_IN_SUB_DIRS += bsd
  MOD_TO_LIST += abi_bsd.o
  endif
endif

ifeq ($(CONFIG_ABI_IBCS),y) 
  SUB_DIRS += ibcs
  O_OBJS += ibcs/abi_ibcs.o
else
  ifeq ($(CONFIG_ABI_IBCS),m)
  MOD_IN_SUB_DIRS += ibcs
  MOD_TO_LIST += abi_ibcs.o
  endif
endif

ifeq ($(CONFIG_ABI_SOLARIS),y) 
  SUB_DIRS += solaris
  O_OBJS += solaris/abi_solaris.o
else
  ifeq ($(CONFIG_ABI_SOLARIS),m)
  MOD_IN_SUB_DIRS += solaris
  MOD_TO_LIST += abi_solaris.o
  endif
endif

ifeq ($(CONFIG_ABI_UW7),y) 
  SUB_DIRS += uw7
  O_OBJS += uw7/abi_uw7.o
else
  ifeq ($(CONFIG_ABI_UW7),m)
  MOD_IN_SUB_DIRS += uw7
  MOD_TO_LIST += abi_uw7.o
  endif
endif

ifeq ($(CONFIG_ARTHUR),y)
  SUB_DIRS += riscos
  O_OBJS += riscos/arthur.o
else
  ifeq ($(CONFIG_ARTHUR),m)
  MOD_IN_SUB_DIRS += riscos
  MOD_TO_LIST += arthur.o
  endif
endif

include $(TOPDIR)/Rules.make

