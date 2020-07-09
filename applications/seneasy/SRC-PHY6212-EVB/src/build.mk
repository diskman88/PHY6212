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

include $(DEFINE_LOCAL)

L_MODULE := libmain

L_SRCS += init/cli_cmd.c \
          init/init.c

L_SRCS += app_main.c app_msg.c gap.c

# ble services
L_SRCS += $(addprefix services/, $(notdir $(wildcard src/services/*.c)))
# $(warning $(wildcard src/services/*.c))

# drivers
L_SRCS += drivers/keyscan.c

L_INCS := include \
          ../../../csi/csi_kernel/rhino/core/include \
          ../../../csi/csi_kernel/rhino/arch/include \
          ../../../out/config \
          ../../../csi/csi_kernel/include

L_SRCS += board/$(CONFIG_BOARD_NAME)/board_ble.c \
          board/$(CONFIG_BOARD_NAME)/board_devices.c \

L_CFLAGS += -Wunused-variable

$(warning $(L_SRCS))

include $(BUILD_MODULE)

