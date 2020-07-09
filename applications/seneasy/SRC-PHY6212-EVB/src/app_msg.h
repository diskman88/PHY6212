
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
typedef enum {
    APP_EVENT_IO        = 0x00000001,       // 输入输出事件，用IO_MSG传递数据
    APP_EVENT_BT        = 0x00000002,       // 蓝牙相关事件
    APP_EVENT_PM        = 0x00000004,       // 睡眠事件
    APP_EVENT_ERR       = 0x80000000
}app_event_t;


/**
 * @brief IO消息定义
 * 
 */
typedef enum {
    IO_MSG_KEYSCAN,
    IO_MSG_VOICE,
    IO_MSG_IR,
    IO_MSG_GPIO
}io_msg_type_t;

/**
 * @brief KEYSCAN sub-message types
 * 
 */
typedef enum {
    IO_MSG_KEYSCAN_KEY_DOWN,
    IO_MSG_KEYSCAN_KEY_RELEASE
}io_keyscan_t;

/**
 * @brief VOICE sub-message types
 * 
 */
typedef enum {
    IO_MSG_VOICE_NEW_FRAME,
}io_voice_t;

/**
 * @brief BT sub-message types
 * 
 */
typedef enum{
    BT_MSG_GATT_WRITE,
    BT_MSG_GATT_READ,
}bt_msg_t;

/**
 * @brief IO消息结构体
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
}io_msg_t;

/**
 * @brief 初始化系统IO消息队列
 * 
 * @return bool 
 */
bool app_init_io_message();

/**
 * @brief 发送一条消息到消息队列
 * 
 * @param msg 
 * @return bool 
 */
bool app_send_io_message(io_msg_t *msg);

/**
 * @brief 从消息队列取一条消息
 * 
 * @param msg 
 * @param ms 
 * @return bool 
 */
bool app_recv_io_message(io_msg_t *msg, uint32_t ms);

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool app_is_io_mesage_full();

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool app_is_io_message_empty();

/**
 * @brief 触发事件
 * 
 * @param event 
 * @return bool 
 */
bool app_event_set(app_event_t event);

/**
 * @brief 等待事件
 * 
 * @param require_flags 等待的时间标志,可以or操作同时等待多个事件
 * @param actl_flags 实际触发的事件
 * @param timeout 
 * @return bool 
 */
bool app_event_get(app_event_t require_flags, app_event_t *actl_flags, uint32_t timeout);

#endif


