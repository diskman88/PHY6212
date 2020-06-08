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
 * @file     sem_test.c
 * @brief    the main function for the sem test
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/
#include <yoc_config.h>
#include <stdio.h>
#include <yoc/yoc.h>
#include "aos/kernel.h"


static aos_sem_t    g_sem;
static aos_timer_t  g_timer;
int g_var = 0;

static void timer_handler(void *arg1, void *arg2)
{
    if (++g_var == 10) {
        aos_sem_signal(&g_sem);
        printf("aos_sem_signal release\r\n");
    }
}

void sem_example_main(void)
{
    int ret = -1;

    g_var = 0;
    ret = aos_sem_new(&g_sem, 0);

    if (ret != 0) {
        printf("aos_sem_new %s\r\n ", (ret == 0) ? "success" : "fail");
    }

    ret = aos_timer_new(&g_timer, timer_handler, NULL, 200, 10);

    if (ret != 0) {
        printf("aos_timer_new %s\r\n ", (ret == 0) ? "success" : "fail");
    }

    ret = aos_sem_wait(&g_sem, 200);
    printf("aos_sem_wait 200ms out of time, %s\r\n ", (ret == -1) ? "success" : "fail");


    ret = aos_sem_wait(&g_sem, AOS_WAIT_FOREVER);
    printf("aos_sem_wait, %s\r\n ", (ret == 0) ? "success" : "fail");


    aos_sem_free(&g_sem);

    aos_timer_stop(&g_timer);
    aos_timer_free(&g_timer);
}

