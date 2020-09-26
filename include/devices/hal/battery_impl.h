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

#ifndef HAL_BATTERY_H
#define HAL_BATTERY_H

#include <stdint.h>

#include <devices/driver.h>

typedef enum {
    VOLTAGE = 0,
    CURRENT
} hal_battery_attr_t;

typedef enum {
    REMOVED = 0,
} hal_battery_event_t;

struct _battery_voltage {
    int volt;
};

struct _battery_current {
    int ampere;
};

typedef struct _battery_voltage battery_voltage_t;
typedef struct _battery_current battery_current_t;

typedef int (*battery_event_cb_t)(hal_battery_event_t event);
typedef struct battery_pin_config {
    int pin;
    battery_event_cb_t event_cb;
} battery_pin_config_t;

typedef struct battery_driver {
    driver_t    drv;
    int         (*fetch)(dev_t *dev, hal_battery_attr_t attr);
    int         (*getvalue)(dev_t *dev, hal_battery_attr_t attr, void *value, size_t size);
    int         (*event_cb)(dev_t *dev, hal_battery_event_t event);
} battery_driver_t;

#endif
