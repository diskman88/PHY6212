
#ifndef _APP_MSG_H
#define _APP_MSG_H

#include <aos/kernel.h>
#include <aos/types.h>
#include <aos/log.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    MSG_BUTTON = 0,
};

typedef struct
{
    uint16_t type;
    uint16_t event;
    union 
    {
        uint32_t param;
        void *lpMsgBuff;
    };
}io_msg_t;

int32_t io_message_queue_init();

int32_t io_send_message(io_msg_t *msg);

int32_t io_recv_message(io_msg_t *msg, uint32_t ms);

int io_msg_get_len();

#endif


