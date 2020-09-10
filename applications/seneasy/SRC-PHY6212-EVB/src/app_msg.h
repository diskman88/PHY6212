
#ifndef _APP_MSG_H
#define _APP_MSG_H

#include <aos/kernel.h>
#include <aos/types.h>
#include <aos/log.h>
#include <stdint.h>
#include <stdint.h>
#include <stdbool.h>

/************************************************************************************
 *  TYPE DEFINES
 * *********************************************************************************/

/**
 * @brief 系统事件定义
 * 
 */
#define     APP_EVENT_MSG       0x00000001       // 输入输出事件，用IO_MSG传递数据
#define     APP_EVENT_PM        0x00000004       // 睡眠事件
#define     APP_EVENT_VOICE     0x00000008       // 语音事件
#define     APP_EVENT_OTA       0x00000010       // OTA事件
#define     APP_EVENT_MASK      0x0000FFFF

/**
 * @brief 系统消息类型
 * 
 */
typedef enum {
    MSG_KEYSCAN,        // 由按键扫描发送的消息
    MSG_BT_GAP,         // 由GAP发出的消息
    MSG_BT_GATT,        // 由GATT发出的消息
}app_message_type_t;

/**
 * @brief 键盘扫描消息类型
 * 
 */
typedef enum {
    MSG_KEYSCAN_KEY_PRESSED,       // 有按键按下了
    MSG_KEYSCAN_KEY_HOLD,
    MSG_KEYSCAN_KEY_COMBINE,
    MSG_KEYSCAN_KEY_RELEASE,       // 有按键释放了
    MSG_KEYSCAN_KEY_RELEASE_ALL,   // 所有按键都释放了
}keyscan_message_type_t;

/**
 * @brief 
 * 
 */
typedef enum {
    MSG_BT_GAP_CONNECTED,
    MSG_BT_GAP_DISCONNECTED,
}bt_gap_message_type_t;

typedef enum {
    MSG_BT_GATT_HID_OUT_CAPS,
    MSG_BT_GATT_DIS,
    MSG_BT_GATT_BAT,
    MSG_BT_GATT_ATV_MIC_OPEN,
    MSG_BT_GATT_ATV_MIC_STOP
}bt_gatt_message_type_t;


/**
 * @brief 消息结构体
 * 
 */
typedef struct
{
    uint16_t type;
    uint16_t subtype;
    union 
    {   
        uint32_t param;
        void *lpMsgBuff;
        uint8_t data[4];
    };
}app_msg_t;

/**
 * @brief 初始化系统IO消息队列
 * 
 * @return bool 
 */
bool app_init_message();

/**
 * @brief 发送一条消息到消息队列
 * 
 * @param msg 
 * @return bool 
 */
bool app_send_message(app_msg_t *msg);

/**
 * @brief 从消息队列取一条消息
 * 
 * @param msg 
 * @param ms 
 * @return bool 
 */
bool app_recv_message(app_msg_t *msg, uint32_t ms);

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool app_is_mesage_full();

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool app_is_message_empty();

/**
 * @brief 触发事件
 * 
 * @param event 
 * @return bool 
 */
bool app_event_set(uint32_t event);

/**
 * @brief 等待事件
 * 
 * @param require_flags 等待的时间标志,可以or操作同时等待多个事件
 * @param actl_flags 实际触发的事件
 * @param timeout 
 * @return bool 
 */
bool app_event_get(uint32_t require_flags, uint32_t *actl_flags, uint32_t timeout);

#endif


