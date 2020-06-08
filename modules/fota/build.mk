##
 # Copyright (C) 2018 C-SKY Microsystems Co., All rights reserved.
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

L_MODULE := libfota

L_CFLAGS := -Wall

L_INCS := \
	$(L_PATH)/include \
	kernel/protocols/network \
	modules/yunio/lwm2m/core

L_SRCS += \
	netio/flash.c \
	netio/netio.c \
	netio/coap.c \
	netio/http.c \
	netio/serial.c

L_SRCS += \
	fota/fota.c \
	fota/fota_coap.c \
	fota/fota_cop.c \
	fota/fota_serial.c

L_SRCS += \
	http/http.c

include $(BUILD_MODULE)
