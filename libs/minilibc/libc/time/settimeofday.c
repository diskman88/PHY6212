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
#include <sys/time.h>
#include <time.h>

extern int clock_settime(clockid_t clockid, const struct timespec *tp);

int settimeofday(const struct timeval *tv, const struct timezone *tz)
{
    struct timespec ts;

    if (!tv || tv->tv_usec >= USEC_PER_SEC) {
        return -1;
    }

    /* Convert the timeval to a timespec */

    ts.tv_sec  = tv->tv_sec;
    ts.tv_nsec = tv->tv_usec * NSEC_PER_USEC;

    /* Let clock_settime do the work */

    return clock_settime(CLOCK_REALTIME, &ts);
}

