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
#include <aos/ble.h>
#include <aos/kernel.h>
#include <app_init.h>

#define TAG "DEMO"
#define DEVICE_NAME "YoC ADV"
#define DEVICE_ADDR {0xCC,0x3B,0xE3,0x82,0xBA,0xC0}

static aos_sem_t sync_sem;
static int connected = 0;
static int start_adv(void)
{
    int ret;
    ad_data_t ad[3] = {0};
    ad_data_t sd[1] = {0};

    uint8_t flag = AD_FLAG_GENERAL | AD_FLAG_NO_BREDR;

    ad[0].type = AD_DATA_TYPE_FLAGS;
    ad[0].data = (uint8_t *)&flag;
    ad[0].len = 1;

    uint8_t uuid16_list[] = {0x0d, 0x18};
    ad[1].type = AD_DATA_TYPE_UUID16_ALL;
    ad[1].data = (uint8_t *)uuid16_list;
    ad[1].len = sizeof(uuid16_list);

    ad[2].type = AD_DATA_TYPE_NAME_COMPLETE;
    ad[2].data = (uint8_t *)DEVICE_NAME;
    ad[2].len = strlen(DEVICE_NAME);

    uint8_t manu_data[10] = {0x01, 0x02, 0x3, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10};
    sd[0].type = AD_DATA_TYPE_MANUFACTURER_DATA;
    sd[0].data = (uint8_t *)manu_data;
    sd[0].len = sizeof(manu_data);

    adv_param_t param = {
        ADV_IND,
        ad,
        sd,
        BLE_ARRAY_NUM(ad),
        BLE_ARRAY_NUM(sd),
        ADV_FAST_INT_MIN_2,
        ADV_FAST_INT_MAX_2,
    };

    ret = ble_stack_adv_start(&param);

    if (ret) {
        LOGE(TAG, "adv start fail %d!", ret);
    } else {
        LOGI(TAG, "adv start!");
        led_set_status(BLINK_FAST);
    }

    LOGI(TAG, "adv_type:%x;adv_interval_min:%.3f ms;adv_interval_max:%.3f ms", param.type, param.interval_min * 0.625, param.interval_max * 0.625);
    return ret;
}

void conn_change(ble_event_en event, void *event_data)
{
    evt_data_gap_conn_change_t *e = (evt_data_gap_conn_change_t *)event_data;

    if (e->connected == CONNECTED) {
        LOGI(TAG, "Connected");
        connected = 1;
        led_set_status(BLINK_SLOW);
    } else {
        LOGI(TAG, "Disconnected");
        connected = 0;
        aos_sem_signal(&sync_sem);
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
    LOGI(TAG, "event %x", event);

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
            LOGW(TAG, "Unhandle event %x", event);
            break;
    }

    return 0;
}

static ble_event_cb_t ble_cb = {
    .callback = event_callback,
};

int app_main(void)
{
    dev_addr_t addr = {DEV_ADDR_LE_RANDOM, DEVICE_ADDR};
    init_param_t init = {
        .dev_name = DEVICE_NAME,
        .dev_addr = &addr,
        .conn_num_max = 1,
    };
    connected = 0;

    board_yoc_init();
    aos_sem_new(&sync_sem, 1);

    LOGI(TAG, "Bluetooth advertising demo!");

    ble_stack_init(&init);
    ble_stack_event_register(&ble_cb);

    while (1) {
        aos_sem_wait(&sync_sem, AOS_WAIT_FOREVER);

        if (!connected) {
            start_adv();
        }
    }

    return 0;
}
