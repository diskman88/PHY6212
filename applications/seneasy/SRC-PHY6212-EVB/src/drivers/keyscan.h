
#ifndef _KEYSCAN_H
#define _KEYSCAN_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief VK按键定义
 * 
 */
typedef enum {
    // 实体按键
    VK_KEY_NULL = 0,
    VK_KEY_01,
    VK_KEY_02,
    VK_KEY_03,
    VK_KEY_04,
    VK_KEY_05,
    VK_KEY_06,
    VK_KEY_07,
    VK_KEY_08,
    VK_KEY_09,
    VK_KEY_10,
    VK_KEY_11,
    VK_KEY_12,
    VK_KEY_13,
    VK_KEY_14,
    VK_KEY_15,
    VK_KEY_16,
    VK_KEY_17,
    // 功能按键
    VK_KEY_FUNC,
    VK_KEY_FUNC1 = VK_KEY_FUNC,
    VK_KEY_FUNC2,
    VK_KEY_FUNC3,
    VK_KEY_FUNC4,
    VK_KEY_FUNC5,
    VK_KEY_FUNC6,
}kscan_key_t;

int create_keyscan_task();

#endif
