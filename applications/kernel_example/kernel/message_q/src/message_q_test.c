/*
 * Copyright (C) 2017 C-SKY Microsystems Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/******************************************************************************
 * @file     message_q_test.c
 * @brief    the main function for the message_q test
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/
#include <aos/kernel.h>
#include <aos/types.h>
#include <aos/log.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#define INIT_TASK_STACK_SIZE    1024
static uint8_t stack_buf1[1024];
aos_task_t   task1;
static uint8_t stack_buf2[1024];
aos_task_t   task2;

#define TEST_CONFIG_QUEUE_BUF_SIZE              (200)
static char         queue_buf1[TEST_CONFIG_QUEUE_BUF_SIZE];
static aos_queue_t  g_queue1;

#define TAG "QUEUE"

static char abuf0[] = "test is message 0";
static char abuf1[] = "test is message 1";
static char abuf2[] = "test is message 2";
static char abuf3[] = "test is message 3";
static char abuf4[] = "test is message 4";
static char error_buf[] = "this is an error";
#define MSG_NUM    5

static void send_Entry(void *arg)
{
    int ret;
    uint32_t i = 0;

    char *buf_p[MSG_NUM + 1] = {abuf0, abuf1, abuf2, abuf3, abuf4, error_buf};

    while (i < 5) {
        ret = aos_queue_send(&g_queue1, buf_p[i], 20);
        i ++;
        aos_msleep(5);

        if (ret != 0) {
            LOGE(TAG, "err send :%d\r\n", i);
        }
    }

    LOGD(TAG, "kernel message queue send  %s\r\n ", (ret == 0) ? "successfully" : "failed");
}

static void recv_Entry(void *arg)
{
    int   ret = 0;
    char msg_recv[32] = {0};
    unsigned int size_recv = 0;

    while (1) {
        memset(msg_recv, 0, sizeof(msg_recv));
        aos_msleep(50);

        ret = aos_queue_recv(&g_queue1, 0, msg_recv, &size_recv);

        if (ret != 0)  {
            printf("in expected, fail to recv message,error:%x\n", ret);
            break;
        }

        LOGD(TAG, "rcv: %s\r\n", (char *)msg_recv);
        aos_msleep(5);
    }

    aos_queue_free(&g_queue1);

    LOGD(TAG, "free the queue successfully!\n");

}

void message_q_example_main(void)
{
    int ret;

    ret =  aos_task_new_ext(&task1, "sendQueue", send_Entry, 0, stack_buf1, INIT_TASK_STACK_SIZE, AOS_DEFAULT_APP_PRI);
    LOGD(TAG, "task1 %s\r\n ", (ret == 0) ? "success" : "fail");


    ret =  aos_task_new_ext(&task2, "receiveQueue", recv_Entry, 0, stack_buf2, INIT_TASK_STACK_SIZE, AOS_DEFAULT_APP_PRI);
    LOGD(TAG, "task2 %s\r\n ", (ret == 0) ? "success" : "fail");

    ret = aos_queue_new(&g_queue1, queue_buf1, TEST_CONFIG_QUEUE_BUF_SIZE, TEST_CONFIG_QUEUE_BUF_SIZE);

    LOGD(TAG, "create the queue %s\r\n ", (ret == 0) ? "success" : "fail");
}
