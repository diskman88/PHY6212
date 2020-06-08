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
 * @file     example_gpio.c
 * @brief    the main function for the GPIO driver
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/
#include <stdio.h>
#include <app_init.h>
#include "dw_gpio.h"
#include "drv_gpio.h"
#include "pin_name.h"
#include "pinmux.h"
#include "pins.h"
#include <app_init.h>


volatile static bool int_flag = 1;

static void gpio_interrupt_handler(int32_t idx)
{
    int_flag = 0;
}

void example_pin_gpio_init(void)
{
    drv_pinmux_config(PIN_GPIO_14, GPIO_PIN_FUNC);
}

void gpio_falling_edge_interrupt(pin_name_e gpio_pin)
{
    gpio_pin_handle_t *pin = NULL;

    example_pin_gpio_init();

    printf("please change the gpio pin %s from high to low\r\n", BOARD_GPIO_PIN_NAME);
    pin = csi_gpio_pin_initialize(gpio_pin, gpio_interrupt_handler);

    int ret = csi_gpio_pin_config_mode(pin, GPIO_MODE_PULLDOWN);
    if (ret != 0) {
        printf("%s, %d, error\n", __func__, __LINE__);
    }

    ret = csi_gpio_pin_config_direction(pin, GPIO_DIRECTION_INPUT);
    if (ret != 0) {
        printf("%s, %d, error\n", __func__, __LINE__);
    }

    ret = csi_gpio_pin_set_irq(pin, GPIO_IRQ_MODE_FALLING_EDGE, 1);
    if (ret != 0) {
        printf("%s, %d, error\n", __func__, __LINE__);
    }

    while (int_flag);

    int_flag = 1;
    csi_gpio_pin_uninitialize(pin);
    printf("gpio falling_edge test successfully!!!\n");
    printf("test gpio successfully\n");
}

/*****************************************************************************
test_gpio: main function of the gpio test

INPUT: NULL

RETURN: NULL

*****************************************************************************/
int example_gpio(pin_name_e gpio_pin)
{
    gpio_falling_edge_interrupt(gpio_pin);
    return 0;
}

int app_main(int argc, char *argv[])
{
    board_yoc_init();

    return example_gpio(PIN_GPIO_14);
}
