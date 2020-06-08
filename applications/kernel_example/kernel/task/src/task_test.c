/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */
#include <yoc_config.h>
#include <stdio.h>
#include <yoc/sysinfo.h>
#include <yoc/yoc.h>
#include "aos/kernel.h"
#include <aos/log.h>


#define TAG    "TASK"

static uint8_t stack_buf1[1024];
static uint8_t stack_buf2[1024];

static aos_task_t   task1;
static aos_task_t   task2;



void Example_TaskHi()
{
    int i=0; 
    printf("high task name %s: \r\n", aos_task_name());

    while(1)
    {
        aos_msleep(1000);
        LOGD(TAG,"test high task times:%d\r\n",i++);
    }
    aos_task_exit(0);
}


void Example_TaskLo()
{
    int i=0;
    printf("low task name %s: \r\n", aos_task_name());
    while(1)
    {
        aos_msleep(1000);
        LOGD(TAG,"test low task times:%d\r\n",i++);
    }
    aos_task_exit(0);
}

void task_example_main(void *arg)
{
    int ret;

    ret = aos_task_new_ext(&task2, "HIGH_NAME", Example_TaskHi,
                           0, stack_buf2, 1024, AOS_DEFAULT_APP_PRI - 1);
    if(ret != 0)
    {
        printf("aos_task_new_ext  high task execute %s\r\n ",(ret == 0)? "success":"fail");
    }

    ret = aos_task_new_ext(&task1, "LOW_NAME", Example_TaskLo,
                           0, stack_buf1, 1024, AOS_DEFAULT_APP_PRI);
    if(ret != 0)
    {
        printf("aos_task_new_ext  low task execute %s\r\n ",(ret == 0)? "success":"fail");
    }

}




