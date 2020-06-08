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

L_PATH := $(call cur-dir)

MK_CFLAGS += -Os -ffunction-sections -fdata-sections

MK_CFLAGS += \
	-g \
	-Wall \
	-ffunction-sections \
	-fdata-sections \
	--specs=rdimon.specs

MK_CFLAGS += -DADV_NCONN_CFG=0x01   -DADV_CONN_CFG=0x02   -DSCAN_CFG=0x04    -DINIT_CFG=0x08   -DBROADCASTER_CFG=0x01 -DOBSERVER_CFG=0x02   -DPERIPHERAL_CFG=0x04   -DCENTRAL_CFG=0x08   -DHOST_CONFIG=0x4

MK_CFLAGS += -D_RTE_ -DARMCM0 -DCFG_CP -DCFG_QFN32 -DCFG_SLEEP_MODE="PWR_MODE_NO_SLEEP" -DDEBUG_INFO="1"
MK_CFLAGS += -I$(YOC_SDK)include\
    -Iout/config \
    -I$(YOC_SDK)csi/libs/include \
    -I$(YOC_SDK)csi/csi_driver/include \
    -I$(YOC_SDK)csi/kernel/include \
    -I$(YOC_SDK)csi/csi_core/cmsis/include \
    -I$(YOC_SDK)csi/csi_kernel/rhino/core/include \
    -I$(YOC_SDK)csi/csi_kernel/rhino/arch/include \
    -I$(YOC_SDK)csi/csi_driver/$(CONFIG_CHIP_VENDOR)/$(CONFIG_CHIP_NAME)/include \
    -I$(YOC_SDK)csi/csi_driver/$(CONFIG_CHIP_VENDOR)/common/include \
    -I$(YOC_SDK)libs/minilibc/include \
    -I$(YOC_SDK)libs/misc/include \
    -I$(YOC_SDK)libs/yunvoice/include \
    -I$(YOC_SDK)libs/mbedtls/include \
    -I$(YOC_SDK)libs/mbedtls/configs \
    -I$(YOC_SDK)libs/libmad/include \
    -I$(YOC_SDK)libs/network/include \
    -I$(YOC_SDK)libs/libogg/include \
    -I$(YOC_SDK)libs/libfaad/include \
    -I$(YOC_SDK)libs/fifo/include \
    -I$(YOC_SDK)libs/libopus/include \
    -I$(YOC_SDK)libs/libopusenc/include \
    -I$(YOC_SDK)kernel/protocols/sal/include \
    -I$(YOC_SDK)kernel/protocols/lwip/include \
    -I$(YOC_SDK)kernel/protocols/lwip/port/include \
    -I$(TARGETS_ROOT_PATH)/include

MK_CFLAGS += \
    -I$(YOC_SDK)modules/ble_dut/include \
    -I$(YOC_SDK)modules/bt/include \
    -I$(YOC_SDK)modules/genie_app/include \
    -I$(YOC_SDK)modules/genie_app/include/genie_app \
    -I$(YOC_SDK)modules/genie_app/include/genie_mesh \
    -I$(YOC_SDK)modules/genie_app/include/mesh_model \
    -I$(YOC_SDK)kernel/protocols/bluetooth/include \
    -I$(YOC_SDK)kernel/protocols/bluetooth/bt_host/include \
    -I$(YOC_SDK)kernel/protocols/bluetooth/bt_mesh \
    -I$(YOC_SDK)kernel/protocols/bluetooth/bt_mesh/inc \
    -I$(YOC_SDK)kernel/protocols/bluetooth/bt_mesh/inc/api

