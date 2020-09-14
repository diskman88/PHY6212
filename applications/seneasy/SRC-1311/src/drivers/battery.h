/**
 * @file battery.h
 * @author zhangkaihua (apple_eat@126.com)
 * @brief 
 * @version 1.0
 * @date 2020-09-07
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef __BATTERY_H
#define __BATTERY_H
#include "pinmux.h"
#include "drv_gpio.h"
#include "drv_adc.h"
#include "adc.h"


#define BATTERY_ADC_CH  ADC_CH2P_P14
#define BATTERY_GPIO    GPIO_P14


#define BATTERY_TYPE_LIPOLY     1
#define BATTERY_TYPE_A          0
#define BATTERY_TYPE_B          0
#define BATTERY_TYPE_C          0


int battery_get_level();

#endif