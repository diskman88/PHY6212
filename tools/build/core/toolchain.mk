#CPU Type Check

ifeq ($(CONFIG_ARCH_ARM), y)

MK_TOOLCHAIN_PREFIX:=arm-none-eabi-
ifeq ($(CONFIG_CPU_CM0), y)
CONFIG_CHIP_CPU:= cortex-m0
else
CONFIG_CHIP_CPU:= cortex-null
MK_TOOLCHAIN_PREFIX:= not-support-
endif

MK_CFLAGS += -mcpu=$(CONFIG_CHIP_CPU) -mthumb

else

ifeq ($(CONFIG_CPU_CK610), y)
MK_TOOLCHAIN_PREFIX:= csky-elf-
else
TOOLCHAIN_PREFIX_CHECK:= $(shell which csky-elfabiv2-gcc)
ifeq ($(TOOLCHAIN_PREFIX_CHECK),)
MK_TOOLCHAIN_PREFIX:=csky-abiv2-elf-
else
MK_TOOLCHAIN_PREFIX:=csky-elfabiv2-
endif
endif

ifeq ($(CONFIG_CPU_CK801), y)
CONFIG_CHIP_CPU:= ck801
else ifeq ($(CONFIG_CPU_CK802), y)
CONFIG_CHIP_CPU:= ck802
else ifeq ($(CONFIG_CPU_CK803), y)
CONFIG_CHIP_CPU:= ck803
else ifeq ($(CONFIG_CPU_CK803F), y)
CONFIG_CHIP_CPU:= ck803f
else ifeq ($(CONFIG_CPU_CK803EF), y)
CONFIG_CHIP_CPU:= ck803ef
else ifeq ($(CONFIG_CPU_CK804EF), y)
CONFIG_CHIP_CPU:= ck804ef
else ifeq ($(CONFIG_CPU_CK803EFR1), y)
CONFIG_CHIP_CPU:= ck803efr1
else ifeq ($(CONFIG_CPU_CK805F), y)
CONFIG_CHIP_CPU:= ck805f
else ifeq ($(CONFIG_CPU_CK805), y)
CONFIG_CHIP_CPU:= ck805
else
CONFIG_CHIP_CPU:= cknull
MK_TOOLCHAIN_PREFIX:= not-support-
endif

MK_CFLAGS += -mcpu=$(CONFIG_CHIP_CPU)
endif

#####################################################################
CC:= $(MK_TOOLCHAIN_PREFIX)gcc
AR:= $(MK_TOOLCHAIN_PREFIX)ar
LD:= $(MK_TOOLCHAIN_PREFIX)ld
AS      := $(MK_TOOLCHAIN_PREFIX)as
OBJDUMP := $(MK_TOOLCHAIN_PREFIX)objdump
READELF := $(MK_TOOLCHAIN_PREFIX)readelf
OBJCOPY := $(MK_TOOLCHAIN_PREFIX)objcopy

HOST_CC:= gcc
HOST_AR:= ar
HOST_LD:= ld
HOST_AS      := as
HOST_OBJDUMP := objdump
HOST_READELF := readelf
HOST_OBJCOPY := objcopy
