/**
 * @file send_key.c
 * @author zhangkaihua
 * @brief 
 * @version 1.0
 * @date 2020-08-25
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "send_key.h"

const rcu_key_map_t rcu_key_map_table[RCU_KEY_NUM] = {
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

static bool get_hid_key(kscan_key_t vk, press_key_data *match_key)
{
    for(int i = 0; i < RCU_KEY_NUM; i++) {
        if (vk == rcu_key_map_table[i].vk) {
            *match_key = rcu_key_map_table[i].hid_key;
            return true;
        }
    }
    return false;
}

static bool get_ir_key(kscan_key_t vk, uint8_t *match_key) 
{
    for(int i = 0; i < RCU_KEY_NUM; i++) {
        if (vk == rcu_key_map_table[i].vk) {
            *match_key = rcu_key_map_table[i].ir_key;
            return true;
        }        
    }
    return false;
}

/**
 * @brief 
 * 
 * @param vk 
 * @return int 
 */
int rcu_send_hid_key_down(kscan_key_t vk)
{
    int ret;
    press_key_data hid_key;
    if (get_hid_key(vk, &hid_key)) {
        ret = hid_key_board_send(&hid_key);
        return ret;
    } else {
        LOGE("KEY", "hid key not found");
        return -1;
    }
}

int rcu_send_hid_key_release()
{
    int ret;
    press_key_data hid_key = {0};
    ret = hid_key_board_send(&hid_key);
    return ret;
}

int rcu_send_ir_key_down(kscan_key_t vk) 
{
    int ret;
    uint8_t ir_key;
    if (get_ir_key(vk, &ir_key)) {
        ret = ir_nec_start_send(0x40, ir_key);
        return ret; 
    } else {
        LOGE("KEY", "ir key not found");
        return -1;
    }
}

int rcu_send_ir_key_release()
{
    return ir_nec_stop_send();
}