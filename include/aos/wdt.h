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

#ifndef __AOS_WDT_H__
#define __AOS_WDT_H__

#include <stdint.h>

uint32_t aos_wdt_index(void);
void aos_wdt_attach(uint32_t index, void (*will)(void *), void *args);
void aos_wdt_detach(uint32_t index);
int  aos_wdt_exists(uint32_t index);
void aos_wdt_feed(uint32_t index, int max_time);
void aos_wdt_show(uint32_t index);
void aos_wdt_showall();

int  aos_wdt_hw_enable(void);
void aos_wdt_hw_disable(void);
void aos_wdt_hw_restart(void);

#endif
