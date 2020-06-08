/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <stdio.h>
#include <yoc_config.h>

#include <aos/aos.h>
#include <aos/kernel.h>
#include <aos/ble.h>
#include <app_init.h>

#define TAG "app_main"

int app_main(int argc, char *argv[])
{
    board_yoc_init();
    return 0;
}
