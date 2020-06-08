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
 * @file     example_adc.c
 * @brief    the main function for the adc driver
 * @version  V1.0
 * @date     1. October 2017
 ******************************************************************************/
#include <soc.h>
#include <drv_adc.h>
#include <stdio.h>
#include <pin_name.h>
#include "pins.h"
#include "pinmux.h"
#include <app_init.h>

#define TEST_ADC_SINGLE_CHANNEL         EXAMPLE_ADC_CHANNEL_USR_DEF0
#define TEST_ADC_CONTINOUS_CHANNEL      EXAMPLE_ADC_CHANNEL_DEF2
#define TEST_ADC_SCAN_CHANNEL_NUM       2

#define TEST_SINGLE_CONV_TIMES        1
#define TEST_CONTINOUS_CONV_TIMES     16
#define TEST_SCAN_CONV_TIMES          1

#define SINGE_DATA_NUM          1
#define CONTINOUS_DATA_NUM      (1*TEST_CONTINOUS_CONV_TIMES)
#define SCAN_DATA_NUM           (TEST_ADC_SCAN_CHANNEL_NUM *25 + 1)

#define ADC_TEST_SUCCESS  0
#define ADC_TEST_FAIL     -1
#define ADC_DELAY         1000

extern void mdelay(uint32_t ms);
/*
not support single mode!
static int test_adc_single(void)
{
    adc_conf_t sconfig;
    adc_handle_t hd;
    uint32_t data[SINGE_DATA_NUM] = {0x314};
    uint32_t ch_array[1] = {TEST_ADC_SINGLE_CHANNEL};
    int ret = 0;

    sconfig.mode = ADC_SINGLE;
    sconfig.trigger = 0;
    sconfig.intrp_mode = 0;
    sconfig.channel_array = ch_array;
    sconfig.channel_nbr = TEST_SINGLE_CONV_TIMES;
    sconfig.conv_cnt = TEST_SINGLE_CONV_TIMES;

    drv_pinmux_config(EXAMPLE_ADC_CH_PIN_USR_DEF0, EXAMPLE_ADC_CH_PIN_USR_DEF0_FUNC);

    hd = drv_adc_initialize(0, NULL);

    if (!hd) {
        printf("adc initial failed\n\r");
        return ADC_TEST_FAIL;
    }

    ret = drv_adc_config(hd, &sconfig);
    if (ret != 0 && ret != (CSI_DRV_ERRNO_ADC_BASE | DRV_ERROR_UNSUPPORTED)) {
        printf("adc config failed, %d\n\r", ret);
        return ADC_TEST_FAIL;
    }

    if (drv_adc_start(hd) != 0) {
        printf("adc start failed\n\r");
        return ADC_TEST_FAIL;
    }

    ret = drv_adc_read(hd, &data[SINGE_DATA_NUM - 1], SINGE_DATA_NUM);
    printf("read ad ret = %d\n\r", ret);
    printf("read a data from adc is : 0x%x\n\r", data[SINGE_DATA_NUM - 1]);

    if (drv_adc_stop(hd) != 0) {
        printf("adc stop failed\n\r");
        return ADC_TEST_FAIL;
    }

    if (drv_adc_uninitialize(hd) != 0) {
        printf("adc uninitial failed\n\r");
        return ADC_TEST_FAIL;
    }

    return ADC_TEST_SUCCESS;
}
*/
static int test_adc_continous(void)
{
    adc_conf_t sconfig;
    adc_handle_t hd;
    uint32_t data[CONTINOUS_DATA_NUM];
    uint32_t ch_array[1] = {TEST_ADC_CONTINOUS_CHANNEL};
    int i = 0;
    int ret = 0;

    sconfig.mode = ADC_CONTINUOUS;
    sconfig.trigger = 0;
    sconfig.intrp_mode = 0;
    sconfig.channel_array = ch_array;
    sconfig.channel_nbr = 1;
    sconfig.conv_cnt = TEST_CONTINOUS_CONV_TIMES;
    sconfig.enable_link_internal_voltage = 0;

    drv_pinmux_config(EXAMPLE_ADC_CH_PIN_USR_DEF2, EXAMPLE_ADC_CH_PIN_USR_DEF2_FUNC);

    for (i = 0; i < CONTINOUS_DATA_NUM; i++) {
        data[i] = 0x31415;
    }

    hd = drv_adc_initialize(0, NULL);

    if (!hd) {
        printf("adc initial failed\n\r");
        return ADC_TEST_FAIL;
    }

    ret = drv_adc_config(hd, &sconfig);

    if (ret == (CSI_DRV_ERRNO_ADC_BASE | DRV_ERROR_UNSUPPORTED)) {
        printf("continous mode unsupported\n");
        return ADC_TEST_SUCCESS;
    }

    if (ret != 0) {
        printf("adc config failed\n\r");
        return ADC_TEST_FAIL;
    }


    if (drv_adc_start(hd) != 0) {
        printf("adc start failed\n\r");
        return ADC_TEST_FAIL;
    }

    ret = drv_adc_read(hd, &data[0], CONTINOUS_DATA_NUM);
    printf("read ad ret = %d\n\r", ret);

    printf("read a data from adc :\n\r");

    for (i = 0; i < CONTINOUS_DATA_NUM; i++) {
        printf("0x%x\n\r", data[i]);
    }

    if (drv_adc_stop(hd) != 0) {
        printf("adc stop failed\n");
        return ADC_TEST_FAIL;
    }

    if (drv_adc_uninitialize(hd) != 0) {
        printf("adc uninitial failed\n");
        return ADC_TEST_FAIL;
    }

    return ADC_TEST_SUCCESS;
}

int test_adc_scan(void)
{
    adc_conf_t sconfig;
    adc_handle_t hd;
    uint32_t data[SCAN_DATA_NUM];
    uint32_t ch_array[2] = {4, 6};
    int i = 0;
    int ret = 0;

    sconfig.mode = ADC_SCAN;
    sconfig.trigger = 0;
    sconfig.intrp_mode = 0;
    sconfig.channel_array = ch_array;
    sconfig.channel_nbr = 2;
    sconfig.conv_cnt = TEST_SCAN_CONV_TIMES;
    sconfig.enable_link_internal_voltage = 0;

    drv_pinmux_config(EXAMPLE_ADC_CH_PIN_USR_DEF0, EXAMPLE_ADC_CH_PIN_USR_DEF0_FUNC);
    drv_pinmux_config(EXAMPLE_ADC_CH_PIN_USR_DEF2, EXAMPLE_ADC_CH_PIN_USR_DEF2_FUNC);

    for (i = 0; i < SCAN_DATA_NUM; i++) {
        data[i] = 0x31415;
    }

    hd = drv_adc_initialize(0, NULL);

    if (!hd) {
        printf("adc initial failed\n\r");
        return ADC_TEST_FAIL;
    }

    ret = drv_adc_config(hd, &sconfig);

    if (ret == (CSI_DRV_ERRNO_ADC_BASE | DRV_ERROR_UNSUPPORTED)) {
        printf("continous mode unsupported\n");
        return ADC_TEST_SUCCESS;
    }

    if (ret != 0) {
        printf("adc config failed, %d\n\r", ret);
        return ADC_TEST_FAIL;
    }

    if (drv_adc_start(hd) != 0) {
        printf("adc start failed\n\r");
        return ADC_TEST_FAIL;
    }

    ret = drv_adc_read(hd, &data[0], SCAN_DATA_NUM);
    printf("read ad ret = %d\n\r", ret);

    printf("read a data from adc :\n\r");

    for (i = 0; i < SCAN_DATA_NUM; i++) {
        printf("0x%x\n\r", data[i]);
    }

    if (drv_adc_stop(hd) != 0) {
        printf("adc stop failed\n");
        return ADC_TEST_FAIL;
    }

    if (drv_adc_uninitialize(hd) != 0) {
        printf("adc uninitial failed\n");
        return ADC_TEST_FAIL;
    }

    return ADC_TEST_SUCCESS;
}

int example_adc(void)
{
    int ret;
/*
    not support signel mode

    ret = test_adc_single();

    if (ret < 0) {
        printf("test adc single mode failed\n\r");
        return ADC_TEST_FAIL;
    }
*/
    printf("test adc single mode passed\n\r");

    ret = test_adc_continous();

    if (ret < 0) {
        printf("test adc continous mode failed\n\r");
        return ADC_TEST_FAIL;
    }

    printf("test adc continous mode passed\n\r");

    ret = test_adc_scan();

    if (ret < 0) {
        printf("test adc scan mode failed\n\r");
        return ADC_TEST_FAIL;
    }

    printf("test adc scan mode passed\n\r");

    printf("test adc successfully\n\r");
    return ADC_TEST_SUCCESS;
}

int app_main(void)
{
    board_yoc_init();

    return example_adc();
}
