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

GAWK_CHECK:= $(shell which gawk)
ifeq ($(GAWK_CHECK),)
$(error Please install gawk first)
endif

MK_BASE_PATH:= $(shell pwd)

#####################################################################
MK_DEFCONFIG_CSI:= ../defconfig
MK_DEFCONFIG:= defconfig
DEFCONFIG_FILE_CHECK:= $(shell ls $(MK_DEFCONFIG) 2>/dev/null)
ifneq ($(DEFCONFIG_FILE_CHECK),)

include $(MK_DEFCONFIG)

CONFIG_CHIP_NAME  := $(patsubst "%",%,$(strip $(CONFIG_CHIP_NAME_STR)))
CONFIG_BOARD_NAME := $(patsubst "%",%,$(strip $(CONFIG_BOARD_NAME_STR)))
CONFIG_CHIP_VENDOR := $(patsubst "%",%,$(strip $(CONFIG_CHIP_VENDOR_STR)))

#####################################################################
DEFINE_LOCAL:= tools/build/core/define_local.mk
BUILD_MODULE:= tools/build/core/build_module.mk
include tools/build/core/functions.mk

#####################################################################
MK_TOOLCHAIN_PREFIX:=

MK_L_OBJS:=
MK_L_SDK_OBJS:=
MK_L_LIBS:=
MK_L_PRE_LIBS:=
MK_L_PRE_LIBS_SRC:=
MK_L_EXCLUDE_LIBS:=
MK_CFLAGS:=
MK_HOST_BIN:=
LDFLAGS:= -r

include tools/build/core/toolchain.mk

#####################################################################
CPRE := @
ifeq ($(V),1)
CPRE :=
endif

#####################################################################
MK_BOARD_DIR := $(shell echo $(CONFIG_CHIP_VENDOR_STR)/$(CONFIG_BOARD_NAME_STR))

#####################################################################
TARGETS_ROOT_PATH := boards/$(MK_BOARD_DIR)
TARGETS_ROOT_PATH_CSI :=../boards/$(MK_BOARD_DIR)
include boards/$(MK_BOARD_DIR)/CFLAGS.mk

#####################################################################


####set out path
MK_OUT_PATH:=  out
YOC_LIB:= yoc_sdk/$(CONFIG_CHIP_CPU)
MK_OUT_PATH_LIB:= $(YOC_LIB)/lib

MK_SDK_PATH:= ../$(CONFIG_BOARD_NAME_STR)_SDK

.PHONY:startup
startup: all

all: auto_config csi_rtos lib
	@echo YoC SDK Done

###############
auto_config:
	$(CPRE) sh tools/build/get_csi_src.sh
	$(CPRE) mkdir -p $(MK_OUT_PATH)
	$(CPRE) mkdir -p $(YOC_LIB)
	$(CPRE) mkdir -p out/config
	$(CPRE) sh tools/build/config2chead.sh $(MK_DEFCONFIG) out/config/yoc_config.h
	$(CPRE) sh tools/build/config2chead.sh mk_csi_config out/config
ifeq ($(CONFIG_BT), y)
	$(CPRE) $(MAKE) -C kernel/protocols/bluetooth
endif

###############
csi_rtos:
	$(CPRE) echo [INFO] make csi
	$(CPRE) $(MAKE) -C csi/ lib TARGETS_ROOT_PATH=$(TARGETS_ROOT_PATH_CSI) CONFIG_FILE=$(MK_DEFCONFIG_CSI) CONFIG_INC_PATH=../out/config

teeos:
	$(CPRE) echo [INFO] make teeos
	$(CPRE) mkdir -p out/config
	$(CPRE) sh tools/build/config2chead.sh $(MK_DEFCONFIG) out/config/yoc_config.h
	$(CPRE) sh tools/build/config2chead.sh mk_csi_config out/config
	$(CPRE) make -C csi/ tee_os TARGETS_ROOT_PATH=$(TARGETS_ROOT_PATH_CSI) CONFIG_FILE=$(MK_DEFCONFIG_CSI)

teeinstall:
	$(CPRE) echo "Install tee to board"
	$(CPRE) cp -f csi/csi_driver/$(CONFIG_CHIP_VENDOR_STR)/common/tee/bin/tee_os_$(CONFIG_CHIP_NAME_STR).bin $(TARGETS_ROOT_PATH)/bootimgs/tee
	$(CPRE) cp -f csi/csi_driver/$(CONFIG_CHIP_VENDOR_STR)/common/tee/bin/tee_os_$(CONFIG_CHIP_NAME_STR) $(TARGETS_ROOT_PATH)/bootimgs/tee.elf
	$(CPRE) echo "$(TARGETS_ROOT_PATH)/bootimgs/tee"

###############
bootinstall:
	$(CPRE) echo "Install boot to board"
	$(CPRE) cp -f bootloader/out/boot.bin $(TARGETS_ROOT_PATH)/bootimgs/boot
	$(CPRE) cp -f bootloader/out/boot.elf $(TARGETS_ROOT_PATH)/bootimgs/
	$(CPRE) cp -f bootloader/out/bomtb $(TARGETS_ROOT_PATH)/bootimgs/
	$(CPRE) echo "$(TARGETS_ROOT_PATH)/bootimgs/boot"

bootloader:auto_config
	$(CPRE) make -C bootloader TARGETS_DEST_CFG_FILE=$(CONFIG_BOARD_NAME_STR)
	$(CPRE) make -C csi/ clean

#####################################################################
include boards/$(MK_BOARD_DIR)/build.mk
include libs/build.mk
include drivers/build.mk
include kernel/build.mk
include modules/build.mk
#####################################################################
prelibs: $(MK_L_PRE_LIBS_SRC)
	$(CPRE) mkdir -p $(MK_OUT_PATH_LIB)/prelibs/
	$(CPRE) if [ -n "$^" ]; then cp $^ $(MK_OUT_PATH_LIB)/prelibs/; fi

lib: $(MK_L_LIBS) prelibs $(MK_L_PRE_LIBS)
#yoc
	$(CPRE) mkdir -p $(YOC_LIB)/include
	$(CPRE) cp -rf include $(YOC_LIB)/
	$(CPRE) cp -rf out/config/* $(YOC_LIB)/include/.

#modules
ifeq ($(CONFIG_MESH_GENIE_APP), y)
	$(CPRE) mkdir -p $(YOC_LIB)/modules/genie_app/include
	$(CPRE) cp -rf modules/genie_app/* $(YOC_LIB)/modules/genie_app/
	$(CPRE) rm -rf $(YOC_LIB)/modules/genie_app/include/tinycrypt
	$(CPRE) find $(YOC_LIB)/modules/genie_app/ -name '*.c' | xargs -I{} rm {}
endif

#libs
	$(CPRE) mkdir -p $(YOC_LIB)/libs/misc
	$(CPRE) cp -rf libs/misc/include $(YOC_LIB)/libs/misc

	$(CPRE) mkdir -p $(YOC_LIB)/libs/minilibc
	$(CPRE) cp -rf libs/minilibc/include $(YOC_LIB)/libs/minilibc

#boards
	$(CPRE) mkdir -p $(YOC_LIB)/boards/$(CONFIG_CHIP_VENDOR_STR)/$(CONFIG_BOARD_NAME_STR)/
	$(CPRE) cp -rf boards/$(CONFIG_CHIP_VENDOR_STR)/$(CONFIG_BOARD_NAME_STR)/bootimgs  $(YOC_LIB)/boards/$(CONFIG_CHIP_VENDOR_STR)/$(CONFIG_BOARD_NAME_STR)/
	$(CPRE) cp -rf boards/$(CONFIG_CHIP_VENDOR_STR)/$(CONFIG_BOARD_NAME_STR)/CFLAGS.mk $(YOC_LIB)/boards/$(CONFIG_CHIP_VENDOR_STR)/$(CONFIG_BOARD_NAME_STR)/
	$(CPRE) cp -rf boards/$(CONFIG_CHIP_VENDOR_STR)/$(CONFIG_BOARD_NAME_STR)/rom1Sym.txt $(YOC_LIB)/boards/$(CONFIG_CHIP_VENDOR_STR)/$(CONFIG_BOARD_NAME_STR)/
	$(CPRE) cp -rf boards/$(CONFIG_CHIP_VENDOR_STR)/$(CONFIG_BOARD_NAME_STR)/configs $(YOC_LIB)/boards/$(CONFIG_CHIP_VENDOR_STR)/$(CONFIG_BOARD_NAME_STR)/
	$(CPRE) cp -rf boards/$(CONFIG_CHIP_VENDOR_STR)/$(CONFIG_BOARD_NAME_STR)/include $(YOC_LIB)/boards/$(CONFIG_CHIP_VENDOR_STR)/$(CONFIG_BOARD_NAME_STR)/

#csi
	$(CPRE) cp -rf csi/out/libcsi.a \
			$(YOC_LIB)/lib/
	$(CPRE) mkdir -p $(YOC_LIB)/csi/csi_driver $(YOC_LIB)/csi/csi_core $(YOC_LIB)/csi/libs/include $(YOC_LIB)/csi/kernel
	$(CPRE) cp -rf csi/csi_driver/include $(YOC_LIB)/csi/csi_driver/
	$(CPRE) cp -rf csi/csi_kernel/rhino/core/include $(YOC_LIB)/csi/kernel/
	$(CPRE) cp -rf csi/csi_kernel/rhino/arch/include $(YOC_LIB)/csi/kernel/
	$(CPRE) cp -rf csi/csi_driver/$(CONFIG_CHIP_VENDOR_STR)/$(CONFIG_CHIP_NAME_STR)/include $(YOC_LIB)/csi/csi_driver/

ifeq ($(CONFIG_ARCH_ARM), y)
	$(CPRE) cp -rf csi/csi_core/cmsis $(YOC_LIB)/csi/csi_core/
else
	$(CPRE) cp -rf csi/csi_core/include $(YOC_LIB)/csi/csi_core/
endif

	$(CPRE) cp -rf csi/libs/include $(YOC_LIB)/csi/libs/

	$(CPRE) mkdir -p $(YOC_LIB)/csi/csi_driver/$(CONFIG_CHIP_VENDOR_STR)/common/
	$(CPRE) cp -rf csi/csi_driver/$(CONFIG_CHIP_VENDOR_STR)/common/include $(YOC_LIB)/csi/csi_driver/$(CONFIG_CHIP_VENDOR_STR)/common/

#tools
	$(CPRE) mkdir -p $(YOC_LIB)/../tools
	$(CPRE) cp -rf tools/build $(YOC_LIB)/../tools/
	$(CPRE) cp -rf tools/$(CONFIG_BOARD_NAME_STR)/ $(YOC_LIB)/../tools/
#kernel
	$(CPRE) mkdir -p $(YOC_LIB)/kernel/protocols/lwip/port 
	$(CPRE) cp -rf kernel/protocols/lwip/include $(YOC_LIB)/kernel/protocols/lwip/
	$(CPRE) cp -rf kernel/protocols/lwip/port/include $(YOC_LIB)/kernel/protocols/lwip/port/
#
	$(CPRE) cp -rf drivers/bt/hci_ch6121/libdrivers_bt.a $(YOC_LIB)/lib/
#bt
	$(CPRE) mkdir -p $(YOC_LIB)/modules/ble_profiles
	$(CPRE) cp -rf modules/ble_profiles/include $(YOC_LIB)/modules/ble_profiles
ifeq ($(CONFIG_BT_MESH), y)
	$(CPRE) mkdir -p $(YOC_LIB)/modules/mesh_node
	$(CPRE) cp -rf modules/mesh_node/include $(YOC_LIB)/modules/mesh_node
	$(CPRE) mkdir -p $(YOC_LIB)/modules/mesh_provisioner
	$(CPRE) cp -rf modules/mesh_provisioner/include $(YOC_LIB)/modules/mesh_provisioner
	$(CPRE) mkdir -p $(YOC_LIB)/modules/mesh_models/common
	$(CPRE) cp -rf modules/mesh_models/common/include $(YOC_LIB)/modules/mesh_models/common
	$(CPRE) mkdir -p $(YOC_LIB)/modules/mesh_models/sig_model
	$(CPRE) cp -rf modules/mesh_models/sig_model/include $(YOC_LIB)/modules/mesh_models/sig_model
	$(CPRE) mkdir -p $(YOC_LIB)/modules/mesh_models/vendor_model
	$(CPRE) cp -rf modules/mesh_models/vendor_model/include $(YOC_LIB)/modules/mesh_models/vendor_model
endif
	$(CPRE) mkdir -p $(YOC_LIB)/modules/ERS
	$(CPRE) cp -rf modules/ERS/include $(YOC_LIB)/modules/ERS
	$(CPRE) mkdir -p $(YOC_LIB)/drivers/keyboard
	$(CPRE) cp -rf drivers/keyboard/include $(YOC_LIB)/drivers/keyboard
	$(CPRE) mkdir -p $(YOC_LIB)/drivers/digitron
	$(CPRE) cp -rf drivers/digitron/include $(YOC_LIB)/drivers/digitron

	$(CPRE) mkdir -p $(YOC_LIB)/kernel/protocols/bluetooth/bt_host/include
	$(CPRE) cp -rf kernel/protocols/bluetooth/bt_host/include $(YOC_LIB)/kernel/protocols/bluetooth/bt_host/
	$(CPRE) cp -rf kernel/protocols/bluetooth/include $(YOC_LIB)/kernel/protocols/bluetooth/
ifeq ($(CONFIG_BT_MESH), y)
	$(CPRE) mkdir -p $(YOC_LIB)/kernel/protocols/bluetooth/bt_mesh
	$(CPRE) cp -rf kernel/protocols/bluetooth/bt_mesh/inc $(YOC_LIB)/kernel/protocols/bluetooth/bt_mesh/
endif
	$(CPRE) cp -rf kernel/protocols/bluetooth/include $(YOC_LIB)/kernel/protocols/bluetooth/
#tinycrypt
	$(CPRE) mkdir -p $(YOC_LIB)/modules/tinycrypt
	$(CPRE) cp -rf kernel/protocols/bluetooth/bt_crypto/tinycrypt/include $(YOC_LIB)/modules/tinycrypt
	$(CPRE) mkdir -p $(YOC_LIB)/modules/ble_dut
	$(CPRE) cp -rf modules/ble_dut/include $(YOC_LIB)/modules/ble_dut
cdk:
	$(CPRE) sh tools/$(CONFIG_BOARD_NAME_STR)/cdk/gen_cdksdk.sh

cdk_install:
	$(CPRE) sh tools/$(CONFIG_BOARD_NAME_STR)/cdk/install_cdksdk.sh

build_test:
	$(CPRE) sh tools/$(CONFIG_BOARD_NAME_STR)/build_test.sh

build_test_cdk:
	$(CPRE) sh tools/$(CONFIG_BOARD_NAME_STR)/build_test_cdk.sh

else
all:help
	@echo
#DEFCONFIG_FILE_CHECK
endif

.PHONY:clean
clean:
	rm -rf $(MK_OUT_PATH) yoc_sdk
	make -C csi/ clean
	make -C bootloader clean
	rm -fr yoc.* include/yoc_config.h
ifeq ($(CONFIG_BT), y)
	$(CPRE) make -C kernel/protocols/bluetooth
endif

getcsi:
	$(CPRE) sh tools/build/get_csi_src.sh

gen_sdk:
	$(CPRE) make -f build_sdk.mk

help:
	@echo "#Build Command Example#"
	@echo "cp defconfig_ch6121_evb defconfig"
	@echo "make"
	@echo
	@echo "#After make, you can execute more commands#"
	@echo "make clean"
	@echo "make cdk"
	@echo "make build_test"
