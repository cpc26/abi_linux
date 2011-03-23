
O_TARGET        := dummy.o

list-multi      := abi-ibcs.o
abi-ibcs-objs	:= sysent.o

obj-$(CONFIG_ABI_IBCS)		+= abi-ibcs.o


include $(TOPDIR)/Rules.make

abi-ibcs.o: $(abi-ibcs-objs)
	$(LD) -r -o $@ $(abi-ibcs-objs)
