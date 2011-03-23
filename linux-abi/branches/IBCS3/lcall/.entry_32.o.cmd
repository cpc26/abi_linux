cmd_/media/sf_WILKUN/linux-abi/linux-abi/branches/IBCS3/lcall/entry_32.o := gcc -Wp,-MD,/media/sf_WILKUN/linux-abi/linux-abi/branches/IBCS3/lcall/.entry_32.o.d  -nostdinc -isystem /usr/lib/gcc/i486-linux-gnu/4.4.3/include  -Iinclude  -I/usr/src/linux-headers-2.6.32-29-generic/arch/x86/include -include include/linux/autoconf.h -Iubuntu/include  -D__KERNEL__ -D__ASSEMBLY__ -m32 -DCONFIG_AS_CFI=1 -DCONFIG_AS_CFI_SIGNAL_FRAME=1       -D_KSL=32  -DMODULE -c -o /media/sf_WILKUN/linux-abi/linux-abi/branches/IBCS3/lcall/entry_32.o /media/sf_WILKUN/linux-abi/linux-abi/branches/IBCS3/lcall/entry_32.S

deps_/media/sf_WILKUN/linux-abi/linux-abi/branches/IBCS3/lcall/entry_32.o := \
  /media/sf_WILKUN/linux-abi/linux-abi/branches/IBCS3/lcall/entry_32.S \
    $(wildcard include/config/x86/32/lazy/gs.h) \
  /usr/src/linux-headers-2.6.32-29-generic/arch/x86/include/asm/segment.h \
    $(wildcard include/config/x86/32.h) \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/cc/stackprotector.h) \
    $(wildcard include/config/paravirt.h) \
  /usr/src/linux-headers-2.6.32-29-generic/arch/x86/include/asm/thread_info.h \
    $(wildcard include/config/debug/stack/usage.h) \
  include/linux/compiler.h \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  /usr/src/linux-headers-2.6.32-29-generic/arch/x86/include/asm/page.h \
    $(wildcard include/config/x86/64.h) \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbdaf.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  /usr/src/linux-headers-2.6.32-29-generic/arch/x86/include/asm/types.h \
    $(wildcard include/config/highmem64g.h) \
  include/asm-generic/types.h \
  include/asm-generic/int-ll64.h \
  /usr/src/linux-headers-2.6.32-29-generic/arch/x86/include/asm/bitsperlong.h \
  include/asm-generic/bitsperlong.h \
  /usr/src/linux-headers-2.6.32-29-generic/arch/x86/include/asm/page_types.h \
  include/linux/const.h \
  /usr/src/linux-headers-2.6.32-29-generic/arch/x86/include/asm/page_32_types.h \
    $(wildcard include/config/highmem4g.h) \
    $(wildcard include/config/page/offset.h) \
    $(wildcard include/config/4kstacks.h) \
    $(wildcard include/config/x86/pae.h) \
  /usr/src/linux-headers-2.6.32-29-generic/arch/x86/include/asm/page_32.h \
    $(wildcard include/config/hugetlb/page.h) \
    $(wildcard include/config/debug/virtual.h) \
    $(wildcard include/config/flatmem.h) \
    $(wildcard include/config/x86/use/3dnow.h) \
    $(wildcard include/config/x86/3dnow.h) \
  include/asm-generic/memory_model.h \
    $(wildcard include/config/discontigmem.h) \
    $(wildcard include/config/sparsemem/vmemmap.h) \
    $(wildcard include/config/sparsemem.h) \
  include/asm-generic/getorder.h \
  include/asm/asm-offsets.h \

/media/sf_WILKUN/linux-abi/linux-abi/branches/IBCS3/lcall/entry_32.o: $(deps_/media/sf_WILKUN/linux-abi/linux-abi/branches/IBCS3/lcall/entry_32.o)

$(deps_/media/sf_WILKUN/linux-abi/linux-abi/branches/IBCS3/lcall/entry_32.o):
