LDSCRIPT := $(SRCTREE)/arch/arm/cpu/arm920t/ep93xx/u-boot.lds

ifdef CONFIG_EDB9301
TEXT_BASE = 0x05700000
endif

ifdef CONFIG_EDB9302
TEXT_BASE = 0x05700000
endif

ifdef CONFIG_EDB9302A
TEXT_BASE = 0xc5700000
endif

ifdef CONFIG_EDB9307
TEXT_BASE = 0x01f00000
endif

ifdef CONFIG_EDB9307A
TEXT_BASE = 0xc1f00000
endif

ifdef CONFIG_EDB9312
TEXT_BASE = 0x01f00000
endif

ifdef CONFIG_EDB9315
TEXT_BASE = 0x01f00000
endif

ifdef CONFIG_EDB9315A
TEXT_BASE = 0xc1f00000
endif