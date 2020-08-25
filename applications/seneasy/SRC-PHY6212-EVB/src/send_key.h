/**
 * @file send_key.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2020-08-25
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef _SEND_KEY_H
#define _SEND_KEY_H

#include "drivers/keyscan.h"
#include "drivers/ir_nec.h"
#include "services/hid_service.h"

#define RCU_KEY_NUM     13
/*****************************************
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
 ****************************************/

typedef struct send_key
{
    // 键盘扫描的扫描码
    kscan_key_t vk;    
    // 对应要发送的HID按键键值
    press_key_data hid_key;
    // 对应要发送的IR键值
    uint8_t ir_key;
}rcu_key_map_t;

int rcu_send_hid_key_down(kscan_key_t vk);
int rcu_send_hid_key_release();
int rcu_send_ir_key_down(kscan_key_t vk);
int rcu_send_ir_key_release();

#endif