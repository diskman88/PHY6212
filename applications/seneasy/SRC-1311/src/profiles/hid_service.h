/**
 * @file hid_service.h
 * @author zhangkaihua (apple_eat@126.com)
 * @brief 
 * @version 1.0
 * @date 2020-09-15
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef __HID_SERVICE_H
#define __HID_SERVICE_H

#include <stdint.h>
#include <stdbool.h>
#include <aos/log.h>
#include <aos/ble.h>

#define HID_DEVICE_KEYBOARD      1
#define HID_DEVICE_MOUSE         1
#define HID_DEVICE_CONSUMER      1
#define HID_DEVICE_VENDOR        1

#define HID_REPORT_ID_KEYBOARD  1
#define HID_REPORT_ID_MOUSE     2
#define HID_REPORT_ID_CONSUMER  3
#define HID_REPORT_ID_VENDOR    4

typedef enum {
    HIDS_IDX_SVC,
    HIDS_IDX_REPORT_MAP_CHAR,
    HIDS_IDX_REPORT_MAP_VAL,
    HIDS_IDX_INFO_CHAR,
    HIDS_IDX_INFO_VAL,
    HIDS_IDX_CONTROL_POINT_CHAR,
    HIDS_IDX_CONTROL_POINT_VAL,
    HIDS_IDX_PROTOCOL_MODE_CHAR,
    HIDS_IDX_PROTOCOL_MODE_VAL,

#if HID_DEVICE_KEYBOARD
    HIDS_IDX_REPORT_KEYBOARD_CHAR,
    HIDS_IDX_REPORT_KEYBOARD_VAL,
    HIDS_IDX_REPORT_KEYBOARD_REF,
    HIDS_IDX_REPORT_KEYBOARD_CCC,
#endif

#if HID_DEVICE_MOUSE
    HIDS_IDX_REPORT_MOUSE_CHAR,
    HIDS_IDX_REPORT_MOUSE_VAL,
    HIDS_IDX_REPORT_MOUSE_REF,
    HIDS_IDX_REPORT_MOUSE_CCC,
#endif

#if HID_DEVICE_CONSUMER
    HIDS_IDX_REPORT_CONSUMER_CHAR,
    HIDS_IDX_REPORT_CONSUMER_VAL,
    HIDS_IDX_REPORT_CONSUMER_REF,
    HIDS_IDX_REPORT_CONSUMER_CCC,
#endif

#if HID_DEVICE_VENDOR
    HIDS_IDX_REPORT_VENDOR_CHAR,
    HIDS_IDX_REPORT_VENDOR_VAL,
    HIDS_IDX_REPORT_VENDOR_REF,
    HIDS_IDX_REPORT_VENDOR_CCC,
#endif

    HIDS_IDX_MAX,
} hids_service_index_e;

enum {
    HIDS_BOOT_PROTOCOL_MODE = 0x00,
    HIDS_REPORT_PROTOCOL_MODE = 0x01,
};

typedef struct _hids_t {
    uint16_t conn_handle;
    uint16_t svc_handle;
    uint8_t protocol_mode;

#if HID_DEVICE_KEYBOARD
    uint16_t ccc_keyboard;
    uint16_t handle_keyboard;
#endif

#if HID_DEVICE_MOUSE
    uint16_t ccc_mouse;
    uint16_t handle_mouse;
#endif

#if HID_DEVICE_CONSUMER
    uint16_t ccc_consumer;
    uint16_t handle_consumer;
#endif

#if HID_DEVICE_VENDOR
    uint16_t ccc_vendor;
    uint16_t handle_vendor;
#endif
} hids_t;
typedef hids_t *hids_handle_t;

/**
 * @brief hids_composite service 初始化
 * 
 * @return hids_handle_t:service相关handle和cccd控制
 */
hids_handle_t hids_composite_init();


#if HID_DEVICE_KEYBOARD
typedef struct  _SPECIAL_KEY_VALUE_ {
    uint8_t    Left_Ctrl        : 1;
    uint8_t    Left_Shift       : 1;
    uint8_t    Left_Alt         : 1;
    uint8_t    Left_Gui         : 1;
    uint8_t    Right_Ctrl       : 1;
    uint8_t    Right_Shift      : 1;
    uint8_t    Right_Alt        : 1;
    uint8_t    Right_Gui        : 1;
} SPECIAL_KEY_VALUE_BIT;

typedef union {
    SPECIAL_KEY_VALUE_BIT  bits;
    uint8_t           data;
} SPCL_KEY;

typedef struct {
    SPCL_KEY  modifier;
    uint8_t     Rsv;
    uint8_t   Code1;
    uint8_t   Code2;
    uint8_t   Code3;
    uint8_t   Code4;
    uint8_t   Code5;
    uint8_t   Code6;
} hid_keybaord_report_t;

/**
 * @brief hid 键盘按键发送
 * 
 * @param p_report 
 * @return int 
 */
int hids_send_keyboard(hid_keybaord_report_t * p_report);
#endif


#if HID_DEVICE_MOUSE
typedef struct 
{
    uint8_t buttons;
    char x;
    char y;
}hid_mouse_report_t;
int hids_send_mouse(hid_mouse_report_t * p_report);
#endif

#if HID_DEVICE_CONSUMER
typedef union 
{
    struct {
        uint8_t VOL_DOWN:1;
        uint8_t VOL_UP:1;
        uint8_t VOICE_OPEN:1;
        uint8_t VOICE_CLOSE:1;
        uint8_t AC_BACK:1;
        uint8_t AC_HOME:1;
        uint8_t MENU_PICK:1;
        uint8_t POWER:1;
    }button;
    uint8_t data;
}hid_consumer_report_t;
int hids_send_consumer(hid_consumer_report_t * p_report);
#endif

#if HID_DEVICE_VENDOR
int hids_send_vendor(uint8_t * data, int len);
#endif

#endif