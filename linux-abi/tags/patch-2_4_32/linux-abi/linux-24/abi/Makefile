
O_TARGET	:= abi.o

res-$(CONFIG_ABI)		+= util/abi-util.o
res-$(CONFIG_ABI_SVR4)		+= svr4/abi-svr4.o
res-$(CONFIG_ABI_SCO)		+= sco/abi-sco.o cxenix/abi-cxenix.o
res-$(CONFIG_ABI_WYSE)		+= wyse/abi-wyse.o 
res-$(CONFIG_ABI_UW7)		+= uw7/abi-uw7.o
res-$(CONFIG_ABI_SOLARIS)	+= solaris/abi-solaris.o 
res-$(CONFIG_ABI_IBCS)		+= ibcs/abi-ibcs.o
res-$(CONFIG_ABI_ISC)		+= isc/abi-isc.o

subdir-$(CONFIG_ABI)		+= util
subdir-$(CONFIG_ABI_SVR4)	+= svr4
subdir-$(CONFIG_ABI_SCO)	+= sco cxenix
subdir-$(CONFIG_ABI_WYSE)	+= wyse
subdir-$(CONFIG_ABI_UW7)	+= uw7
subdir-$(CONFIG_ABI_SOLARIS)	+= solaris
subdir-$(CONFIG_ABI_IBCS)	+= ibcs
subdir-$(CONFIG_ABI_ISC)	+= isc


obj-y	+= $(res-y)

include $(TOPDIR)/Rules.make
