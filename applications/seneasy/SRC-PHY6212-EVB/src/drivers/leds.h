/**
 * @file led.h
 * @author zhangkaihua
 * @brief 
 * @version 0.1
 * @date 2020-08-25
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef _LED_H
#define _LED_H
#include <yoc_config.h>
#include <aos/log.h>
#include <aos/kernel.h>
#include <app_init.h>
#include <stdint.h>
#include <stdbool.h>
#include "drv_gpio.h"
#include "gpio.h"

#define RED_LED_PIN GPIO_P23


void rcu_led_init();

void rcu_led_red_on();

void rcu_led_red_off();

void rcu_led_red_flash(int on_time, int off_time);

#endif
