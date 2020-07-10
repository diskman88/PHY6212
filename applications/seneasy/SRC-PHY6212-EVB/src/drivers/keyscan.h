
#ifndef _KEYSCAN_H
#define _KEYSCAN_H

#include <stdint.h>
#include <stdbool.h>

#include "../app_msg.h"

typedef enum {
    VK_KEY_1 = 0x31,
    VK_KEY_2 = 0x32,
    VK_KEY_3 = 0x33,
    VK_KEY_4 = 0x34,
    VK_KEY_5 = 0x35,
    VK_KEY_6 = 0x36,
    VK_KEY_7 = 0x37,
    VK_KEY_8 = 0x38,
    VK_KEY_9 = 0x39,
    VK_KEY_10 = 0x3A,
    VK_KEY_11 = 0x3B,
    VK_KEY_12 = 0x3C,
    VK_KEY_13 = 0x3D,

    VK_FUNC_START = 0x80,
    VK_FUNC_1 = 0x80,
    VK_FUNC_2,
    VK_FUNC_3,
    VK_FUNC_4,
    VK_FUNC_5,
    VK_FUNC_6,
    VK_FUNC_END 
}kscan_key_t;


int create_keyscan_task();

#endif
