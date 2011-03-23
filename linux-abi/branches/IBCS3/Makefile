ifeq ($(MAKELEVEL),0)
  export ABI_DIR := $(shell pwd)
  export ABI_VER := $(shell uname -r)
else
  include $(ABI_DIR)/CONFIG
  include $(ABI_DIR)/SETMOD
endif

all:
	 $(MAKE) -C /lib/modules/$(ABI_VER)/build M=$(ABI_DIR) modules

clean:
	 $(MAKE) -C /lib/modules/$(ABI_VER)/build M=$(ABI_DIR) clean
