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
 * @file     timer_test.c
 * @brief    the main function for the timer test
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/
#include <yoc_config.h>
#include <stdio.h>
#include <yoc/sysinfo.h>
#include <yoc/yoc.h>
#include "aos/kernel.h"


static void Timer1_Callback(void *arg1, void *arg2);
static void Timer2_Callback(void *arg1, void *arg2);

uint32_t g_timercount1 = 0;
uint32_t g_timercount2 = 0;

static void Timer1_Callback(void *arg1, void *arg2)
{
    uint64_t tick_last1;
    tick_last1 = aos_now_ms();
    g_timercount1 ++;
    printf("g_timercount1=%d\n", g_timercount1);
    printf("tick_last1 = %llu\n", tick_last1);
}

static void Timer2_Callback(void *arg1, void *arg2)
{
    uint64_t tick_last2;
    g_timercount2 ++;
    tick_last2 = aos_now_ms();
    printf("g_timercount2=%d\n", g_timercount2);
    printf("tick_last2 = %llu\n", tick_last2);
}

void timer_example_main(void)
{
    int ret;
    aos_timer_t  g_timer1;
    aos_timer_t  g_timer2;

    ret = aos_timer_new(&g_timer1, Timer1_Callback, NULL, 1000, 1);

    if (ret != 0) {
        printf("aos_timer_new  g_timer1 %s\r\n ", (ret == 0) ? "success" : "fail");
    }

    if (!(aos_timer_start(&g_timer1))) {
        printf("start Timer1 successfully\n");
    }

    aos_msleep(200);

    if (aos_timer_stop(&g_timer1) == 0) {
        printf("stop Timer1 successfully\n");
    }

    aos_timer_change(&g_timer1, 500);

    aos_timer_start(&g_timer1);
    aos_msleep(500);
    aos_timer_stop(&g_timer1);
    aos_timer_free(&g_timer1);

    ret = aos_timer_new(&g_timer2, Timer2_Callback, NULL, 200, 10);

    if (ret != 0) {
        printf("aos_timer_new g_timer2 %s\r\n ", (ret == 0) ? "success" : "fail");
    }

    aos_timer_start(&g_timer2);
    printf("start Timer2\n");

    aos_msleep(2000);
    aos_timer_stop(&g_timer2);
    aos_msleep(2000);
    aos_timer_free(&g_timer2);
    printf("test kernel timer successfully !\n");

}





