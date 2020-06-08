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
#include <app_init.h>

#define TAG "DEMO"
#define DEVICE_NAME "YoC_IBECON"

#define DEVICE_ADDR {0xCC,0x3B,0xE3,0x89,0xBA,0xC0}
//#define     E_UID           0X0
#define     E_URL           0X10
//#define     E_TLM           0X20

int app_main(int argc, char *argv[])
{
    int ret;
    dev_addr_t addr = {DEV_ADDR_LE_RANDOM, DEVICE_ADDR};
    init_param_t init = {
        .dev_name = NULL,
        .dev_addr = &addr,
        .conn_num_max = 0,
    };
    ad_data_t ad[3] = {0};
    ad_data_t sd[1] = {0};

    board_yoc_init();

    LOGI(TAG, "Bluetooth ibeacon demo!");

    ble_stack_init(&init);
    //ble_stack_event_register(NULL);

    uint8_t flag = AD_FLAG_NO_BREDR;
    ad[0].type = AD_DATA_TYPE_FLAGS;
    ad[0].data = (uint8_t *)&flag;
    ad[0].len = 1;

    uint8_t uuid16_list[] = {0xaa, 0xfe};
    ad[1].type = AD_DATA_TYPE_UUID16_ALL;
    ad[1].data = (uint8_t *)uuid16_list;
    ad[1].len = sizeof(uuid16_list);

#ifdef     E_UID
    //UID Data
    uint8_t url_list[] = {0xaa, 0xfe,/* Eddystone UUID */
                          E_UID, /* Frame Type*/
                          0x00, /* Tx Power at 0 m*/
                          0x0B, /*10 Bit NAME SPACE */
                          0xB8,
                          0x1b,
                          0x00,
                          0x00,
                          0x00,
                          0x00,
                          0x80,
                          0x00,
                          0x00,
                          0x80,/* 6 Bit Instance*/
                          0x00,
                          0x00,
                          0x00,
                          0x00,
                          0x00,
                          0x00,/*RFU MUST 0*/
                          0x00,
                         };
    ad[2].type = AD_DATA_TYPE_SVC_DATA16;
    ad[2].data = (uint8_t *)url_list;
    ad[2].len = sizeof(url_list);
#endif

#ifdef E_URL
    uint8_t url_list[] = {0xaa, 0xfe,/* Eddystone UUID */
                          E_URL,/* Eddystone-URL frame type */
                          0x00, /* Calibrated Tx power at 0m */
                          0x00, /* URL Scheme Prefix http://www. */
                          'c', 'o','p', '.', 'c', '-',
                          's', 'k', 'y',
                          0x07 /* .com */
                         };
    ad[2].type = AD_DATA_TYPE_SVC_DATA16;
    ad[2].data = (uint8_t *)url_list;
    ad[2].len = sizeof(url_list);
#endif

#ifdef E_TLM
    //TLM Data
    uint8_t info_list[] = {0xaa, 0xfe,/* Eddystone UUID */
                           E_TLM, /* Eddystone-TLM frame type */
                           0x00, /* TLM VerSion value=00 */
                           0x0B, /* VBATT[0],1mV/bit */
                           0xB8, /* VBATT[1],1mV/bit */
                           0x1b, /* beacon temperature */
                           0x00, /* beacon temperature */
                           0x00, /* adv PDU count */
                           0x00,
                           0x00,
                           0x80,
                           0x00,/* time since power-on reboot*/
                           0x00,
                           0x80,
                           0x00,
                          };
    ad[2].type = AD_DATA_TYPE_SVC_DATA16;
    ad[2].data = (uint8_t *)info_list;
    ad[2].len = sizeof(info_list);
#endif
    //SD DATA
    sd[0].type = AD_DATA_TYPE_NAME_COMPLETE;
    sd[0].data = (uint8_t *)DEVICE_NAME;
    sd[0].len = sizeof(DEVICE_NAME);

    adv_param_t param = {
        ADV_NONCONN_IND,
        ad,
        sd,
        BLE_ARRAY_NUM(ad),
        BLE_ARRAY_NUM(sd),
        ADV_FAST_INT_MIN_2,
        ADV_FAST_INT_MAX_2,
    };

    ret = ble_stack_adv_start(&param);

    if (ret) {
        LOGE(TAG, "ibeacon start fail %d!", ret);
    } else {
        LOGI(TAG, "ibeacon start!");
        led_set_status(BLINK_FAST);
    }

    return 0;
}
