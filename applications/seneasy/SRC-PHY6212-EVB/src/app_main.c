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

#include "app_msg.h"
#include "gap.h"

#include "drivers/keyscan.h"
#include "drivers/voice/voice_driver.h"

#include "services/hid_service.h"

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

// typedef struct 
// {
//     kscan_key_t vk;
//     press_key_data kb;
// }vk_keyboard_map_t;

// const vk_keyboard_map_t vk_to_keyboard_map[10 + 1]= {

void rcu_send_hid_kb(kscan_key_t vk)
{
    const struct { kscan_key_t vk; press_key_data kb;} vk_to_keyboard_map[10+1] = {
        [0] = {
            .vk = VK_KEY_01,
            .kb = {.keydata = {{0}}, .Rsv=0, .Code1=0x1E, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        }, 
        [1] = {
            .vk = VK_KEY_02,
            .kb = {.keydata = {{0}}, .Rsv=0, .Code1=0x1F, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        },
        [2] = {
            .vk = VK_KEY_03,
            .kb = {.keydata = {{0}}, .Rsv=0, .Code1=0x20, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        },
        [3] = {
            .vk = VK_KEY_04,
            .kb = {.keydata = {{0}}, .Rsv=0, .Code1=0x21, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        },
        [4] = {
            .vk = VK_KEY_05,
            .kb = {.keydata = {{0}}, .Rsv=0, .Code1=0x22, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        },
        [5] = {
            .vk = VK_KEY_06,
            .kb = {.keydata = {{0}}, .Rsv=0, .Code1=0x23, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        },
        [6] = {
            .vk = VK_KEY_07,
            .kb = {.keydata = {{0}}, .Rsv=0, .Code1=0x24, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        },
        [7] = {
            .vk = VK_KEY_08,
            .kb = {.keydata = {{0}}, .Rsv=0, .Code1=0x25, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        },
        [8] = {
            .vk = VK_KEY_09,
            .kb = {.keydata = {{0}}, .Rsv=0, .Code1=0x26, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        },
        [9] = {
            .vk = VK_KEY_10,
            .kb = {.keydata = {{0}}, .Rsv=0, .Code1=0x27, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        },
        // VK=0时，释放按键
        [10] = {
            .vk = 0,
            .kb = {.keydata = {{0}}, .Rsv=0, .Code1=0x00, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        }
    };
    int i = 0; 
    do {
        if (vk == vk_to_keyboard_map[i].vk) {
            press_key_data hid_keys = vk_to_keyboard_map[i].kb;
            key_board_send(&hid_keys);
            break;
        }
    }while(vk_to_keyboard_map[i++].vk != 0);
}

/**
 * @brief 处理所有按键(VK)消息
 * 
 * @param vk， 按下的按键
 * @param state ，KSCAN_KEY_PRESSED/KSCAN_KEY_RELEASE
 */
void rcu_vk_handle(kscan_key_t vk, int16_t state)
{
    if (state == MSG_KEYSCAN_KEY_PRESSED) {
        // 实体键处理
        if (vk < VK_KEY_FUNC) {
            // 1.hid keyboad 按键处理
            rcu_send_hid_kb(vk);
        } 
        // 功能键处理
        else {
            switch (vk) {
                // 配对按键处理
                case VK_KEY_FUNC6:
                    rcu_ble_pairing();
                    break;
                default:    
                    break;
            }
        }
    } else {
        rcu_send_hid_kb(0);
    }
}


/**
 * @brief 系统消息处理接口
 * 
 * @param msg 
 */
void app_message_handle(app_msg_t *msg)
{
    switch(msg->type) {
        case MSG_KEYSCAN:
            LOGI("APP", "io message: %s = %2x\n", 
                                    (msg->subtype == MSG_KEYSCAN_KEY_PRESSED) ? "KEY DOWN":"KEY RELEASE", msg->param);                       
            rcu_vk_handle(msg->param, msg->subtype);
            break;
        case MSG_BT_GAP:
            break;
        case MSG_BT_GATT:

            if (msg->subtype == MSG_BT_GATT_ATV_MIC_OPEN) {
                if(g_gap_data.p_atvv->char_rx_cccd & CCC_VALUE_NOTIFY) {
                    voice_handle_mic_start();
                } else {
                    LOGI(TAG, "voice service not enable");
                }
            }

            if (msg->subtype == MSG_BT_GATT_ATV_MIC_STOP) {
                voice_handle_mic_stop();
            }
            break;
        default:
            break;
    }
}

int app_main(int argc, char *argv[])
{
    uart_csky_register(0);

    spiflash_csky_register(0);

    board_yoc_init();

    // 构建消息队列
    app_init_message();

    // 按键扫描在独立的线程运行
    create_keyscan_task();

    // 启动蓝牙
    rcu_ble_init();

#ifdef __DEBUG__
    cli_reg_cmd_keysend();
#endif
    while (1) {
        uint32_t actl_flags;
        app_event_get(APP_EVENT_MSG | APP_EVENT_VOICE, &actl_flags, AOS_WAIT_FOREVER);

        /**
         * @brief 处理系统消息：消息由各模块产生
         */
        if (actl_flags & APP_EVENT_MSG) {
            app_msg_t msg;
            if (app_recv_message(&msg, 0)) {
                // 处理消息
                app_message_handle(&msg);
                // 消息队列里还有未处理的消息
                if (app_is_message_empty() == false) {
                    app_event_set(APP_EVENT_MSG);
                }
            }
        }

        /**
         * @brief 语音数据事件：microphone启动后，由ADC DMA中断触发
         */
        if (actl_flags & APP_EVENT_VOICE) {
            voice_trans_frame_t frame;
            bool first = true;
            uint16_t start = 0;
            uint16_t lost = 0;

            int err = 0;
            int retry = 0;
            while (voice_fifo_top(&frame)) {    // 获取FIFO头
                if (first) {
                    first = false;
                    start = frame.seq_id;
                }
                // 调用service发送数据:
                //    如果发送失败(数据缓冲区满)，退出本次发送
                //    如果发送成功，队列弹出一位
                if (atvv_voice_send((uint8_t *)&frame, sizeof(voice_trans_frame_t))) {
                    lost++;
                    break;
                } else {   // 发送成功，队列出列
                    voice_fifo_pop();
                }

                // 最多连续发送20位
                retry++;
                if (retry > 20) break;

                // retry = 0;
                // do {
                //     err = atvv_voice_send((uint8_t *)&frame, sizeof(voice_trans_frame_t));
                //     if (err != 0) {
                //         break;
                //         aos_msleep(2);
                //         retry++;
                //         if(retry > 10) {
                //             LOGE("APP", "ble send voice data unsuccessed, data may be lost:0x%x", err);
                //             break;
                //         }                        
                //     }
                // }while (err);
            }
            LOGI("APP", "voice trans start=%d， end=%d， lost=%d", start, frame.seq_id, lost);
        }
    }
    
    return 0;
}
