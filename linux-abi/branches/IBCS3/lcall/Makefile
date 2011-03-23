ifdef CONFIG_64BIT
abi_lcall-objs	:= lcall.o entry_64.o
else
abi_lcall-objs	:= lcall.o entry_32.o
endif

obj-m	+= abi_lcall.o

include $(ABI_DIR)/SETFLG
