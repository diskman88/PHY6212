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
#include <yoc/partition.h>
#include <yoc/ota_server.h>

#define TAG "DEMO"
#define DEVICE_NAME "YoC OTA"
#define DEVICE_ADDR                                                                                \
    {                                                                                              \
        0xCC, 0x3B, 0x13, 0x88, 0xBA, 0xC0                                                         \
    }

static aos_sem_t sync_sem;

static int start_adv()
{
    ad_data_t ad[2] = {0};

    uint8_t flag = AD_FLAG_GENERAL | AD_FLAG_NO_BREDR;
    ad[0].type   = AD_DATA_TYPE_FLAGS;
    ad[0].data   = (uint8_t *)&flag;
    ad[0].len    = 1;

    ad[1].type = AD_DATA_TYPE_NAME_COMPLETE;
    ad[1].data = (uint8_t *)DEVICE_NAME;
    ad[1].len  = strlen(DEVICE_NAME);

    adv_param_t param = {
        ADV_IND, ad, NULL, BLE_ARRAY_NUM(ad), 0, ADV_FAST_INT_MIN_1, ADV_FAST_INT_MAX_1,
    };

    int ret = ble_stack_adv_start(&param);

    if (ret) {
        LOGE(TAG, "adv start fail %d!", ret);
    } else {
        LOGI(TAG, "adv start!");
        led_set_status(BLINK_FAST);
    }

    return 0;
}

void conn_change(ble_event_en event, void *event_data)
{
    evt_data_gap_conn_change_t *e = (evt_data_gap_conn_change_t *)event_data;

    if (e->connected == CONNECTED) {
        LOGI(TAG, "Connected");
        led_set_status(BLINK_SLOW);
    } else {
        LOGI(TAG, "Disconnected");
        led_set_status(BLINK_FAST);
        start_adv();
    }
}

static int event_callback(ble_event_en event, void *event_data)
{
    switch (event) {
        case EVENT_GAP_CONN_CHANGE:
            conn_change(event, event_data);
            break;

        default:
            LOGD(TAG, "Unhandle event %d\n", event);
            break;
    }

    return 0;
}

static ble_event_cb_t ble_cb = {
    .callback = event_callback,
};

static void ota_event(ota_state_en ota_state)
{
    aos_sem_signal(&sync_sem);
}

int app_main(int argc, char *argv[])
{
    dev_addr_t   addr = {DEV_ADDR_LE_RANDOM, DEVICE_ADDR};
    init_param_t init = {
        .dev_name     = DEVICE_NAME,
        .dev_addr     = &addr,
        .conn_num_max = 1,
    };

    board_yoc_init();

    LOGI(TAG, "bluetooth YoC OTA demo!");

    ble_stack_init(&init);

    ble_ota_init(ota_event);

    ble_stack_setting_load();

    ble_stack_event_register(&ble_cb);

    start_adv();

    aos_sem_new(&sync_sem, 0);

    while (1) {
        aos_sem_wait(&sync_sem, AOS_WAIT_FOREVER);
        ble_ota_process();
    }

    return 0;
}
