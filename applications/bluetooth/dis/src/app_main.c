/*
 * Copyright (C) 2017 C-SKY Microsystems Co., All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <yoc_config.h>
#include <aos/log.h>
#include <aos/kernel.h>
#include <aos/ble.h>
#include <app_init.h>
#include <yoc/dis.h>

#define TAG "DEMO"

#define DEVICE_NAME "YoC DIS"
#define DEVICE_ADDR {0xCC,0x3B,0xE3,0x82,0x00,0xC0}

int16_t g_conn_hanlde = -1;
dis_handle_t g_dis_handle = NULL;

#define MANUFACTURER_NAME "PINGTOUGE"
#define MODEL_NUMBER "MODE_DEMO"
#define SERIAL_NUMBER "00000001"
#define HW_REV "0.0.1"
#define FW_REV "0.0.2"
#define SW_REV "0.0.3"

static dis_info_t dis_info = {
    MANUFACTURER_NAME,
    MODEL_NUMBER,
    SERIAL_NUMBER,
    HW_REV,
    FW_REV,
    SW_REV,
    NULL,
    NULL,
    NULL,
};

static void conn_change(ble_event_en event, void *event_data)
{
    evt_data_gap_conn_change_t *e = (evt_data_gap_conn_change_t *)event_data;

    if (e->connected == CONNECTED) {
        g_conn_hanlde = e->conn_handle;
        LOGI(TAG, "Connected");
        led_set_status(BLINK_SLOW);
    } else {
        g_conn_hanlde = -1;
        LOGI(TAG, "Disconnected");
        led_set_status(BLINK_FAST);
    }
}

static void conn_param_update(ble_event_en event, void *event_data)
{
    evt_data_gap_conn_param_update_t *e = event_data;

    LOGI(TAG, "LE conn param updated: int 0x%04x lat %d to %d\n", e->interval,
           e->latency, e->timeout);
}

static void mtu_exchange(ble_event_en event, void *event_data)
{
    evt_data_gatt_mtu_exchange_t *e = (evt_data_gatt_mtu_exchange_t *)event_data;

    if (e->err == 0) {
        LOGI(TAG, "mtu exchange, MTU %d", ble_stack_gatt_mtu_get(e->conn_handle));
    } else {
        LOGE(TAG, "mtu exchange fail, %d", e->err);
    }
}

static int event_callback(ble_event_en event, void *event_data)
{
    LOGD(TAG, "event %x", event);

    switch (event) {
        case EVENT_GAP_CONN_CHANGE:
            conn_change(event, event_data);
            break;
        case EVENT_GAP_CONN_PARAM_UPDATE:
            conn_param_update(event, event_data);
            break;
        case EVENT_GATT_MTU_EXCHANGE:
            mtu_exchange(event, event_data);
            break;

        default:
            LOGD(TAG, "Unhandle event %x", event);
            break;
    }

    return 0;
}

static ble_event_cb_t ble_cb = {
    .callback = event_callback,
};

int app_main(int argc, char *argv[])
{
    int ret;

    dev_addr_t addr = {DEV_ADDR_LE_RANDOM, DEVICE_ADDR};
    init_param_t init = {
        .dev_name = DEVICE_NAME,
        .dev_addr = &addr,
        .conn_num_max = 1,
    };
    ad_data_t ad[3] = {0};

    board_yoc_init();

    LOGI(TAG, "Bluetooth DIS demo!");

    ble_stack_init(&init);

    ble_stack_event_register(&ble_cb);

    g_dis_handle = dis_init(&dis_info);

    if (g_dis_handle == NULL) {
        LOGE(TAG, "DIS init FAIL!!!!");
        return -1;
    }

    uint8_t flag = AD_FLAG_GENERAL | AD_FLAG_NO_BREDR;
    ad[0].type = AD_DATA_TYPE_FLAGS;
    ad[0].data = (uint8_t *)&flag;
    ad[0].len = 1;

    uint8_t uuid16_list[] = {0x0a, 0x18}; /* UUID_DIS */
    ad[1].type = AD_DATA_TYPE_UUID16_ALL;
    ad[1].data = (uint8_t *)uuid16_list;
    ad[1].len = sizeof(uuid16_list);

    ad[2].type = AD_DATA_TYPE_NAME_COMPLETE;
    ad[2].data = (uint8_t *)DEVICE_NAME;
    ad[2].len = strlen(DEVICE_NAME);

    adv_param_t param = {
        ADV_IND,
        ad,
        NULL,
        BLE_ARRAY_NUM(ad),
        0,
        ADV_FAST_INT_MIN_1,
        ADV_FAST_INT_MAX_1,
    };

    ret = ble_stack_adv_start(&param);

    if (ret) {
        LOGE(TAG, "adv start fail %d!", ret);
    } else {
        LOGI(TAG, "adv start!");
        led_set_status(BLINK_FAST);
    }

    return 0;
}
