##
 # Copyright (C) 2017 C-SKY Microsystems Co., All rights reserved.
 #
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 #   http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
##

$(info )
$(info *************************************************)
$(info  Use: [make    ] for completely build.)
$(info  Use: [make clean] for remove temporary files .)
$(info *************************************************)
$(info )

EXE_EXT:=$(shell which ls | grep -o .exe)
MK_BASE_PATH:= .
include $(MK_BASE_PATH)/defconfig

YOC_DIR := $(MK_BASE_PATH)/../../../../yoc_sdk
YOC_LIBS := $(CONFIG_LIBS) $(CONFIG_IOT_LIBS)
YOC_DEFS :=

####set out path
MK_OUT_PATH:=  out
MK_OUT_PATH_LIB:= $(MK_OUT_PATH)/lib
MK_GENERATED_IMGS_PATH:=generated


#####################################################################
DEFINE_LOCAL:= $(YOC_DIR)/tools/build/core/define_local.mk
BUILD_MODULE:= $(YOC_DIR)/tools/build/core/build_module.mk
include $(YOC_DIR)/tools/build/core/functions.mk
include $(YOC_DIR)/tools/build/core/yoc_defs.mk

#####################################################################
MK_TOOLCHAIN_PREFIX:=

MK_L_OBJS:=
MK_L_SDK_OBJS:=
MK_L_LIBS:=
MK_L_PRE_LIBS:=
MK_L_EXCLUDE_LIBS:=
MK_CFLAGS:= $(YOC_DEFS)
MK_HOST_BIN:=
LDFLAGS:= -r

include $(YOC_DIR)/tools/build/core/toolchain.mk

YOC_SDK := $(YOC_DIR)/$(CONFIG_CHIP_CPU)/
#####################################################################
CPRE := @
ifeq ($(V),1)
CPRE :=
endif

#####################################################################
TARGETS_ROOT_PATH := $(YOC_DIR)/$(CONFIG_CHIP_CPU)/boards/$(CONFIG_CHIP_VENDOR)/$(CONFIG_BOARD_NAME)
include $(TARGETS_ROOT_PATH)/CFLAGS.mk
#####################################################################
.PHONY:startup
startup: all

all: image hexfiles
ifeq ($(CONFIG_CPU_CM0),y)
	$(CPRE) $(READELF) -S yoc.elf
else
	@bash $(YOC_DIR)/tools/build/showmap.bsh
endif

#####################################################################

include src/build.mk

BIN_NAME := yoc
IMAGE_FILE := $(MK_OUT_PATH)/$(BIN_NAME)

TARGETS_LD_SCRIPT := $(shell echo boards/$(CONFIG_BOARD_NAME)/configs/gcc_eflash.ld)

L_LDFLAGS += -mcpu=$(CONFIG_CHIP_CPU) -nostartfiles -Wl,--gc-sections -lm
ifeq ($(CONFIG_CPU_CM0),y)
L_LDFLAGS += -Wl,-Map='$(MK_OUT_PATH)/$(BIN_NAME).map'
else
L_LDFLAGS += -Wl,-ckmap='$(MK_OUT_PATH)/$(BIN_NAME).map'
endif
L_LDFLAGS += -T $(TARGETS_LD_SCRIPT)
L_LDFLAGS += -L $(YOC_SDK)lib -L $(YOC_SDK)lib/prelibs

image: $(IMAGE_FILE)
$(IMAGE_FILE): $(MK_L_LIBS) $(MK_L_PRE_LIBS)
	@ echo [INFO] Create $(BIN_NAME) images
	$(CPRE) sh $(YOC_DIR)/tools/build/gen_ldfile.sh boards/$(CONFIG_BOARD_NAME)/configs/config.yaml $(TARGETS_LD_SCRIPT).S $(TARGETS_LD_SCRIPT)
	$(CPRE) $(CC) -o $@.elf -Wl,--whole-archive $^ $(YOC_LIBS:%=-l%) $(THIRD_PARTY_LIBS) -Wl,--no-whole-archive $(L_LDFLAGS) -Wl,-zmax-page-size=1024 $(TARGETS_ROOT_PATH)/rom1Sym.txt
	$(CPRE) rm $(TARGETS_LD_SCRIPT)

	$(CPRE) $(OBJDUMP) -d $@.elf > $@.asm
	$(CPRE) $(READELF) -s $@.elf > $@.s
ifeq ($(CONFIG_CPU_CM0),y)
else
	$(CPRE) $(OBJCOPY) -O binary $@.elf $@.bin
endif
	$(CPRE) cp $(MK_OUT_PATH)/$(BIN_NAME).* ./

.PHONY:hexfiles
hexfiles:
	@echo [INFO] Create bin files
	$(CPRE) rm -fr $(MK_GENERATED_IMGS_PATH)
	@mkdir $(MK_GENERATED_IMGS_PATH) $(MK_GENERATED_IMGS_PATH)/data -p
ifeq ($(CONFIG_CPU_CM0),y)
	$(CPRE) cp $(TARGETS_ROOT_PATH)/bootimgs/boot $(MK_GENERATED_IMGS_PATH)/data/
	$(CPRE) cp $(TARGETS_ROOT_PATH)/bootimgs/bomtb $(MK_GENERATED_IMGS_PATH)/data/
	$(CPRE) cp boards/$(CONFIG_BOARD_NAME)/script/genbin.sh $(MK_GENERATED_IMGS_PATH)/data/
	$(CPRE) sh $(MK_GENERATED_IMGS_PATH)/data/genbin.sh $(MK_GENERATED_IMGS_PATH)/data
	$(CPRE) cp boards/$(CONFIG_BOARD_NAME)/configs/config.yaml $(MK_GENERATED_IMGS_PATH)/data
	$(CPRE) $(YOC_DIR)/tools/build/product$(EXE_EXT) image $(MK_GENERATED_IMGS_PATH)/images.zip -i $(MK_GENERATED_IMGS_PATH)/data -p -l -v "v1.0"
	$(CPRE) $(YOC_DIR)/tools/build/product$(EXE_EXT) image $(MK_GENERATED_IMGS_PATH)/images.zip -e $(MK_GENERATED_IMGS_PATH) -x
	$(CPRE) cp $(MK_GENERATED_IMGS_PATH)/total_image.hex $(MK_GENERATED_IMGS_PATH)/total_image.hexf
	$(CPRE) rm -fr $(MK_GENERATED_IMGS_PATH)/data
else
	$(CPRE) cp $(TARGETS_ROOT_PATH)/bootimgs/boot $(MK_GENERATED_IMGS_PATH)/data/
	$(CPRE) cp $(TARGETS_ROOT_PATH)/bootimgs/tee $(MK_GENERATED_IMGS_PATH)/data/
	$(CPRE) cp boards/$(CONFIG_BOARD_NAME)/configs/config.yaml $(MK_GENERATED_IMGS_PATH)/data/
	$(CPRE) cp -f yoc.bin $(MK_GENERATED_IMGS_PATH)/data/prim
	$(CPRE) $(YOC_DIR)/tools/build/product$(EXE_EXT) image $(MK_GENERATED_IMGS_PATH)/images.zip -i $(MK_GENERATED_IMGS_PATH)/data -l -p
	$(CPRE) $(YOC_DIR)/tools/build/product$(EXE_EXT) image $(MK_GENERATED_IMGS_PATH)/images.zip -e $(MK_GENERATED_IMGS_PATH) -x
	$(CPRE) rm -fr $(MK_GENERATED_IMGS_PATH)/data
endif

.PHONY:clean
clean:
	rm -fr out yoc.*
