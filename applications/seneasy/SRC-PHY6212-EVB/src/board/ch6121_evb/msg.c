#include <aos/kernel.h>
#include <aos/types.h>
#include <aos/log.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "msg.h"

#define msg_queue_len   20
static io_msg_t io_msg_queue_buff[msg_queue_len];
static aos_queue_t io_msg_queue;

int32_t io_message_queue_init()
{
    return aos_queue_new(&io_msg_queue, io_msg_queue_buff, sizeof(io_msg_t) * msg_queue_len, sizeof(io_msg_t));
}

int32_t io_send_message(io_msg_t *msg)
{
    return aos_queue_send(&io_msg_queue, msg, sizeof(io_msg_t));
}

int32_t io_recv_message(io_msg_t *msg, uint32_t ms)
{
    uint32_t size;
    // int ret = aos_queue_recv(&io_msg_queue, ms, msg, &size);
    // if (ret != 0 || size != sizeof(io_msg_t)) {
    //     return -1;
    // } else {
    //     return 0;
    // }
    return aos_queue_recv(&io_msg_queue, ms, msg, &size);
}

int io_msg_get_len()
{
    return aos_queue_get_count(&io_msg_queue);
}