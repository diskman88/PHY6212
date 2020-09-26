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

L_MODULE := libyoc_kernel

L_CFLAGS :=

L_INCS += \
    csi/csi_kernel/rhino/core/include \
    csi/csi_kernel/rhino/arch/include

#vfs
ifeq ($(CONFIG_VFS), y)
L_INCS += kernel/vfs/include \
          kernel/fs/spiffs/include

L_SRCS += vfs/device.c \
          vfs/select.c \
          vfs/vfs_file.c \
          vfs/vfs_inode.c \
          vfs/vfs_register.c \
          vfs/vfs.c

L_SRCS += fs/spiffs/src/spiffs_cache.c \
          fs/spiffs/src/spiffs_check.c \
          fs/spiffs/src/spiffs_gc.c \
          fs/spiffs/src/spiffs_port.c
endif

ifeq ($(CONFIG_YLOOP),y)
L_SRCS += \
    eventloop/evtloop_main.c \
    eventloop/evtloop_device.c \
    eventloop/evtloop_event.c \
    eventloop/yloop.c
endif

ifeq ($(CONFIG_FATFS),y)
L_INCS += include/aos
L_INCS += kernel/vfs/include
L_INCS += kernel/fs/fatfs/include
L_INCS += kernel/fs/fatfs/src/sd_disk
L_INCS += csi/drivers/sdmmc/core
L_INCS += csi/drivers/sdmmc/host

L_SRCS += \
    fs/fatfs/src/diskio.c           \
    fs/fatfs/src/ff.c               \
    fs/fatfs/src/ffunicode.c        \
    fs/fatfs/src/fatfs_port.c        \
    fs/fatfs/src/ffsystem.c         \
    fs/fatfs/src/sd_disk/sd_disk.c
endif

L_SRCS += misc/syslog.c misc/sysinfo.c
L_SRCS += misc/settings_ali.c
L_SRCS += misc/kernel.c
L_SRCS += misc/mm.c
L_SRCS += misc/partition.c
L_SRCS += misc/softwdt.c
L_SRCS += misc/except.c
#L_SRCS += misc/select.c
L_SRCS += misc/manifest_info.c
L_SRCS += misc/list.c
#L_SRCS += misc/cop_register.c
L_SRCS += misc/kv.c
L_SRCS += misc/malloc.c

L_SRCS += uService/utask.c
L_SRCS += uService/uservice.c
L_SRCS += uService/event.c
L_SRCS += uService/rpc.c
L_SRCS += uService/event_svr.c
#L_SRCS += ipc/ipc.c
#L_SRCS += ipc/channel_mailbox.c

#L_SRCS += protocols/lwip/port/sys_arch.c

ifeq ($(CONFIG_IPC_MAILBOX), y)
# L_INCS += kernel/ipc/mailbox kernel/ipc kernel/ipc/mailbox_test
# L_SRCS += ipc/mailbox/channel_mailbox.c
# #L_SRCS += ipc/mailbox_test/mailbox_cli.c ipc/mailbox_test/mailbox_srv.c

# L_INCS += kernel/ipc/hci
# L_SRCS += ipc/hci/ipc_hci_ctrl.c ipc/hci/ipc_hci_host.c

endif

include $(BUILD_MODULE)

# include kernel/fs/build.mk
# include kernel/lpm/build.mk
# include kernel/vfs/build.mk
# include kernel/eventloop/build.mk

include kernel/protocols/build.mk
include kernel/fs/build.mk
