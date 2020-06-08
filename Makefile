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
	$(CPRE) mkdir -p $(MK_OUT_PATH)
	$(CPRE) mkdir -p $(YOC_LIB)
	$(CPRE) mkdir -p out/config
	$(CPRE) sh tools/build/config2chead.sh $(MK_DEFCONFIG) out/config/yoc_config.h
	$(CPRE) sh tools/build/config2chead.sh mk_csi_config out/config

###############
csi_rtos:
	$(CPRE) echo [INFO] make csi
	$(CPRE) $(MAKE) -C csi/ lib TARGETS_ROOT_PATH=$(TARGETS_ROOT_PATH_CSI) CONFIG_FILE=$(MK_DEFCONFIG_CSI) CONFIG_INC_PATH=../out/config

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
	$(CPRE) mkdir -p $(YOC_LIB)/modules/ble_dut/
	$(CPRE) cp -rf modules/ble_dut/include $(YOC_LIB)/modules/ble_dut/

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
	$(CPRE) mkdir -p $(YOC_LIB)/kernel/protocols/lwip/port $(YOC_LIB)/kernel/protocols/sal/
	$(CPRE) cp -rf kernel/protocols/lwip/include $(YOC_LIB)/kernel/protocols/lwip/
	$(CPRE) cp -rf kernel/protocols/lwip/port/include $(YOC_LIB)/kernel/protocols/lwip/port/

#bt

	$(CPRE) mkdir -p $(YOC_LIB)/modules/bt
	$(CPRE) cp -rf modules/bt/include $(YOC_LIB)/modules/bt
	$(CPRE) mkdir -p $(YOC_LIB)/kernel/protocols/bluetooth/bt_host/include
	$(CPRE) cp -rf kernel/protocols/bluetooth/bt_host/include $(YOC_LIB)/kernel/protocols/bluetooth/bt_host/
	$(CPRE) cp -rf kernel/protocols/bluetooth/include $(YOC_LIB)/kernel/protocols/bluetooth/

#tinycrypt
	$(CPRE) mkdir -p $(YOC_LIB)/modules/tinycrypt
	$(CPRE) cp -rf kernel/protocols/bluetooth/bt_crypto/tinycrypt/include $(YOC_LIB)/modules/tinycrypt

#prebuild libs
ifeq ($(CONFIG_BT_CENTRAL)_$(CONFIG_BT_PERIPHERAL), y_y)
	$(CPRE) cp -rf kernel/protocols/bluetooth/bt_host/central_peripheral/libbt_host.a $(YOC_LIB)/lib/
	$(CPRE) cp -rf kernel/protocols/bluetooth/bt_host/central_peripheral/libdrivers_bt.a $(YOC_LIB)/lib/
else ifeq ($(CONFIG_BT_CENTRAL), y)
	$(CPRE) cp -rf kernel/protocols/bluetooth/bt_host/central/libbt_host.a $(YOC_LIB)/lib/
	$(CPRE) cp -rf kernel/protocols/bluetooth/bt_host/central/libdrivers_bt.a $(YOC_LIB)/lib/
else ifeq ($(CONFIG_BT_PERIPHERAL), y)
	$(CPRE) cp -rf kernel/protocols/bluetooth/bt_host/peripheral/libbt_host.a $(YOC_LIB)/lib/
	$(CPRE) cp -rf kernel/protocols/bluetooth/bt_host/peripheral/libdrivers_bt.a $(YOC_LIB)/lib/
endif

	$(CPRE) cp -rf kernel/protocols/bluetooth/bt_crypto/libbt_crypto.a $(YOC_LIB)/lib/
	$(CPRE) cp -rf kernel/protocols/bluetooth/bt_crypto/libtinycrypt.a $(YOC_LIB)/lib/

	$(CPRE) cp -rf kernel/fs/kv2/libkv.a $(YOC_LIB)/lib/

else
all:help
	@echo
#DEFCONFIG_FILE_CHECK
endif

.PHONY:clean
clean:
	rm -rf $(MK_OUT_PATH) yoc_sdk
	make -C csi/ clean
	rm -fr yoc.* include/yoc_config.h

help:
	@echo "#Build Command Example#"
	@echo "cp defconfig_ch6121_evb defconfig"
	@echo "make"
	@echo
	@echo "#After make, you can execute more commands#"
	@echo "make clean"
	@echo "make cdk"
	@echo "make build_test"
