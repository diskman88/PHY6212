L_PATH := $(call cur-dir)

include $(DEFINE_LOCAL)

L_MODULE := libaters

L_CFLAGS := -Wall

L_INCS := modules/yunio/onenet/ciscore


L_SRCS := at/at_basic_cmd.c \
          at/at_service_cmd.c \
          at/at_lib.c \
          at/at_ble/at_ble.c \
          at/at_ble/at_ble_kv.c \
          at/at_fota.c

ifeq ($(CONFIG_AT), y)
include $(BUILD_MODULE)
endif

