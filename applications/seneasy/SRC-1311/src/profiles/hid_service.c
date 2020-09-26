
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <aos/log.h>
#include <aos/ble.h>

#include "hid_service.h"
#define TAG "HIDS"

static char ReportDescriptor[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0xa1, 0x01,                    // COLLECTION (Application)
#if HID_DEVICE_KEYBOARD
    0x05, 0x01,                    //   USAGE_PAGE (Generic Desktop) 
    0x09, 0x06,                    //   USAGE (Keyboard)
    0xa1, 0x01,                    //   COLLECTION (Application)
    0x85, HID_REPORT_ID_KEYBOARD,  //     REPORT_ID (1)
    0x05, 0x07,                    //     USAGE_PAGE (Keyboard)
    0x19, 0xe0,                    //     USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,                    //     USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x95, 0x08,                    //     REPORT_COUNT (8)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    0x95, 0x06,                    //     REPORT_COUNT (6)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x65,                    //     LOGICAL_MAXIMUM (101)
    0x05, 0x07,                    //     USAGE_PAGE (Keyboard)
    0x19, 0x00,                    //     USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                    //     USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    //     INPUT (Data,Ary,Abs)
    0xc0,                          //   END_COLLECTION
#endif

#if HID_DEVICE_MOUSE
    0x05, 0x01,                    //   USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                    //   USAGE (Mouse)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x09, 0x01,                    //     USAGE (Pointer)
    0xa1, 0x00,                    //     COLLECTION (Physical)
    0x85, HID_REPORT_ID_MOUSE,     //       REPORT_ID (2)
    0x05, 0x09,                    //       USAGE_PAGE (Button)
    0x19, 0x01,                    //       USAGE_MINIMUM (Button 1)
    0x29, 0x03,                    //       USAGE_MAXIMUM (Button 3)
    0x15, 0x00,                    //       LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //       LOGICAL_MAXIMUM (1)
    0x95, 0x03,                    //       REPORT_COUNT (3)
    0x75, 0x01,                    //       REPORT_SIZE (1)
    0x81, 0x02,                    //       INPUT (Data,Var,Abs)
    0x05, 0x01,                    //       USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //       USAGE (X)
    0x09, 0x31,                    //       USAGE (Y)
    0x15, 0x81,                    //       LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //       LOGICAL_MAXIMUM (127)
    0x95, 0x02,                    //       REPORT_COUNT (2)
    0x75, 0x08,                    //       REPORT_SIZE (8)
    0x81, 0x06,                    //       INPUT (Data,Var,Rel)
    0xc0,                          //     END_COLLECTION
    0xc0,                          //   END_COLLECTION
#endif

#if HID_DEVICE_CONSUMER
    0x05, 0x0c,                    //   USAGE_PAGE (Consumer Devices)
    0x09, 0x01,                    //   USAGE (Consumer Control)
    0xa1, 0x01,                    //   COLLECTION (Application)
    0x85, HID_REPORT_ID_CONSUMER,  //     REPORT_ID (3)
    0x05, 0x0c,                    //     USAGE_PAGE (Consumer Devices)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x09, 0xea,                    //     USAGE (Volume Down)
    0x09, 0xe9,                    //     USAGE (Volume Up)
    0x09, 0xcf,                    //     USAGE (Voice OPEN)
    0x0a, 0xcc, 0xcc,              //     USAGE (voice CLOSE) 
    0x0a, 0x24, 0x02,              //     USAGE (AC BACK)
    0x0a, 0x23, 0x02,              //     USAGE (AC HOME)
    0x09, 0x41,                    //     USAGE (Menu  Pick)
    0x09, 0x30,                    //     USAGE (Power)
    0x81, 0x00,                    //     INPUT (Data,Ary,Abs)
    0x95, 0x08,                    //     REPORT_COUNT (8)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0xc0,                          //   END_COLLECTION
#endif

#if HID_DEVICE_VENDOR
    0x06, 0x00, 0xff,              //   USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    //   USAGE (Vendor Usage 1)
    0xa1, 0x01,                    //   COLLECTION (Application)
    0x85, HID_REPORT_ID_VENDOR,    //     REPORT_ID (4)
    0x15, 0x00,                    //     Logical Minimum */
    0x25, 0xff,                    //     Logical Maximum */
    0x19, 0x00,                    //     Usage Minimum */
    0x29, 0xff,                    //     Usage Maximun */
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x40,                    //     REPORT_COUNT (64)
    0x81, 0x00,                    //     INPUT (Data,Ary,Abs)
    0xc0,                          //   END_COLLECTION
#endif
    0xc0                           // END_COLLECTION
};

/**
 * @brief hid device info
 * 
 */
enum {
    HIDS_REMOTE_WAKE = 0x01,
    HIDS_NORMALLY_CONNECTABLE = 0x02,
};
typedef struct hids_info_t {
    uint16_t bcdHID;
    uint8_t countryCode;
    uint8_t flags;
} hids_info_t;
static hids_info_t g_hids_info = {
    0x0101,
    0x00,
    HIDS_NORMALLY_CONNECTABLE | HIDS_REMOTE_WAKE,
};
/**
 * @brief hid report ref
 * 
 */
enum {
    HIDS_INPUT = 0x01,
    HIDS_OUTPUT = 0x02,
    HIDS_FEATURE = 0x03,
};
typedef struct hids_report_ref_t {
    uint8_t id;
    uint8_t type;
} hids_report_ref_t;

//static hids_report_ref_t report_feature_ref = {
//    0x00,
//    HIDS_FEATURE,
//};
#if HID_DEVICE_KEYBOARD
static struct bt_gatt_ccc_cfg_t ccc_data_keyboard[2] = {};
static hids_report_ref_t report_ref_keybaord = {.id = HID_REPORT_ID_KEYBOARD, .type = HIDS_INPUT};
#endif

#if HID_DEVICE_MOUSE    
static struct bt_gatt_ccc_cfg_t ccc_data_mouse[2] = {};
static hids_report_ref_t report_ref_mouse = {.id = HID_REPORT_ID_MOUSE, .type = HIDS_INPUT};
#endif

#if HID_DEVICE_CONSUMER
static struct bt_gatt_ccc_cfg_t ccc_data_consumer[2] = {};
static hids_report_ref_t report_ref_consumer = {.id = HID_REPORT_ID_CONSUMER, .type = HIDS_INPUT};
#endif

#if HID_DEVICE_VENDOR
static struct bt_gatt_ccc_cfg_t ccc_data_vendor[2] = {};
static hids_report_ref_t report_ref_vendor = {.id = HID_REPORT_ID_VENDOR, .type = HIDS_INPUT};
#endif

static hids_t hids_composite = {0};

gatt_attr_t  hids_composite_attrs[] = {
    /**0.hid primary service **/
    GATT_PRIMARY_SERVICE_DEFINE(UUID_HIDS),
    /* 1. HID report map */
    GATT_CHAR_DEFINE(UUID_HIDS_REPORT_MAP, GATT_CHRC_PROP_READ),
    GATT_CHAR_VAL_DEFINE(UUID_HIDS_REPORT_MAP, GATT_PERM_READ),
    /* 3. HID Information */
    GATT_CHAR_DEFINE(UUID_HIDS_INFO, GATT_CHRC_PROP_READ),
    GATT_CHAR_VAL_DEFINE(UUID_HIDS_INFO, GATT_PERM_READ),
    /* 5. HID Control Point */
    GATT_CHAR_DEFINE(UUID_HIDS_CTRL_POINT, GATT_CHRC_PROP_WRITE_WITHOUT_RESP),
    GATT_CHAR_VAL_DEFINE(UUID_HIDS_CTRL_POINT, GATT_PERM_WRITE),
    /* 7. Protocol Mode *///low power Suspend mode ,0x00 Boot Protocol Mode 0x01 report protocol mode
    GATT_CHAR_DEFINE(UUID_HIDS_PROTOCOL_MODE, GATT_CHRC_PROP_READ | GATT_CHRC_PROP_WRITE_WITHOUT_RESP),
    GATT_CHAR_VAL_DEFINE(UUID_HIDS_PROTOCOL_MODE, GATT_PERM_READ | GATT_PERM_WRITE),

    /** reports **/
#if HID_DEVICE_KEYBOARD
    /* 9 */
    GATT_CHAR_DEFINE(UUID_HIDS_REPORT, GATT_CHRC_PROP_READ | GATT_CHRC_PROP_WRITE | GATT_CHRC_PROP_NOTIFY),
    GATT_CHAR_VAL_DEFINE(UUID_HIDS_REPORT, GATT_PERM_READ | GATT_PERM_READ_AUTHEN),
    GATT_CHAR_DESCRIPTOR_DEFINE(UUID_HIDS_REPORT_REF, GATT_PERM_READ),
    GATT_CHAR_CCC_DEFINE(ccc_data_keyboard),
#endif

#if HID_DEVICE_MOUSE
    GATT_CHAR_DEFINE(UUID_HIDS_REPORT, GATT_CHRC_PROP_READ | GATT_CHRC_PROP_WRITE | GATT_CHRC_PROP_NOTIFY),
    GATT_CHAR_VAL_DEFINE(UUID_HIDS_REPORT, GATT_PERM_READ | GATT_PERM_READ_AUTHEN),
    GATT_CHAR_DESCRIPTOR_DEFINE(UUID_HIDS_REPORT_REF, GATT_PERM_READ),
    GATT_CHAR_CCC_DEFINE(ccc_data_mouse),
#endif

#if HID_DEVICE_CONSUMER
    GATT_CHAR_DEFINE(UUID_HIDS_REPORT, GATT_CHRC_PROP_READ | GATT_CHRC_PROP_WRITE | GATT_CHRC_PROP_NOTIFY),
    GATT_CHAR_VAL_DEFINE(UUID_HIDS_REPORT, GATT_PERM_READ | GATT_PERM_READ_AUTHEN),
    GATT_CHAR_DESCRIPTOR_DEFINE(UUID_HIDS_REPORT_REF, GATT_PERM_READ),
    GATT_CHAR_CCC_DEFINE(ccc_data_consumer),
#endif

#if HID_DEVICE_VENDOR
    GATT_CHAR_DEFINE(UUID_HIDS_REPORT, GATT_CHRC_PROP_READ | GATT_CHRC_PROP_WRITE | GATT_CHRC_PROP_NOTIFY),
    GATT_CHAR_VAL_DEFINE(UUID_HIDS_REPORT, GATT_PERM_READ | GATT_PERM_READ_AUTHEN),
    GATT_CHAR_DESCRIPTOR_DEFINE(UUID_HIDS_REPORT_REF, GATT_PERM_READ),
    GATT_CHAR_CCC_DEFINE(ccc_data_vendor),
#endif
};


static void event_char_read(ble_event_en event, evt_data_gatt_char_read_t *e)
{
    if (hids_composite.conn_handle == 0xFFFF || e->char_handle < hids_composite.svc_handle || e->char_handle >= hids_composite.svc_handle + HIDS_IDX_MAX) {
        return;
    }

    uint16_t hids_char_idx = e->char_handle - hids_composite.svc_handle;

    switch (hids_char_idx) {
        case HIDS_IDX_REPORT_MAP_VAL:
            e->data = (uint8_t *)ReportDescriptor;
            e->len = sizeof(ReportDescriptor);
            break;
        case HIDS_IDX_INFO_VAL:
            e->data = (uint8_t *)&g_hids_info;
            e->len = sizeof(g_hids_info);
            break;
        case HIDS_IDX_PROTOCOL_MODE_VAL:
            e->data = &(hids_composite.protocol_mode);
            e->len = 1;
            break;

#if HID_DEVICE_KEYBOARD
        case HIDS_IDX_REPORT_KEYBOARD_VAL:
            break;
        case HIDS_IDX_REPORT_KEYBOARD_REF:
            e->data = (uint8_t *)&report_ref_keybaord;
            e->len = sizeof(report_ref_keybaord);
            break;
#endif

#if HID_DEVICE_MOUSE
        case HIDS_IDX_REPORT_MOUSE_VAL:
            break;
        case HIDS_IDX_REPORT_MOUSE_REF:
            e->data = (uint8_t *)&report_ref_mouse;
            e->len = sizeof(report_ref_mouse);
            break;
#endif

#if HID_DEVICE_CONSUMER
        case HIDS_IDX_REPORT_CONSUMER_VAL:
            break;
        case HIDS_IDX_REPORT_CONSUMER_REF:
            e->data = (uint8_t *)&report_ref_consumer;
            e->len = sizeof(report_ref_consumer);
            break;
#endif

#if HID_DEVICE_VENDOR
        case HIDS_IDX_REPORT_VENDOR_VAL:
            break;
        case HIDS_IDX_REPORT_VENDOR_REF:
            e->data = (uint8_t *)&report_ref_vendor;
            e->len = sizeof(report_ref_vendor);
            break;
#endif

        default:
            LOGI(TAG, "unhandle event:%x\r\n\r\n", hids_char_idx);
            break;
    }
}

static void event_char_write(ble_event_en event, evt_data_gatt_char_write_t *e)
{
    int ires = 0;

    if (hids_composite.conn_handle == 0xFFFF || e->char_handle < hids_composite.svc_handle || e->char_handle >= hids_composite.svc_handle + HIDS_IDX_MAX) {
        return;
    }

    uint16_t hids_char_idx = e->char_handle - hids_composite.svc_handle;

    switch (hids_char_idx) {
        case HIDS_IDX_CONTROL_POINT_VAL:
            LOGI(TAG, "control cmd %d, %s\n", e->data[0], e->data[0] == 0x00 ? " Suspend" :
                 "Exit Suspend");
            break;
        case HIDS_IDX_PROTOCOL_MODE_VAL:
            hids_composite.protocol_mode = e->data[0];
            break;

        default:
            break;
    }

    if (ires != 0) {
        LOGI(TAG, "event_char_write execute err\r\n");
    }
}

static void event_char_ccc_change(ble_event_en event, evt_data_gatt_char_ccc_change_t *e)
{
    if (hids_composite.conn_handle == 0xFFFF || e->char_handle < hids_composite.svc_handle || e->char_handle >= hids_composite.svc_handle + HIDS_IDX_MAX) {
        return;
    }

    uint16_t hids_char_idx = e->char_handle - hids_composite.svc_handle;

    switch (hids_char_idx) {
#if HID_DEVICE_KEYBOARD
        case HIDS_IDX_REPORT_KEYBOARD_CCC:
            hids_composite.ccc_keyboard = e->ccc_value;
            break;
#endif

#if HID_DEVICE_MOUSE
        case HIDS_IDX_REPORT_MOUSE_CCC:
            hids_composite.ccc_mouse = e->ccc_value;
            break;
#endif

#if HID_DEVICE_CONSUMER
        case HIDS_IDX_REPORT_CONSUMER_CCC:
            hids_composite.ccc_consumer = e->ccc_value;
            break;
#endif

#if HID_DEVICE_VENDOR
        case HIDS_IDX_REPORT_VENDOR_CCC:
            hids_composite.ccc_vendor = e->ccc_value;

#endif 
        default:
            break;
    }
}

static void conn_change(ble_event_en event, evt_data_gap_conn_change_t *e)
{
    if (e->connected == CONNECTED) {
        hids_composite.conn_handle = e->conn_handle;
    } else {
        hids_composite.conn_handle = 0xFFFF;
    }
}

static int hids_event_callback(ble_event_en event, void *event_data)
{
    switch (event) {
        case EVENT_GATT_CHAR_READ:
            event_char_read(event, event_data);
            break;

        case EVENT_GATT_CHAR_WRITE:
            event_char_write(event, event_data);
            break;

        case EVENT_GAP_CONN_CHANGE:
            conn_change(event, event_data);
            break;

        case EVENT_GATT_CHAR_CCC_CHANGE:
            event_char_ccc_change(event, event_data);
            break;

        default:
            break;
    }

    return 0;
}

static ble_event_cb_t ble_cb = {
    .callback = hids_event_callback,
};

gatt_service hids_service;

hids_handle_t hids_composite_init()
{
    int ret = 0;
    ret = ble_stack_event_register(&ble_cb);
    if (ret) {
        return NULL;
    }

    ret = ble_stack_gatt_registe_service(&hids_service,hids_composite_attrs, BLE_ARRAY_NUM(hids_composite_attrs));
    if (ret < 0) {
        return NULL;
    }

    hids_composite.conn_handle = 0xFFFF;
    hids_composite.svc_handle = ret;
    hids_composite.protocol_mode = HIDS_REPORT_PROTOCOL_MODE;

    return &hids_composite;
}

#if HID_DEVICE_KEYBOARD
int hids_send_keyboard(hid_keybaord_report_t *p_report)
{
    if(hids_composite.conn_handle == 0xFFFF) {
        return -BLE_STACK_ERR_NULL;
    }

    if (hids_composite.protocol_mode == HIDS_REPORT_PROTOCOL_MODE && hids_composite.ccc_keyboard == CCC_VALUE_NOTIFY) {
        return ble_stack_gatt_notificate(hids_composite.conn_handle, hids_composite.svc_handle + HIDS_IDX_REPORT_KEYBOARD_VAL, (uint8_t *)p_report, sizeof(hid_keybaord_report_t));
    } else {
        return -BLE_STACK_ERR_NULL;
    }
}
#endif

#if HID_DEVICE_MOUSE
int hids_send_mouse(hid_mouse_report_t * p_report)
{
    if(hids_composite.conn_handle == 0xFFFF) {
        return -BLE_STACK_ERR_NULL;
    }

    if (hids_composite.ccc_mouse == CCC_VALUE_NOTIFY) {
        return ble_stack_gatt_notificate(hids_composite.conn_handle, hids_composite.svc_handle + HIDS_IDX_REPORT_MOUSE_VAL, (uint8_t *)p_report, sizeof(hid_mouse_report_t));
    } else {
        return -BLE_STACK_ERR_NULL;
    }    
}
#endif

#if HID_DEVICE_CONSUMER
int hids_send_consumer(hid_consumer_report_t * p_report)
{
    if(hids_composite.conn_handle == 0xFFFF) {
        return -BLE_STACK_ERR_NULL;
    }

    if (hids_composite.ccc_consumer == CCC_VALUE_NOTIFY) {
        return ble_stack_gatt_notificate(hids_composite.conn_handle, hids_composite.svc_handle + HIDS_IDX_REPORT_CONSUMER_VAL, (uint8_t *)p_report, sizeof(hid_consumer_report_t));
    } else {
        return -BLE_STACK_ERR_NULL;
    }    
}
#endif

#if HID_DEVICE_VENDOR
int hids_send_vendor(uint8_t * data, int len)
{
    if(hids_composite.conn_handle == 0xFFFF) {
        return -BLE_STACK_ERR_NULL;
    }

    if (hids_composite.ccc_vendor == CCC_VALUE_NOTIFY) {
        return ble_stack_gatt_notificate(hids_composite.conn_handle, hids_composite.svc_handle + HIDS_IDX_REPORT_CONSUMER_VAL, data, len);
    } else {
        return -BLE_STACK_ERR_NULL;
    }   
}
#endif
