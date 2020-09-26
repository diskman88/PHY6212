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
#include <soc.h>
#include <sys/time.h>
#include <time.h>

extern struct timespec g_basetime;
extern struct timespec last_readtime;
extern int coretimspec(struct timespec *ts);

int clock_settime(clockid_t clockid, const struct timespec *tp)
{
    struct timespec bias;
    uint32_t      flags;
    int             ret = OK;

    if (clockid == CLOCK_REALTIME) {
        /* Interrupts are disabled here so that the in-memory time
         * representation and the RTC setting will be as close as
         * possible.
         */

        flags = csi_irq_save();

        /* Get the elapsed time since power up (in milliseconds).  This is a
         * bias value that we need to use to correct the base time.
         */

        coretimspec(&bias);
        g_basetime.tv_sec = tp->tv_sec;
        g_basetime.tv_nsec = tp->tv_nsec;
        last_readtime.tv_sec = bias.tv_sec;
        last_readtime.tv_nsec = bias.tv_nsec;
        csi_irq_restore(flags);

    } else {
        ret = -1;
    }

    return ret;
}
