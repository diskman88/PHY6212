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
#include <stdio.h>
#include <yoc/sysinfo.h>
#include <yoc/yoc.h>
#include "aos/kernel.h"
#include <app_init.h>

#define INIT_TASK_STACK_SIZE    2048

static uint8_t stack_buf[2048];

aos_task_t   task;
extern void event_example_main(void *arg);

int app_main(int argc, char *argv[])
{
    int ret;

    board_yoc_init();

    ret =
        aos_task_new_ext(&task, "event_example_main", event_example_main, 0, stack_buf, INIT_TASK_STACK_SIZE, AOS_DEFAULT_APP_PRI
                         - 1);

    printf("aos_task_new_ext main task execute %s\r\n ", (ret == 0) ? "success" : "fail");

    return 0;
}
