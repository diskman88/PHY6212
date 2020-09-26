#include "hid_service.h"
#include "../gap.h"

#define TAG_HID         "hid"

static uint8_t report_map[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x06,       // Usage (Keyboard)

    0xA1, 0x01,       // Collection (Application)
    0x05, 0x07,       // Usage Page (Key Codes)
    0x19, 0xe0,       // Usage Minimum (0xE0 -> LeftControl)
    0x29, 0xe7,       // Usage Maximum (0xE7 -> Right GUI)
    0x15, 0x00,       // Logical Minimum (0)
    0x25, 0x01,       // Logical Maximum (1)
    0x75, 0x01,       // Report Size (1)
    0x95, 0x08,       // Report Count (8)
    0x81, 0x02,       // Input (Data, Variable, Absolute)
    0x95, 0x01,       // Report Count (1)
    0x75, 0x08,       // Report Size (8)
    0x81, 0x01,       // Input (Constant) reserved byte(1)
    //LED输出报告
    0x95, 0x05,       // Report Count (5)
    0x75, 0x01,       // Report Size (1)
    0x05, 0x08,       // Usage Page (Page# for LEDs)
    0x19, 0x01,       // Usage Minimum (1)
    0x29, 0x05,       // Usage Maximum (5)
    0x91, 0x02,       // Output (Data, Variable, Absolute), Led report
    0x95, 0x01,       // Report Count (1)
    0x75, 0x03,       // Report Size (3)
    0x91, 0x01,       // Output (Data, Variable, Absolute), Led report padding

    0x95, 0x06,       // Report Count (6)
    0x75, 0x08,       // Report Size (8)
    0x15, 0x00,       // Logical Minimum (0)
    0x25, 0x65,       // Logical Maximum (101)
    0x05, 0x07,       // Usage Page (Key codes)
    0x19, 0x00,       // Usage Minimum (0)
    0x29, 0x65,       // Usage Maximum (101)  0x65    Keyboard Application
    0x81, 0x00,       // Input (Data, Array) Key array(6 bytes)
    /*
        0x09, 0x05,       // Usage (Vendor Defined)
        0x15, 0x00,       // Logical Minimum (0)
        0x26, 0xFF, 0x00, // Logical Maximum (255)
        0x75, 0x08,       // Report Count (2)
        0x95, 0x02,       // Report Size (8 bit)
        0xB1, 0x02,       // Feature (Data, Variable, Absolute)
    */
    0xC0              // End Collection (Application)
};

static hids_handle_t g_hids_handle = NULL;

static uint8_t report_output_data[1] = {
    0x00,
};

static press_key_data  send_data;

static void event_output_write(ble_event_en event, void *event_data)
{
    // CTRL_LED led_status;

    evt_data_gatt_char_write_t *e = (evt_data_gatt_char_write_t *)event_data;
    LOGI(TAG_HID, "EVENT_GATT_CHAR_WRITE_CB,%x,%x\r\n", e->len, e->data[0]);

    if (e->len == 1) {
        report_output_data[0] = e->data[0];
        // led_status.data = e->data[0];
        // Light_Led(led_status);
    }
}

int hid_key_board_send(press_key_data *senddata)
{
    int iflag = 0;

    //code data not 0
    if ( g_gap_data.conn_handle != -1) {
        iflag = hids_key_send(g_hids_handle, (uint8_t *)(senddata), sizeof(press_key_data));
        // memset(senddata, 0, sizeof(press_key_data));
        // iflag |= hids_key_send(g_hids_handle, (uint8_t *)(senddata), sizeof(press_key_data));
        return iflag;
    }

    return -1;
}

int hid_service_init()
{
    int s_flag;

    g_hids_handle = hids_init(HIDS_REPORT_PROTOCOL_MODE);

    if (g_hids_handle == NULL) {
        LOGE(TAG_HID, "HIDS init FAIL!!!!");
        return -1;
    }

    s_flag = set_data_map(report_map, sizeof(report_map), REPORT_MAP);

    if (s_flag == -1) {
        LOGE(TAG_HID, "set_report_map FAIL!!!!");
        return s_flag;
    }

    s_flag = set_data_map((uint8_t *)(&send_data), sizeof(send_data), REPORT_INPUT);

    if (s_flag == -1) {
        LOGE(TAG_HID, "set_report_input FAIL!!!!");
        return s_flag;
    }

    s_flag = set_data_map(report_output_data, sizeof(report_output_data), REPORT_OUTPUT);

    if (s_flag == -1) {
        LOGE(TAG_HID, "set_report_output FAIL!!!!");
        return s_flag;
    }

    s_flag = init_hids_call_func(HIDS_IDX_REPORT_OUTPUT_VAL, (void *)event_output_write);

    if (s_flag == -1) {
        LOGE(TAG_HID, "init_hids_call_func FAIL!!!!");
        return s_flag;
    }
    
    return 0;
}