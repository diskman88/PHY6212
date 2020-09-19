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


// keyboard keys
const hid_keybaord_report_t kb_release_all = {.modifier = {{0}}, .Rsv=0, .Code1=0, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 };
const hid_keybaord_report_t kb_up    = {.modifier = {{0}}, .Rsv=0, .Code1=0x52, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 };
const hid_keybaord_report_t kb_down  = {.modifier = {{0}}, .Rsv=0, .Code1=0x51, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 };
const hid_keybaord_report_t kb_left  = {.modifier = {{0}}, .Rsv=0, .Code1=0x50, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 };
const hid_keybaord_report_t kb_right = {.modifier = {{0}}, .Rsv=0, .Code1=0x4F, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 };
const hid_keybaord_report_t kb_menu  = {.modifier = {{0}}, .Rsv=0, .Code1=0x65, .Code2=0, .Code3=0, .Code4=0, .Code5=0, .Code6=0 };
// consumer keys
const hid_consumer_report_t mm_release_all = {.data = 0};

const hid_consumer_report_t mm_power     = {.button = {.VOL_DOWN = 0, .VOL_UP = 0, .VOICE_OPEN = 0, .VOICE_CLOSE = 0, 
                                                   .AC_BACK = 0, .AC_HOME = 0, .MENU_PICK = 0, .POWER = 1}};

const hid_consumer_report_t mm_pick      = {.button = {.VOL_DOWN = 0, .VOL_UP = 0, .VOICE_OPEN = 0, .VOICE_CLOSE = 0, 
                                                   .AC_BACK = 0, .AC_HOME = 0, .MENU_PICK = 1, .POWER = 0}};

const hid_consumer_report_t mm_home      = {.button = {.VOL_DOWN = 0, .VOL_UP = 0, .VOICE_OPEN = 0, .VOICE_CLOSE = 0, 
                                                   .AC_BACK = 0, .AC_HOME = 1, .MENU_PICK = 0, .POWER = 0}}; 

const hid_consumer_report_t mm_back      = {.button = {.VOL_DOWN = 0, .VOL_UP = 0, .VOICE_OPEN = 0, .VOICE_CLOSE = 0, 
                                                   .AC_BACK = 1, .AC_HOME = 0, .MENU_PICK = 0, .POWER = 0}};                                                             

const hid_consumer_report_t mm_voice_open  = {.button = {.VOL_DOWN = 0, .VOL_UP = 0, .VOICE_OPEN = 1, .VOICE_CLOSE = 0, 
                                                   .AC_BACK = 0, .AC_HOME = 0, .MENU_PICK = 0, .POWER = 0}};

const hid_consumer_report_t mm_voice_close = {.button = {.VOL_DOWN = 0, .VOL_UP = 0, .VOICE_OPEN = 0, .VOICE_CLOSE = 1, 
                                                   .AC_BACK = 0, .AC_HOME = 0, .MENU_PICK = 0, .POWER = 0}};

const hid_consumer_report_t mm_vol_up    = {.button = {.VOL_DOWN = 0, .VOL_UP = 1, .VOICE_OPEN = 0, .VOICE_CLOSE = 0, 
                                                   .AC_BACK = 0, .AC_HOME = 0, .MENU_PICK = 0, .POWER = 0}};
const hid_consumer_report_t mm_vol_down  = {.button = {.VOL_DOWN = 1, .VOL_UP = 0, .VOICE_OPEN = 0, .VOICE_CLOSE = 0, 
                                                   .AC_BACK = 0, .AC_HOME = 1, .MENU_PICK = 0, .POWER = 0}};
typedef struct 
{
    kscan_key_t vk;
    const hid_keybaord_report_t *kb_report;
    const hid_consumer_report_t *mm_report;
    const hid_mouse_report_t *mouse_report;
    uint8_t ir_key;
}rcu_key_def_t;

const rcu_key_def_t rcu_key_map_table[RCU_KEY_NUM] = {
    [0] = {
        .vk = VK_KEY_01,    // AC_HOME
        .kb_report = NULL,
        .mm_report = &mm_power,
        .ir_key = 0
    }, 
    [1] = {
        .vk = VK_KEY_05,    // UP
        .kb_report = &kb_up,
        .mm_report = NULL,
        .ir_key = 0
    },
    [2] = {
        .vk = VK_KEY_07,    // DOWN
        .kb_report = &kb_down,
        .mm_report = NULL,
        .ir_key = 0
    },
    [3] = {
        .vk = VK_KEY_02,    // LEFT
        .kb_report = &kb_left,
        .mm_report = NULL,
        .ir_key = 0
    },
    [4] = {
        .vk = VK_KEY_10,    // RIGHT
        .kb_report = &kb_right,
        .mm_report = NULL,
        .ir_key = 0
    },
    [5] = {
        .vk = VK_KEY_06,    // OK
        .kb_report = NULL,
        .mm_report = &mm_pick,
        .ir_key = 0
    },
    [6] = {
        .vk = VK_KEY_03,    // HOME
        .kb_report = NULL,
        .mm_report = &mm_home,
        .ir_key = 0
    },
    [7] = {
        .vk = VK_KEY_08,    // BACK
        .kb_report = NULL,
        .mm_report = &mm_back,
        .ir_key = 0
    },
    [8] = {
        .vk = VK_KEY_11,    // MENU
        .kb_report = &kb_menu,
        .mm_report = NULL,
        .ir_key = 0
    },
    [9] = {
        .vk = VK_KEY_12,    // VOICE
        .kb_report = NULL,
        .mm_report = NULL,
        .ir_key = 0,
    },
    [10] = {
        .vk = VK_KEY_04,    // VOL+
        .kb_report = NULL,
        .mm_report = &mm_vol_up,
        .ir_key = 0
    },
    [11] = {
        .vk = VK_KEY_13,    // VOL-
        .kb_report = NULL,
        .mm_report = &mm_vol_down,
        .ir_key = 0
    }     
};



// static bool get_ir_key(kscan_key_t vk, uint8_t *match_key) 
// {
//     for(int i = 0; i < RCU_KEY_NUM; i++) {
//         if (vk == rcu_key_map_table[i].vk) {
//             *match_key = rcu_key_map_table[i].ir_key;
//             return true;
//         }        
//     }
//     return false;
// }

// /**
//  * @brief 
//  * 
//  * @param vk 
//  * @return int 
//  */
// int rcu_send_hid_key_down(kscan_key_t vk)
// {
//     int ret;
//     press_key_data hid_key;
//     if (get_hid_key(vk, &hid_key)) {
//         ret = hid_key_board_send(&hid_key);
//         return ret;
//     } else {
//         LOGE("KEY", "hid key not found");
//         return -1;
//     }
// }

// int rcu_send_hid_key_release()
// {
//     int ret;
//     press_key_data hid_key = {0};
//     ret = hid_key_board_send(&hid_key);
//     return ret;
// }

// int rcu_send_ir_key_down(kscan_key_t vk) 
// {
//     int ret;
//     uint8_t ir_key;
//     if (get_ir_key(vk, &ir_key)) {
//         ret = ir_nec_start_send(0x40, ir_key);
//         return ret; 
//     } else {
//         LOGE("KEY", "ir key not found");
//         return -1;
//     }
// }

// int rcu_send_ir_key_release()
// {
//     return ir_nec_stop_send();
// }

static bool is_key_down_kb = false;
static bool is_key_down_mm = false;
static bool is_key_down_voice = false;
int rcu_send_key_down(kscan_key_t vk)
{
    hid_keybaord_report_t keyboard;
    hid_consumer_report_t consumer;
    // 语音键特殊处理
    if (vk == VK_KEY_12) {
        is_key_down_voice = true;
        memcpy(&consumer, &mm_voice_open, sizeof(hid_consumer_report_t));
        return hids_send_consumer(&consumer);
    }

    for(int i = 0; i < RCU_KEY_NUM; i++) {
        if (rcu_key_map_table[i].vk == vk) {
            const rcu_key_def_t *key = &rcu_key_map_table[i];
            if (key->kb_report != NULL) {
                is_key_down_kb = true;
                memcpy(&keyboard, key->kb_report, sizeof(hid_keybaord_report_t));
                return hids_send_keyboard(&keyboard);
            }
            if (key->mm_report != NULL) {
                is_key_down_mm = true;
                memcpy(&consumer, key->mm_report, sizeof(hid_consumer_report_t));
                return hids_send_consumer(&consumer);
            }
        }
    }
    return -1;
}

int rcu_send_key_release(kscan_key_t vk) 
{
    hid_keybaord_report_t keyboard;
    hid_consumer_report_t consumer;
    if (is_key_down_voice) {
        memcpy(&consumer, &mm_voice_close, sizeof(hid_consumer_report_t));
        hids_send_consumer(&consumer);
        is_key_down_voice = false;
    }
    if (is_key_down_kb) {
        memcpy(&keyboard, &kb_release_all, sizeof(hid_keybaord_report_t));
        hids_send_keyboard(&keyboard);
        is_key_down_kb = false;
    }

    if (is_key_down_mm) {
        memcpy(&consumer, &mm_release_all, sizeof(hid_consumer_report_t));
        hids_send_consumer(&consumer);
        is_key_down_mm = false;
    }

    return 0;
}
