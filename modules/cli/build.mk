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

L_MODULE := libcli

L_CFLAGS += -Wall

L_INCS := csi/csi_driver/include drivers/wifi/esp8266
L_INCS += include/aos
L_INCS += kernel/fs/fatfs/include

L_SRCS := aos_cli.c \
    cli_service.c \
    sys/cli_help.c \
    sys/cli_ps.c   \
    sys/cli_free.c \
    sys/cli_factory.c \
    sys/cli_sysinfo.c \
    sys/cli_sysconf.c \
    sys/cli_kvtool.c \
    sys/cli_lpm.c

L_SRCS += sys/cli_addr_op.c

#L_SRCS += net/cli_ping.c
#L_SRCS += net/cli_ifconfig.c
#L_SRCS += net/cli_ntp.c

ifeq ($(CONFIG_TCPIP),y)
L_SRCS += net/cli_lwip_mem.c
endif

include $(BUILD_MODULE)
