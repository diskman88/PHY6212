
#include "app_msg.h"


#define IO_MSG_QUEUE_LEN   20
static io_msg_t io_msg_queue_buff[IO_MSG_QUEUE_LEN];
static aos_queue_t io_msg_queue;
static aos_event_t app_event_flags;

/**
 * @brief 初始化系统IO消息队列
 * 
 * @return bool 
 */
bool app_init_io_message()
{
    if (aos_queue_new(&io_msg_queue, io_msg_queue_buff, sizeof(io_msg_t)*IO_MSG_QUEUE_LEN, sizeof(io_msg_t)) != 0){
        LOGE("MSG", "can`t create io message queue");
        return false;
    }

    if (aos_event_new(&app_event_flags, 0) != 0) {
        LOGE("MSG", "can`t create system event");
        return false;
    }

    return true;
}

/**
 * @brief 发送一条消息到消息队列
 * 
 * @param msg 
 * @return int32_t 
 */
bool app_send_io_message(io_msg_t *msg)
{
    if (aos_queue_send(&io_msg_queue, msg, sizeof(io_msg_t)) != 0) {
        LOGE("MSG", "failed to send io message");
        return false;
    }

    if (aos_event_set(&app_event_flags, APP_EVENT_IO, AOS_EVENT_OR) != 0) {
        LOGE("MSG", "failed to set io event");
        return false;
    }
    
    return true;
}

/**
 * @brief 从消息队列取一条消息
 * 
 * @param msg 
 * @param ms 
 * @return bool 
 */
bool app_recv_io_message(io_msg_t *msg, uint32_t ms)
{
    uint32_t size;
    
    if (aos_queue_recv(&io_msg_queue, ms, msg, &size) != 0) {
        // LOGE("MSG", "failed to recv io message")
        return false;
    } else {
        return true;
    }
}


/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool app_is_io_mesage_full()
{
    if (aos_queue_get_count(&io_msg_queue) < IO_MSG_QUEUE_LEN){
        return false;
    } else {
        return true;
    }
}

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool app_is_io_message_empty()
{
    if (aos_queue_get_count(&io_msg_queue) == 0){
        return true;
    } else {
        return false;
    }    
}

/**
 * @brief 触发事件
 * 
 * @param event 
 * @return bool 
 */
bool app_event_set(app_event_t event)
{
    if(aos_event_set(&app_event_flags, event, AOS_EVENT_OR) != 0) {
        // LOGE("MSG", "event error %d", err);
        return false;
    } else {
        return true;
    }
}

/**
 * @brief 等待事件
 * 
 * @param require_flags 等待的时间标志,可以or操作同时等待多个事件
 * @param actl_flags 实际触发的事件
 * @param timeout 
 * @return bool 
 */
bool app_event_get(app_event_t require_flags, app_event_t *actl_flags, uint32_t timeout)
{
    if(aos_event_get(&app_event_flags, require_flags, AOS_EVENT_OR_CLEAR, actl_flags, timeout) != 0) {
        //
        return false;
    } else {
        return true;
    }
}