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
#include "drivers/ir_nec.h"

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

/**
 * @brief 
 * | 键名  | code | IR | HID |
 * | :--- | :---: | :---: | :---: |
 * | Power| S1  | 0x12 | 0x0066 |
 * | Mute | S8  | 0x10 | 0x007F |
 * | Home | S13 | 0x51 | 0x004A |
 * | Menu | S9  | 0x5B | 0x0076 |
 * | Vol- | S12 | 0x1E | 0x0081 |
 * | Vol+ | S11 | 0x1A | 0x0080 |
 * | Up   | S6  | 0x19 | 0x0052 |
 * | Down | S7  | 0x1D | 0x0051 |
 * | Left | S5  | 0x46 | 0x0050 |
 * | Right| S2  | 0x47 | 0x004f |
 * | OK   | S3  | 0x0A | 0x0058 |
 * | Back | S10 | 0x40 | 0x0029 |
 * | Voice| S4  | 0x61 | 0x0075 |
 */
#define RCU_KEY_NUM     13
const struct { kscan_key_t vk; press_key_data hid_key; uint8_t ir_key;} vk_map_table[RCU_KEY_NUM] = {
    [0] = {
        .vk = VK_KEY_01,    // POWER
        .hid_key = {.keydata = {{0}}, .Rsv=0, .Code1=0x66, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        .ir_key = 0x12,
    }, 
    [1] = {
        .vk = VK_KEY_08,    // MUTE
        .hid_key = {.keydata = {{0}}, .Rsv=0, .Code1=0x7F, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        .ir_key = 0x10,
    },
    [2] = {
        .vk = VK_KEY_13,    // HOME
        .hid_key = {.keydata = {{0}}, .Rsv=0, .Code1=0x4A, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        .ir_key = 0x51,
    },
    [3] = {
        .vk = VK_KEY_09,    // MENU
        .hid_key = {.keydata = {{0}}, .Rsv=0, .Code1=0x76, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        .ir_key = 0x5B,
    },
    [4] = {
        .vk = VK_KEY_12,    // VOL-
        .hid_key = {.keydata = {{0}}, .Rsv=0, .Code1=0x1E, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        .ir_key = 0x1E,
    },
    [5] = {
        .vk = VK_KEY_11,    // VOL+
        .hid_key = {.keydata = {{0}}, .Rsv=0, .Code1=0x1A, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        .ir_key = 0x1A,
    },
    [6] = {
        .vk = VK_KEY_06,    // UP
        .hid_key = {.keydata = {{0}}, .Rsv=0, .Code1=0x52, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        .ir_key = 0x19,
    },
    [7] = {
        .vk = VK_KEY_07,    // DOWN
        .hid_key = {.keydata = {{0}}, .Rsv=0, .Code1=0x51, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        .ir_key = 0x1D,
    },
    [8] = {
        .vk = VK_KEY_05,    // LEFT
        .hid_key = {.keydata = {{0}}, .Rsv=0, .Code1=0x50, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        .ir_key = 0x46,
    },
    [9] = {
        .vk = VK_KEY_02,    // RIGHT
        .hid_key = {.keydata = {{0}}, .Rsv=0, .Code1=0x4F, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        .ir_key = 0x47,
    },
    [10] = {
        .vk = VK_KEY_03,    // OK
        .hid_key = {.keydata = {{0}}, .Rsv=0, .Code1=0x58, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        .ir_key = 0x0A,
    },
    [11] = {
        .vk = VK_KEY_10,    // BACK
        .hid_key = {.keydata = {{0}}, .Rsv=0, .Code1=0x29, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        .ir_key = 0x40,
    },
    [12] = {
        .vk = VK_KEY_04,    // VOICE
        .hid_key = {.keydata = {{0}}, .Rsv=0, .Code1=0x75, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
        .ir_key = 0x61,
    },        
};

press_key_data vk_to_hid_key(kscan_key_t vk)
{
    press_key_data key = {.keydata = {{0}}, .Rsv=0, .Code1=0, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 };
    for(int i = 0; i < RCU_KEY_NUM; i++) {
        if (vk == vk_map_table[i].vk) {
            key = vk_map_table[i].hid_key;
            break;
        }
    }
    return key;
}

uint8_t vk_to_ir_key(kscan_key_t vk) 
{
    uint8_t key = 0;
    for(int i = 0; i < RCU_KEY_NUM; i++) {
        if (vk == vk_map_table[i].vk) {
            key = vk_map_table[i].ir_key;
            break;
        }        
    }
    return key;
}


void rcu_send_hid_kb(kscan_key_t vk)
{


}

/**
 * @brief 处理所有按键(VK)消息
 * 
 * @param vk， 按下的按键
 * @param state ，KSCAN_KEY_PRESSED/KSCAN_KEY_RELEASE
 */
void rcu_vk_handle(kscan_key_t vk, int16_t state)
{
    // 有按键按下:
    if (state == MSG_KEYSCAN_KEY_PRESSED) {
        // 语音键处理
        // 实体键处理
        if (vk < VK_KEY_FUNC) {
            // 1.hid keyboad 按键处理
            if (g_gap_data.state == GAP_STATE_PAIRED || g_gap_data.state == GAP_STATE_CONNECTED) {
                press_key_data hid_key = vk_to_hid_key(vk);
                hid_key_board_send(&hid_key);
            } else {
                uint8_t ir_key = vk_to_ir_key(vk);
                LOGI("APP", "Send IR CODE %x", ir_key);
                ir_nec_start_send(0x40, ir_key);
            }
        } 
        // 功能键处理
        else {
            switch (vk) {
                // 配对按键处理
                case VK_KEY_FUNC1:
                    rcu_ble_pairing();
                    break;
                default:    
                    break;
            }
        }
    }
    // 按键松开:
    else {
        if (g_gap_data.state == GAP_STATE_PAIRED || g_gap_data.state == GAP_STATE_CONNECTED) {
            press_key_data hid_key = {0};
            hid_key_board_send(&hid_key);
        } else {
            ir_nec_stop_send();
        }
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
    }
    
    return 0;
}
