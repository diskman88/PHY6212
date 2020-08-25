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

#include "led.h"
#include "pinmux.h"
#include "gpio.h"

// gpio_pin_handle_t led = NULL;

void led_on()
{
    drv_pinmux_config(LED_PIN, PIN_FUNC_GPIO);
    phy_gpio_pin_init(LED_PIN, OEN);
    phy_gpio_pull_set(LED_PIN, PULL_DOWN);
    phy_gpio_write(LED_PIN, 0);

    // if (led == NULL) {
    //     led = csi_gpio_pin_initialize(LED_PIN, NULL);
    //     csi_gpio_pin_config_direction(led, GPIO_DIRECTION_OUTPUT);
    //     csi_gpio_pin_config_mode(led, GPIO_MODE_PUSH_PULL);
    // } 
    // csi_gpio_pin_write(led, true);
}

void led_off()
{
    drv_pinmux_config(LED_PIN, PIN_FUNC_GPIO);
    phy_gpio_pin_init(LED_PIN, OEN);
    phy_gpio_pull_set(LED_PIN, WEAK_PULL_UP);
    phy_gpio_write(LED_PIN, 1);
}

void led_flash(uint32_t duty, uint32_t period, uint16_t times)
{

}