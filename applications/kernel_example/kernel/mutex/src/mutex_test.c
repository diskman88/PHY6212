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
 * @file     mutex_test.c
 * @brief    the main function for the mutex test
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/
#include <yoc_config.h>
#include <stdio.h>
#include <yoc/sysinfo.h>
#include <yoc/yoc.h>
#include "aos/kernel.h"

static uint8_t stack_buf1[1024];
static uint8_t stack_buf2[1024];

static aos_task_t   task1;
static aos_task_t   task2;

static aos_mutex_t g_mutex1;


static void example_mutextask1(void *arg)
{
    int uwRet;

    printf("task1 try to get  mutex, wait 10 ticks.\n");

    uwRet = aos_mutex_lock(&g_mutex1, 10);

    if (uwRet == 0) {
        printf("task1 get  g_mutex1.\n");
        aos_mutex_unlock(&g_mutex1);

        return;
    } else if (uwRet != 0 || uwRet < 0) {
        printf("task1 timeout and try to get  mutex, wait forever.\n");
        uwRet = aos_mutex_lock(&g_mutex1, -1);

        if (uwRet == 0) {
            printf("task1 wait forever,get  g_mutex1.\n");
            aos_mutex_unlock(&g_mutex1);
        }
    }

    return;
}

static void example_mutextask2(void *arg)
{
    int uwRet;

    printf("task2 try to get  mutex, wait forever.\n");
    uwRet = aos_mutex_lock(&g_mutex1, -1);
    if(uwRet != 0)
    {
        printf("example_mutextask2 aos_mutex_lock fail\r\n");
    }
    printf("task2 get g_mutex1 and suspend 1000 ticks.\n");
    aos_msleep(1000);

    printf("task2 resumed and post the g_mutex1\n");
    aos_mutex_unlock(&g_mutex1);

    return;
}

void mutex_example_main(void *arg)
{
    int ret;

    ret = aos_mutex_new(&g_mutex1);

    if (ret != 0) {
        printf("aos_mutex_new fail\r\n");
    }

    /* create task1 */
    ret = aos_task_new_ext(&task1, "event_test_task1", example_mutextask1,
                           0, stack_buf1, 1024, AOS_DEFAULT_APP_PRI);
    printf("aos_task_new_ext task1  execute %s\r\n ", (ret == 0) ? "success" : "fail");

    /* create task2, which has a lower pri then task1, this make sure task1 will run first */

    ret = aos_task_new_ext(&task2, "event_test_task2", example_mutextask2,
                           0, stack_buf2, 1024, AOS_DEFAULT_APP_PRI - 1);
    printf("aos_task_new_ext task2  execute %s\r\n ", (ret == 0) ? "success" : "fail");

    aos_msleep(5000);

    aos_mutex_free(&g_mutex1);

    printf("test mutex successfully !\n");
}
