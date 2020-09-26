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

L_MODULE := libdrivers

L_CFLAGS := -Wall

L_SRCS :=

L_INCS += 

## uart driver ##
L_SRCS += uart/console_uart.c
L_SRCS += uart/uart.c
L_SRCS += uart/uart_drv.c

## flash driver ##
L_SRCS += flash/flash_drv.c
L_SRCS += flash/spiflash_drv.c
L_SRCS += flash/flash.c

## led ##
L_SRCS += led/led_rgb_drv.c
L_SRCS += led/led.c

## conmmon device ##
L_SRCS += common/device.c

## keyboard ##
L_SRCS += keyboard/keyboard.c

## digitron ##
L_SRCS += digitron/digitron.c

ifeq ($(CONFIG_BT), y)
L_SRCS += bt/hci_drv.c
endif

include $(BUILD_MODULE)

include drivers/bt/build.mk
