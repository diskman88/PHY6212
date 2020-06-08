/*
 * Copyright (C) 2017 C-SKY Microsystems Co., All rights reserved.
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

#include <yoc_config.h>
#include <aos/kernel.h>
#include <stdio.h>
#include <time.h>
#include <soc.h>
#include <sys/types.h>
#include "csi_core.h"
#include "drv_timer.h"

extern uint64_t g_sys_tick_count;
extern uint32_t csi_coret_get_value(void);
extern uint32_t csi_coret_get_load(void);

#ifndef CONFIG_CLOCK_BASETIME
#define CONFIG_CLOCK_BASETIME 1003939200
#endif
struct timespec g_basetime = {
    .tv_sec = CONFIG_CLOCK_BASETIME,
    .tv_nsec = 0
};

struct timespec last_readtime = {
    .tv_sec = 0,
    .tv_nsec = 0
};

/*
 * return : the coretim register count in a tick(calculate by TICK_PER_SEC)
 */

static uint32_t coretim_getpass(void)
{
    uint32_t cvalue;
    int      value;

    uint32_t loadtime;
    loadtime = csi_coret_get_load();
    cvalue = csi_coret_get_value();
    value = loadtime - cvalue;

    return value > 0 ? value : 0;
}

int coretimspec(struct timespec *ts)
{
    uint32_t msecs;
    uint32_t secs;
    uint32_t nsecs = 0;

    if (ts == NULL) {
        return -1;
    }

    while (1) {
        uint64_t clk1, clk2;
        uint32_t pass1, pass2;

        clk1 = aos_now_ms();
        pass1 = coretim_getpass();
        clk2 = aos_now_ms();
        pass2 = coretim_getpass();

        if (clk1 == clk2 && pass2 > pass1) {
            msecs = clk1;
            nsecs = pass2 * (NSEC_PER_SEC / drv_get_sys_freq());
            break;
        }
    }

    secs = msecs / MSEC_PER_SEC;

    nsecs += (msecs - (secs * MSEC_PER_SEC)) * NSEC_PER_MSEC;

    if (nsecs > NSEC_PER_SEC) {
        nsecs -= NSEC_PER_SEC;
        secs += 1;
    }

    ts->tv_sec = (time_t)secs;
    ts->tv_nsec = (long)nsecs;
    return OK;
}
