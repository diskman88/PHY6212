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
#include "pm.h"

// 驱动和service
#include "drivers/led.h"
#include "drivers/keyscan.h"
#include "drivers/voice/voice_driver.h"
#include "services/hid_service.h"
// 系统接口
#include "app_msg.h"
#include "gap.h"
#include "send_key.h"

#define TAG  "APP"

#ifdef __DEBUG__
static void cmd_keysend_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    static press_key_data  send_data;
    if (argc == 2) {
        if (g_gap_data.conn_handle != -1) {
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
#endif

/**
 * @brief 处理所有按键(VK)消息
 * 
 * @param vk， 按下的按键
 * @param state ，KSCAN_KEY_PRESSED/KSCAN_KEY_RELEASE
 */
void on_msg_keyscan(kscan_key_t vk, int16_t state)
{
    /**
     * @brief 设备连接状态判断
     * ble连接已连接：mode = 1；
     *  1.按键发送HID键码
     * ble未连接： mode = 0
     *  1.按键发送IR键码
     */
    uint8_t mode = 0;
    if (g_gap_data.state == GAP_STATE_PAIRED || g_gap_data.state == GAP_STATE_CONNECTED) { 
        mode = 1;
    }

    // 1.功能键按下处理
    if (vk >= VK_KEY_FUNC && state == MSG_KEYSCAN_KEY_PRESSED) {
        led_on();
        // 组合键(10+3):启动广播
        if (vk == VK_KEY_FUNC1) {
            rcu_ble_pairing();
        }
    } 
    // 2.发码键按下处理
    else if (vk < VK_KEY_FUNC && state == MSG_KEYSCAN_KEY_PRESSED) {
        if (mode == 1) {
            rcu_send_hid_key_down(vk);
        } else {
            rcu_send_ir_key_down(vk);
        }
    }
    // 3.其他状态
    else {
        LOGE(TAG, "vk=%d, state=%d", vk, state);
        if (mode == 1) {
            rcu_send_hid_key_release();
        }
        rcu_send_ir_key_release();      // 按键释放时停止ir发送，防止状态切换后ir无法停止
    }
}

void event_handle_voice()
{
    voice_trans_frame_t frame;
    bool first = true;
    uint16_t start = 0;
    int err = 0;
    int retry = 0;
    while (voice_fifo_top(&frame)) {    // 指向FIFO头部
        if (first) {
            first = false;
            start = frame.seq_id;
        }
        // 调用service发送数据:
        //    如果发送失败(数据缓冲区满)，退出本次发送
        //    如果发送成功，队列弹出一位
        if (atvv_voice_send((uint8_t *)&frame, sizeof(voice_trans_frame_t))) {
            err++;
            break;
        } else {   
            // 发送成功，弹出头部元素
            voice_fifo_pop();
        }

        // 继续发送，最多连续发送20帧
        retry++;
        if (retry > 20) break;
    }
    LOGI("APP", "voice trans start=%d， end=%d， lost=%d", start, frame.seq_id, err);
}

/**
 * @brief 系统消息处理接口
 * 
 * @param msg 
 */
void event_handle_message()
{
    app_msg_t msg;
    if (app_recv_message(&msg, 0) == false) {
        return;
    }

    switch(msg.type) {
        case MSG_KEYSCAN:
            LOGI("APP", "io message: %s = %2x\n", 
                                    (msg.subtype == MSG_KEYSCAN_KEY_PRESSED) ? "KEY DOWN":"KEY RELEASE", msg.param);                       
            on_msg_keyscan(msg.param, msg.subtype);
            break;
        case MSG_BT_GAP:
            break;
        case MSG_BT_GATT:
            if (msg.subtype == MSG_BT_GATT_ATV_MIC_OPEN) {
                if(g_gap_data.p_atvv->char_rx_cccd & CCC_VALUE_NOTIFY) {
                    voice_handle_mic_start();
                } else {
                    LOGI(TAG, "voice service not enable");
                }
            }
            if (msg.subtype == MSG_BT_GATT_ATV_MIC_STOP) {
                voice_handle_mic_stop();
            }
            break;
        default:
            break;
    }
    // 消息队列里还有未处理的消息
    if (app_is_message_empty() == false) {
        app_event_set(APP_EVENT_MSG);
    }
}

int app_main(int argc, char *argv[])
{
    uart_csky_register(0);

    spiflash_csky_register(0);

    board_yoc_init();

    // 构建消息队列
    app_init_message();
    // ir_nec_start_send(0x55, 0x01);
    // 按键扫描在独立的线程运行
    create_keyscan_task();
    // ir_nec_start_send(0x55, 0x31);
    // 启动蓝牙
    rcu_ble_init();

#ifdef __DEBUG__
    cli_reg_cmd_keysend();
#endif

    while (1) {
        uint32_t actl_flags;
        app_event_get(APP_EVENT_MASK, &actl_flags, AOS_WAIT_FOREVER);

        /**
         * @brief 处理系统消息：消息由各模块产生
         */
        if (actl_flags & APP_EVENT_MSG) {
            event_handle_message();
        }

        /**
         * @brief 语音数据事件：microphone启动后，由ADC DMA中断触发
         */
        if (actl_flags & APP_EVENT_VOICE) {
            event_handle_voice();
        }

        /**
         * @brief OTA事件：主机通过ota service发送OTA命令时触发
         */
        if (actl_flags & APP_EVENT_OTA) {
            ble_ota_process();
        }

    }
    
    return 0;
}
