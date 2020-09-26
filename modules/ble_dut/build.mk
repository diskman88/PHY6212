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

L_MODULE := libble_dut

L_CFLAGS += -Wall

L_INCS += $(L_PATH)/ drivers/bt/hci_ch6121/dtm kernel/protocols/bluetooth/include

L_SRCS := ble_dut_test.c dut_rf_test.c dut_uart_driver.c dut_utility.c dut_at_cmd.c

ifeq ($(CONFIG_BT), y)
include $(BUILD_MODULE)
endif