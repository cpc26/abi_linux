
O_TARGET        := dummy.o

list-multi      := abi-util.o
abi-util-objs	:= stat.o plist.o

obj-$(CONFIG_ABI)	+= abi-util.o


include $(TOPDIR)/Rules.make

abi-util.o: $(abi-util-objs)
	$(LD) -r -o $@ $(abi-util-objs)
