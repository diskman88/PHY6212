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
#include <yoc/cli.h>
#include <aos/cli.h>
#include <devices/devicelist.h>
#include <stdint.h>
#include <stdbool.h>

#include "services/hid_service.h"
#include "services/battary_service.h"
#include "services/dis_service.h"
#include "services/audio_service.h"

#include "board/ch6121_evb/msg.h"
#include "board/ch6121_evb/event.h"
#include "pm.h"

#define TAG  "DEMO"

#define DEVICE_NAME "remote control"
#define DEVICE_ADDR {0xE8,0x3B,0xE3,0x88,0xB1,0xC8}

int16_t g_conn_handle = -1;
static int16_t g_paired_hanlde = -1;
static dev_addr_t g_paired_addr;

// static aos_sem_t sync_sem;

aos_timer_t adv_timer = {0};

static void stop_adv(void *arg1, void *arg2)
{
    LOGI(TAG, "adv timer trigged");
    ble_stack_adv_stop();
    aos_timer_stop(&adv_timer);
}

static void start_adv(int ms)
{
    ad_data_t ad[4] = {0};
    uint8_t flag = AD_FLAG_GENERAL | AD_FLAG_NO_BREDR;
    ad[0].type = AD_DATA_TYPE_FLAGS;
    ad[0].data = (uint8_t *)&flag;
    ad[0].len = 1;

    uint8_t uuid16_list[] = {0x12, 0x18, 0x0f, 0x18}; /* UUID_BAS, UUID_HIDS */
    ad[1].type = AD_DATA_TYPE_UUID16_ALL;
    ad[1].data = (uint8_t *)uuid16_list;
    ad[1].len = sizeof(uuid16_list);

    ad[2].type = AD_DATA_TYPE_NAME_COMPLETE;
    ad[2].data = (uint8_t *)DEVICE_NAME;
    ad[2].len = strlen(DEVICE_NAME);

    ad[3].type = AD_DATA_TYPE_GAP_APPEARANCE;
    ad[3].data = (uint8_t *)(uint8_t[]) {
        0xc1, 0x03
    };
    ad[3].len = 2;

    adv_param_t param = {
        ADV_IND,
        ad,
        NULL,
        BLE_ARRAY_NUM(ad),
        0,
        ADV_FAST_INT_MIN_1,
        ADV_FAST_INT_MAX_1,
    };

    if (g_paired_hanlde != -1) {
        param.filter_policy = ADV_FILTER_POLICY_ALL_REQ;
    }

    int ret = ble_stack_adv_start(&param);

    if (ret) {
        LOGE(TAG, "adv start fail %d!", ret);
    } else {
        int s;
        if (adv_timer.hdl.cb == NULL) {
            s = aos_timer_new_ext(&adv_timer, stop_adv, NULL, ms, 0, 1);
        } else {
            s = aos_timer_change_without_repeat(&adv_timer, ms);
            s = aos_timer_start(&adv_timer);
        }

        if (s == 0) {
            LOGE(TAG, "adv start!, will stop after %d ms", ms);
        } else {
            LOGE(TAG, "adv start!, but stop timer can`t work:%d", s);
        }

        led_set_status(BLINK_FAST);
    }
}

static void gap_event_pairing_passkey_display(void *event_data)
{
    evt_data_smp_passkey_display_t *e = (evt_data_smp_passkey_display_t *)event_data;

    LOGI(TAG, "passkey is %s", e->passkey);
}

static void gap_event_smp_complete(void *event_data)
{
    evt_data_smp_pairing_complete_t *e = (evt_data_smp_pairing_complete_t *)event_data;

    if (e->err == 0) {
        g_paired_addr = e->peer_addr;
        ble_stack_white_list_add(&e->peer_addr);
        g_paired_hanlde = e->conn_handle;
    }

    LOGI(TAG, "pairing %s!!!", e->err ? "FAIL" : "SUCCESS");
}

static void gap_event_smp_cancel(void *event_data)
{
    LOGI(TAG, "pairing cancel");
}

static void gap_event_smp_pairing_confirm(evt_data_smp_pairing_confirm_t *e)
{
    LOGD(TAG, "Confirm pairing for");
}

static void gap_event_conn_security_change(void *event_data)
{
    evt_data_gap_security_change_t *e = (evt_data_gap_security_change_t *)event_data;
    LOGD(TAG, "conn %d security level change to level %d\n", e->conn_handle, e->level);
}

static void gap_event_conn_change(void *event_data)
{
    evt_data_gap_conn_change_t *e = (evt_data_gap_conn_change_t *)event_data;

    if (e->connected == CONNECTED && e->err == 0) {
        g_conn_handle = e->conn_handle;
        LOGI(TAG, "Connected");
        led_set_status(BLINK_SLOW);
        // 停止广播计时器
        aos_timer_stop(&adv_timer);

        ble_stack_security(g_conn_handle, 2);
    } else {
        g_conn_handle = -1;
        LOGI(TAG, "Disconnected err %d", e->err);
        // aos_sem_signal(&sync_sem);
        app_event_set(APP_EVENT_BT);
        led_set_status(BLINK_FAST);
        start_adv(30000);
    }
}

static void gap_event_conn_param_update(void *event_data)
{
    evt_data_gap_conn_param_update_t *e = event_data;

    LOGD(TAG, "LE conn param updated: int 0x%04x lat %d to %d\n", e->interval,
         e->latency, e->timeout);

}

static void gap_event_mtu_exchange(void *event_data)
{
    evt_data_gatt_mtu_exchange_t *e = (evt_data_gatt_mtu_exchange_t *)event_data;

    if (e->err == 0) {
        LOGI(TAG, "mtu exchange, MTU %d", ble_stack_gatt_mtu_get(e->conn_handle));
    } else {
        LOGE(TAG, "mtu exchange fail, %x", e->err);
    }
}

static int gap_event_callback(ble_event_en event, void *event_data)
{
    LOGD(TAG, "D event %x\n", event);

    switch (event) {
        case EVENT_GAP_CONN_CHANGE:
            gap_event_conn_change(event_data);
            break;

        case EVENT_GAP_CONN_PARAM_UPDATE:
            gap_event_conn_param_update(event_data);
            break;

        case EVENT_SMP_PASSKEY_DISPLAY:
            gap_event_pairing_passkey_display(event_data);
            break;

        case EVENT_SMP_PAIRING_COMPLETE:
            gap_event_smp_complete(event_data);
            break;

        case EVENT_SMP_PAIRING_CONFIRM:
            gap_event_smp_pairing_confirm(event_data);
            ble_stack_smp_passkey_confirm(g_conn_handle);
            break;

        case EVENT_SMP_CANCEL:
            gap_event_smp_cancel(event_data);
            break;

        case EVENT_GAP_CONN_SECURITY_CHANGE:
            gap_event_conn_security_change(event_data);
            break;

        case EVENT_GATT_MTU_EXCHANGE:
            gap_event_mtu_exchange(event_data);
            break;

        case EVENT_GAP_ADV_TIMEOUT:
            // start_adv();
            break;

        default:
            break;
    }

    return 0;
}

static ble_event_cb_t ble_cb = {
    .callback = gap_event_callback,
};


static void cmd_keysend_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    static press_key_data  send_data;
    if (argc == 2) {
        if (g_conn_handle != -1) {
            memset(&send_data, 0, sizeof(send_data));
            send_data.Code6 = atoi(argv[1]);
            key_board_send(&send_data);
            LOGD(TAG, "key send %d", atoi(argv[1]));

        }
    }
}

void cli_reg_cmd_keysend(void)
{
    static const struct cli_command cmd_info = {
        "keysend",
        "keysend commands",
        cmd_keysend_func,
    };

    aos_cli_register_command(&cmd_info);
}

void Light_Led(CTRL_LED usdata)
{
    if (usdata.bits.Cap_Lock) {
        LOGI(TAG, "Cap_Lock on\r\n");
    }

    if (usdata.bits.Num_Lock) {
        LOGI(TAG, "Num_Lock on\r\n");
    }

    if (usdata.bits.Scroll_Lock) {
        LOGI(TAG, "Scroll_Lock on\r\n");
    }

    if (usdata.bits.Compose) {
        LOGI(TAG, "Compose on\r\n");
    }

    if (usdata.bits.Kana) {
        LOGI(TAG, "Kana on\r\n");
    }
}

extern int create_keyscan_task();

int app_main(int argc, char *argv[])
{
    dev_addr_t addr = {DEV_ADDR_LE_RANDOM, DEVICE_ADDR};
    init_param_t init = {
        .dev_name = DEVICE_NAME,
        .dev_addr = &addr,
        .conn_num_max = 1,
    };

    uart_csky_register(0);

    spiflash_csky_register(0);

    board_yoc_init();

    cli_reg_cmd_keysend();

    LOGI(TAG, "Bluetooth HID demo!");

    ble_stack_init(&init);

    ble_stack_setting_load();

    ble_stack_event_register(&ble_cb);

    ble_stack_iocapability_set(IO_CAP_IN_YESNO | IO_CAP_OUT_NONE);

    hid_service_init();

    dis_service_init();

    battary_service_init();

    audio_service_init();

    create_keyscan_task();
    
    start_adv(30000);

    while (1) {
        uint32_t actl_flags;
        app_event_get(APP_EVENT_BT | APP_EVENT_IO | APP_EVENT_WAKEUP, &actl_flags, AOS_WAIT_FOREVER);
        // if (actl_flags & APP_EVENT_BT) {
        //     if (g_conn_handle == -1) {
        //         start_adv();
        //     }
        // }
        // if (actl_flags & APP_EVENT_IO) {
        //     io_msg_t msg;
        //     if (io_recv_message(&msg, 10) == 0) {
        //         LOGI(TAG, "msg:type=%d, event=%d", msg.type, msg.event);
        //     }

        // }
    }
    
    return 0;
}
