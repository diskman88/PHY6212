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
#include <pwrmgr.h>

// 驱动和service
#include "drivers/leds.h"
#include "drivers/keyscan.h"
#include "drivers/voice/voice_driver.h"
// #include "services/hid_service.h"
// 系统接口
#include "app_msg.h"
#include "profiles/gap.h"
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
void on_msg_key(kscan_key_t vk, int16_t state)
{
    // 0.任意键按下
    if (state != MSG_KEYSCAN_KEY_RELEASE_ALL || state != MSG_KEYSCAN_KEY_RELEASE_ALL) {
        if (g_gap_data.state == GAP_STATE_IDLE) {   // 处于未连接,未广播状态
            if (g_gap_data.bond.is_bonded) {    // 设备已绑定主机
                rcu_ble_start_adversting(ADV_START_RECONNECT);
            } else {
                if (vk == VK_KEY_01) {  // 只有电源键发起非直连广播
                    rcu_ble_start_adversting(ADV_START_POWER_KEY);
                }
            }
        }
    }
    // 1.功能键(组合键:HOME[S3]+MENU[S11]):启动广播
    if (vk == VK_KEY_FUNC1 && state == MSG_KEYSCAN_KEY_HOLD) {
        rcu_ble_start_adversting(ADV_START_PAIRING_KEY);
    }
    // 2.功能键(组合键:OK[S6]+MENU[S11]):清除配对
    if (vk == VK_KEY_FUNC2 && state == MSG_KEYSCAN_KEY_HOLD) {
        rcu_ble_clear_pairing();
    } 
    
    // 发码键处理
    if (g_gap_data.state == GAP_STATE_PAIRED || g_gap_data.state == GAP_STATE_CONNECTED) {
        if (state == MSG_KEYSCAN_KEY_PRESSED) {
            int err = 0;
            // err = rcu_send_hid_key_down(vk);    // ble连接已连接：按键发送HID键码
            err = rcu_send_key_down(vk);
            if (err == 0) {
                rcu_led_red_on();
            }
        } else {
            // rcu_send_hid_key_release();
            rcu_send_key_release(vk);
            rcu_led_red_off();
        }
    }
}

void event_handler_voice()
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
const char *key_state_str[5] = {
    "PRESSED",
    "HOLD",
    "COMBINE",
    "RELEASE",
    "RELEASE_ALL"
};
void event_handler_io_message()
{
    app_msg_t msg;
    if (app_recv_message(&msg, 0) == false) {
        return;
    }
    switch(msg.type) {
        case MSG_KEYSCAN:
            LOGI("APP", "key_%02d: %s", msg.param, key_state_str[msg.subtype]);                       
            on_msg_key(msg.param, msg.subtype);
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

    rcu_led_init();
    
    // 构建消息队列
    app_init_message();
    // ir_nec_start_send(0x55, 0x01);
    // 按键扫描在独立的线程运行
    create_keyscan_task();
    // ir_nec_start_send(0x55, 0x31);
    // 启动蓝牙
    rcu_ble_init();

    // disableSleepInPM(0x01);

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
            event_handler_io_message();
        }

        /**
         * @brief 语音数据事件：microphone启动后，由ADC DMA中断触发
         */
        if (actl_flags & APP_EVENT_VOICE) {
            event_handler_voice();
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


/***********************************************************************************
 * 睡眠处理
 **********************************************************************************/
#include "drv_usart.h"
extern void registers_save(uint32_t *mem, uint32_t *addr, int size);
static uint32_t usart_regs_saved[5];
static void usart_prepare_sleep_action(void)
{
    uint32_t addr = 0x40004000;
    uint32_t read_num = 0;
    // 清空 Receive FIFO
    while (*(volatile uint32_t *)(addr + 0x14) & 0x1) {
        *(volatile uint32_t *)addr;
        if (read_num++ >= 16) {
            break;
        }
    }
    // 等待串口空闲
    while (*(volatile uint32_t *)(addr + 0x7c) & 0x1);
    // !!! PHY6212 串口的发送寄存器,接收寄存器, 中断寄存器,分频寄存器地址复用,需要特殊读写序列..
    *(volatile uint32_t *)(addr + 0xc) |= 0x80;     // 允许读写波特率分频寄存器
    registers_save((uint32_t *)usart_regs_saved, (uint32_t *)addr, 2);      // 保存波特率分频寄存器
    *(volatile uint32_t *)(addr + 0xc) &= ~0x80;        // 禁止读写波特率分频寄存器(地址映射为读写和中断寄存器)
    // 保存中断使能
    registers_save(&usart_regs_saved[2], (uint32_t *)addr + 1, 1);
    registers_save(&usart_regs_saved[3], (uint32_t *)addr + 3, 2);
}

static void usart_wakeup_action(void)
{
    drv_pinmux_config(P9, UART_TX);
    drv_pinmux_config(P10, UART_RX);
    uint32_t addr = 0x40004000;
    while (*(volatile uint32_t *)(addr + 0x7c) & 0x1);
    *(volatile uint32_t *)(addr + 0xc) |= 0x80;
    registers_save((uint32_t *)addr, usart_regs_saved, 2);
    *(volatile uint32_t *)(addr + 0xc) &= ~0x80;
    registers_save((uint32_t *)addr + 1, &usart_regs_saved[2], 1);
    registers_save((uint32_t *)addr + 3, &usart_regs_saved[3], 2);
}

/**
 * @brief 系统进入低功耗入口
 * 
 * @return int 
 */
int pm_prepare_sleep_action()
{
    hal_ret_sram_enable(RET_SRAM0 | RET_SRAM1 | RET_SRAM2);
    // LOGI("PM", "prepare to sleep");

    kscan_prepare_sleep_action();
    csi_usart_prepare_sleep_action(0);
    // usart_prepare_sleep_action();
    // csi_pinmux_prepare_sleep_action();
    return 0;
}

/**
 * @brief 系统退出低功耗入口
 * 
 * @return int 
 */
int pm_after_sleep_action()
{
    // csi_pinmux_wakeup_sleep_action();
    kscan_wakeup_action();
    drv_pinmux_config(P9, UART_TX);
    drv_pinmux_config(P10, UART_RX);
    csi_usart_wakeup_sleep_action(0);
    // usart_wakeup_action();

    // LOGI("PM", "wakeup");
    return 0;
}