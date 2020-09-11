L_PATH := $(call cur-dir)

include $(DEFINE_LOCAL)

L_MODULE := libaters

L_CFLAGS := -Wall

L_INCS := modules/yunio/onenet/ciscore

L_SRCS := at/at_basic_cmd.c \
          at/at_service_cmd.c \
          at/at_lib.c

ifeq ($(CONFIG_AT_OTA), y)
L_SRCS += at/at_fota.c
endif

ifeq ($(CONFIG_BT_CENTRAL)_$(CONFIG_BT_PERIPHERAL), y_y)
L_SRCS += at/at_ble/at_ble.c \
          at/at_ble/at_ble_kv.c
endif

ifeq ($(CONFIG_BT_MESH), y)
L_SRCS += at/at_mesh/at_mesh.c \
          at/at_mesh/at_mesh_com_init.c
endif

ifeq ($(CONFIG_AT), y)
include $(BUILD_MODULE)
endif

