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

#ifndef HAL_LED_H
#define HAL_LED_H

#include <stdint.h>

#include <devices/driver.h>

#define BLINK_ON        1
#define BLINK_OFF       0

#define LED_PIN_NOT_SET 0xffffffff

typedef struct led_pin_config {
    int pin_r;
    int pin_g;
    int pin_b;
    int flip;
} led_pin_config_t;

typedef struct led_driver {
    driver_t    drv;
    int         (*control)(dev_t *dev, int color, int on_time, int off_time);
} led_driver_t;

#endif
