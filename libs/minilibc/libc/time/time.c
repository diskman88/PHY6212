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
#include <time.h>

extern int clock_gettime(clockid_t clockid, struct timespec *tp);

time_t time(time_t *tloc)
{
  struct timespec ts;
  int ret;

  /* Get the current time from the system */

  ret = clock_gettime(CLOCK_REALTIME, &ts);
  if (ret == 0)
    {
      /* Return the seconds since the epoch */

      if (tloc)
        {
          *tloc = ts.tv_sec;
        }

      return ts.tv_sec;
    }

  return (time_t)-1;
}
