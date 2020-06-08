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
 * @file     event_test.c
 * @brief    the main function for the event test
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <aos/kernel.h>


#define EVENT_FLAG_3        0x000000F0
#define EVENT_INIT_FLAG     0x0F0F0F0F
static uint8_t stack_buf1[1024];
static uint8_t stack_buf2[1024];

static aos_event_t  event;
static aos_task_t   task1;
static aos_task_t   task2;
static aos_sem_t    sem;

static void event_test_task1_entry(void *arg)
{
    int  ret;
    uint32_t actl_flags;

    /* try to get flag EVENT_FLAG_3(0x000000F0) with OR operation should fail */
    ret = aos_event_get(&event, EVENT_FLAG_3, AOS_EVENT_OR, &actl_flags, 0);
    printf("no wait event should fail,execute %s\r\n ", (ret != 0) ? "success" : "fail");
    /*
     * try to get flag EVENT_FLAG_3(0x000000F0) with OR operation should wait here,
     * task2 will set the flags with 0x000000F0, and then task1 will continue
     */
    ret = aos_event_get(&event, EVENT_FLAG_3, AOS_EVENT_OR, &actl_flags, AOS_WAIT_FOREVER);
    printf("wait forever event should success,execute %s\r\n ", (ret == 0) ? "success" : "fail");

    /* give the samphore to let main task continue*/
    aos_sem_signal(&sem);

    printf("execute %s\r\n ", (ret == 0) ? "success" : "fail");

    /* exit self */
    aos_task_exit(0);
}

static void event_test_task2_entry(void *arg)
{
    int  ret;

    /*set event flag(0x000000F0) with OR operation, this will un-block task1 */

    ret = aos_event_set(&event, EVENT_FLAG_3, AOS_EVENT_OR);

    if(ret != 0){
        printf("aos_event_set err\r\n");
    }
    /* exit self */
    aos_task_exit(0);
}

void event_example_main(void *arg)
{
    int  ret;

    /* create event */

    ret = aos_event_new(&event, EVENT_INIT_FLAG);
    printf("aos_event_new  event execute %s\r\n ", (ret == 0) ? "success" : "fail");

    /*create a semphore which will be used for sync multi-task*/
    ret = aos_sem_new(&sem, 0);
    printf("aos_sem_new sem execute %s\r\n ", (ret == 0) ? "success" : "fail");

    /*the event now should has event flag value as EVENT_INIT_FLAG = 0x0F0F0F0F */

    /* create task1 */

    ret = aos_task_new_ext(&task1, "event_test_task1", event_test_task1_entry,
                           0, stack_buf1, 1024, AOS_DEFAULT_APP_PRI);
    printf("aos_task_new_ext task1  execute %s\r\n ", (ret == 0) ? "success" : "fail");

    /* create task2, which has a lower pri then task1, this make sure task1 will run first */

    ret = aos_task_new_ext(&task2, "event_test_task2", event_test_task2_entry,
                           0, stack_buf2, 1024, AOS_DEFAULT_APP_PRI + 1);
    printf("aos_task_new_ext task2  execute %s\r\n ", (ret == 0) ? "success" : "fail");

    /* wait for task1 to give samphore */
    ret = aos_sem_wait(&sem, AOS_WAIT_FOREVER);

    /* destory samphore */
    aos_sem_free(&sem);

    /* destory event */

    aos_event_free(&event);

}

