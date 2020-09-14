/**
 * @file led.c
 * @author zhangkaihua
 * @brief 
 * @version 0.1
 * @date 2020-08-25
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "leds.h"
#include "pinmux.h"
#include "gpio.h"
#include <devices/device.h>
#include <devices/devicelist.h>
#include <devices/led.h>

static dev_t *dev_red_led = NULL;
static led_pin_config_t led_config = {
        .flip = 0,
        .pin_r = GPIO_P23,
        .pin_g = LED_PIN_NOT_SET,
        .pin_b = LED_PIN_NOT_SET,
    };
    
void rcu_led_init()
{
    // led 控制
    led_rgb_register(&led_config, 0);
    dev_red_led = led_open_id("ledrgb", 0);
    if (dev_red_led == NULL) {
        LOGE("MAIN", "led register failed");
    }
}

void rcu_led_red_on()
{
    // led_control(dev_red_led, COLOR_RED, 1, 0);
    led_control(dev_red_led, COLOR_WHITE, -1, -1);
}

void rcu_led_red_off()
{
    // led_control(dev_red_led, COLOR_RED, 0, 1);
    led_control(dev_red_led, COLOR_BLACK, -1, -1);
}

void rcu_led_red_flash(int on_time, int off_time)
{
    led_control(dev_red_led, COLOR_RED, on_time, off_time);
}
