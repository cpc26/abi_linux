
O_TARGET        := dummy.o

list-multi      := abi-isc.o
abi-isc-objs	:= sysent.o misc.o

obj-$(CONFIG_ABI_ISC)		+= abi-isc.o


include $(TOPDIR)/Rules.make

abi-isc.o: $(abi-isc-objs)
	$(LD) -r -o $@ $(abi-isc-objs)
