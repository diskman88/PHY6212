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
 * @file     main.c
 * @brief    CSI Source File for main
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/
#include <yoc_config.h>
#include <k_api.h>
#include <aos/log.h>
#include <aos/kv.h>
#include <yoc/sysinfo.h>
#include <yoc/yoc.h>
#include <devices/devicelist.h>

#define INIT_TASK_STACK_SIZE 2048
static cpu_stack_t app_stack[INIT_TASK_STACK_SIZE / 4] __attribute((section(".data")));
static ktask_t app_task_handle = {0};

extern void app_main();
static void application_task_entry(void *arg)
{
    board_base_init();

    app_main();
}

__attribute((weak)) int main(void)
{
    /* kernel init */
    aos_init();

    /* init task */
    krhino_task_create(&app_task_handle, "app_task", NULL,
                       AOS_DEFAULT_APP_PRI, 0, app_stack,
                       INIT_TASK_STACK_SIZE / 4, application_task_entry, 1);

    /* kernel start */
    aos_start();

    return 0;
}

