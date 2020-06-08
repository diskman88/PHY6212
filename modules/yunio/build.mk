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

# this mk not BUILD_MODULE, cannot use L_PATH
CUR_PATH := $(call cur-dir)

ifeq ($(CONFIG_CLOUDIO_AWSIOT), y)
    include $(CUR_PATH)/awsiot/build.mk
endif

ifneq ($(CONFIG_CLOUDIO_ALIMQTT)$(CONFIG_CLOUDIO_ALICOAP),)
    include $(CUR_PATH)/aliot/build.mk
endif

ifeq ($(CONFIG_CLOUDIO_ONENET), y)
    include $(CUR_PATH)/onenet/build.mk
endif

ifeq ($(CONFIG_CLOUDIO_OCEANCON), y)
    include $(CUR_PATH)/lwm2m/build.mk
endif

