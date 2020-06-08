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

#ifndef DEVICE_BATTERY_H
#define DEVICE_BATTERY_H

#include "hal/battery_impl.h"

#define battery_open(name) device_open(name)
#define battery_open_id(name, id) device_open_id(name, id)
#define battery_close(dev) device_close(dev)

int battery_fetch(dev_t *dev, hal_battery_attr_t attr);
int battery_getvalue(dev_t *dev, hal_battery_attr_t attr, void *value, size_t size);
int battery_event_cb(hal_battery_event_t event);

#endif
