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
 * @file     example_pwm.c
 * @brief    the main function for the PWM driver
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/
#include "drv_pwm.h"
#include "stdio.h"
#include <pins.h>
#include <pinmux.h>
#include <app_init.h>

extern void mdelay(uint32_t ms);

void example_pin_pwm_init(void)
{
    drv_pinmux_config(EXAMPLE_PWM_CH_PIN, EXAMPLE_PWM_CH_FUNC);
}

int32_t  pwm_signal_test(uint32_t pwm_idx, uint8_t pwm_ch)
{
    int32_t ret;
    pwm_handle_t pwm_handle;

    example_pin_pwm_init();

    pwm_handle = csi_pwm_initialize(pwm_idx);

    if (pwm_handle == NULL) {
        printf("csi_pwm_initialize error\n");
        return -1;
    }

    ret = csi_pwm_config(pwm_handle, pwm_ch, 3000, 1500);

    if (ret < 0) {
        printf("csi_pwm_config error\n");
        return -1;
    }

    csi_pwm_start(pwm_handle, pwm_ch);
    mdelay(20);

    ret = csi_pwm_config(pwm_handle, pwm_ch, 200, 150);

    if (ret < 0) {
        printf("csi_pwm_config error\n");
        return -1;
    }

    mdelay(20);
    csi_pwm_stop(pwm_handle, pwm_ch);


    csi_pwm_uninitialize(pwm_handle);

    return 0;

}

int example_pwm(uint32_t pwm_idx, uint8_t pwm_ch)
{
    int32_t ret;
    ret = pwm_signal_test(pwm_idx, pwm_ch);

    if (ret < 0) {
        printf("pwm_signal_test error\n");
        return -1;
    }

    printf("pwm_signal_test OK\n");

    return 0;
}

int app_main(void)
{
    board_yoc_init();

    return example_pwm(EXAMPLE_PWM_IDX, EXAMPLE_PWM_CH);
}
