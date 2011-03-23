
O_TARGET        := dummy.o

list-multi      := abi-wyse.o
abi-wyse-objs	:= sysent.o ptrace.o socket.o

obj-$(CONFIG_ABI_WYSE)		+= abi-wyse.o


include $(TOPDIR)/Rules.make

abi-wyse.o: $(abi-wyse-objs)
	$(LD) -r -o $@ $(abi-wyse-objs)
