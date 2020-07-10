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

typedef struct 
{
    kscan_key_t vk;
    press_key_data kb;
}vk_keyboard_map_t;

const vk_keyboard_map_t vk_to_keyboard_map[10 + 1]= {
    [0] = {
        .vk = VK_KEY_1,
        .kb = {.keydata = {{0}}, .Rsv=0, .Code1=0x1E, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
    }, 
    [1] = {
        .vk = VK_KEY_2,
        .kb = {.keydata = {{0}}, .Rsv=0, .Code1=0x1F, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
    },
    [2] = {
        .vk = VK_KEY_3,
        .kb = {.keydata = {{0}}, .Rsv=0, .Code1=0x20, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
    },
    [3] = {
        .vk = VK_KEY_4,
        .kb = {.keydata = {{0}}, .Rsv=0, .Code1=0x21, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
    },
    [4] = {
        .vk = VK_KEY_5,
        .kb = {.keydata = {{0}}, .Rsv=0, .Code1=0x22, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
    },
    [5] = {
        .vk = VK_KEY_6,
        .kb = {.keydata = {{0}}, .Rsv=0, .Code1=0x23, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
    },
    [6] = {
        .vk = VK_KEY_7,
        .kb = {.keydata = {{0}}, .Rsv=0, .Code1=0x24, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
    },
    [7] = {
        .vk = VK_KEY_8,
        .kb = {.keydata = {{0}}, .Rsv=0, .Code1=0x25, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
    },
    [8] = {
        .vk = VK_KEY_9,
        .kb = {.keydata = {{0}}, .Rsv=0, .Code1=0x26, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
    },
    [9] = {
        .vk = VK_KEY_10,
        .kb = {.keydata = {{0}}, .Rsv=0, .Code1=0x27, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
    },
    [10] = {
        .vk = 0,
        .kb = {.keydata = {{0}}, .Rsv=0, .Code1=0x00, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 },
    }
};

void rcu_vk_kb_handle(kscan_key_t vk, int16_t state)
{
    press_key_data kb_input = {.keydata = {{0}}, .Rsv=0, .Code1=0x00, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 };
    int i = 0; 
    if (state == IO_MSG_KEYSCAN_KEY_DOWN) {
        do {
            if (vk == vk_to_keyboard_map[i].vk) {
                kb_input = vk_to_keyboard_map[i].kb;
                key_board_send(&kb_input);
                break;
            }
            i++;
        }while(vk_to_keyboard_map[i].vk != 0);
    } else {
        key_board_send(&kb_input);
    }
}

int app_main(int argc, char *argv[])
{
    uart_csky_register(0);

    spiflash_csky_register(0);

    board_yoc_init();

    // 构建消息队列
    app_init_io_message();

    // 按键扫描在独立的线程运行
    create_keyscan_task();

    // 启动蓝牙
    rcu_ble_init();

#ifdef __DEBUG__
    cli_reg_cmd_keysend();
#endif
   
    while (1) {
        uint32_t actl_flags;
        app_event_get(APP_EVENT_BT | APP_EVENT_IO | APP_EVENT_PM, &actl_flags, AOS_WAIT_FOREVER);
        LOGI("APP", ">>:APP new event = %x:", actl_flags);

        if (actl_flags & APP_EVENT_IO) {
            io_msg_t msg;
            if (app_recv_io_message(&msg, 20)) {
                switch (msg.type)
                {
                    case IO_MSG_KEYSCAN:
                        // 1.hid keyboad 按键处理
                        rcu_vk_kb_handle(msg.param, msg.subtype);
                        LOGI("APP", "handle key = %d", msg.param);
                        // 2.GAP配对功能
                        if (msg.subtype == IO_MSG_KEYSCAN_KEY_DOWN && msg.param == VK_FUNC_6) {
                            rcu_ble_pairing();
                        }
                        break;
                    case IO_MSG_VOICE:
                        break;
                    default:
                        break;
                }
            }
        }
    }
    
    return 0;
}
