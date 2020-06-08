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
 * @file     example_wdt.c
 * @brief    the main function for the WDT driver
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/
#include "drv_wdt.h"
#include "drv_intc.h"
#include "app_init.h"
#include "stdio.h"
#include <app_init.h>
#include "pins.h"


extern void mdelay(uint32_t ms);

#define TEST_WDT_RESET 0
#define WDT_TIMEOUT    2000  //(0x10000 << 7)/ (drv_get_sys_freq() / 1000)

static void wdt_event_cb_fun(int32_t idx, wdt_event_e event)
{
    
}


#if (TEST_WDT_RESET)
static int32_t wdt_fun_sysreset(wdt_handle_t wdt_handle)
{
    int32_t ret;

    ret = csi_wdt_restart(wdt_handle);    // restart the counter to its initial value

    if (ret < 0) {
        return -1;
    }

    typedef void(*void_func)();
    void_func func = NULL;
    func(); 
    printf("system will be restarted!\n");
    return -1;
}
#endif

static int32_t test_wdt(void)
{
    //uint32_t base = 0u;
    //uint32_t irq = 0u;
    int32_t ret;
    wdt_handle_t wdt_handle = NULL;

    wdt_handle = csi_wdt_initialize(0, wdt_event_cb_fun);
    //close feed wdt intr
    //target_get_wdt(0, &base, &irq);
    if (wdt_handle == NULL) {
        printf("csi_wdt_initialize error\n");
        return -1;
    }

    ret = csi_wdt_set_timeout(wdt_handle, WDT_TIMEOUT);

    if (ret < 0) {
        printf("csi_wdt_set_timeout error\n");
        return -1;
    }

#if (TEST_WDT_RESET)
    ret = wdt_fun_sysreset(wdt_handle);

    if (ret < 0) {
        printf("wdt_fun_sysreset error\n");
        return -1;
    }

#endif
    ret = csi_wdt_uninitialize(wdt_handle);

    if (ret < 0) {
        printf("csi_wdt_uninitialize error\n");
        return -1;
    }


    return 0;
}

int example_wdt(void)
{
    int ret;
    ret = test_wdt();

    if (ret < 0) {
        printf("test_wdt failed\n");
        return -1;
    }

    printf("test_wdt successfully\n");
    return 0;
}

int app_main(int argc, char *argv[])
{
    board_yoc_init();

    return example_wdt();
}
