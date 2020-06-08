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

#ifndef HAL_SENSOR_IMPL_H
#define HAL_SENSOR_IMPL_H

#include <stdint.h>

#include <devices/driver.h>

typedef struct sensor_pin_config {
    int pin;
} sensor_pin_config_t;

typedef struct sensor_driver {
    driver_t drv;
    int (*fetch)(dev_t *dev);
    int (*getvalue)(dev_t *dev, void *value, size_t size);
} sensor_driver_t;

#endif
