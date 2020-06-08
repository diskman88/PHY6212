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
 * @file     example_spi.c
 * @brief    the main function for the SPI driver
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/
#include "dw_spi.h"
#include "drv_spi.h"
#include "stdio.h"
#include <string.h>
#include <app_init.h>
#include "pins.h"
#include "gpio.h"
#include "w25q64fv.h"
#include "pinmux.h"
#include <app_init.h>


#define OPERATE_ADDR    0x0
#define OPERATE_LEN     256
#define SPIFLASH_BASE_VALUE 0x0
static uint8_t erase_read_flag = 0;
static uint8_t program_read_flag = 0;


extern int32_t w25q64flash_read_id(spi_handle_t handle, uint32_t *id_num);
extern int32_t w25q64flash_erase_sector(spi_handle_t handle, uint32_t addr);
extern int32_t w25q64flash_erase_chip(spi_handle_t handle);
extern int32_t w25q64flash_program_data(spi_handle_t handle, uint32_t addr, const void *data, uint32_t cnt);
extern int32_t w25q64flash_read_data(spi_handle_t handle, uint32_t addr, void *data, uint32_t cnt);

static void spi_event_cb_fun(int32_t idx, spi_event_e event)
{
    //printf("\nspi_event_cb_fun:%d\n",event);
}

void example_pin_spi_init(void)
{
    drv_pinmux_config(PIN_SPI_MISO, SPI_0_RX);
    drv_pinmux_config(PIN_SPI_MOSI, SPI_0_TX);
    drv_pinmux_config(PIN_SPI_CS, SPI_0_SSN);
    drv_pinmux_config(PIN_SPI_SCK, SPI_0_SCK);
}

static int test_spi_eeprom(int32_t idx)
{
    uint8_t id[5] = {0x11, 0x11};
    uint8_t input[OPERATE_LEN] = {0};
    uint8_t output[OPERATE_LEN] = {0};
    int i;
    int32_t ret;
    spi_handle_t spi_handle_t;

    example_pin_spi_init();

    spi_handle_t = csi_spi_initialize(idx, spi_event_cb_fun);

    if (spi_handle_t == NULL) {
        printf(" csi_spi_initialize error\n");
        return -1;
    }

    ret = w25q64flash_read_id(spi_handle_t, (uint32_t *)&id);

    if (ret < 0) {
        printf(" flash_read_id error\n");
        return -1;
    }

    printf("the spiflash id is %x %x\r\n", id[3], id[4]);

    ret = w25q64flash_erase_sector(spi_handle_t, OPERATE_ADDR);

    if (ret < 0) {
        printf(" flash_erase_sector error\n");
        return -1;
    }

    ret = w25q64flash_read_data(spi_handle_t, OPERATE_ADDR, output, OPERATE_LEN);

    if (ret < 0) {
        printf(" flash_read_data error\n");
        return -1;
    }

    printf("erase sector and then read\n");

    for (i = 0; i < OPERATE_LEN; i++) {
        if ((i % 10) == 0) {
            printf("output[%d]", i);
        }

        printf("%x ", output[i]);

        if (((i + 1) % 10) == 0) {
            printf("\n");
        }

        if (output[i] != 0xff) {
            erase_read_flag = 1;
            break;
        }
    }

    printf("\n");

    if (erase_read_flag == 1) {
        printf("flash erase check and read check error\n");
        return -1;
    }

    for (i = 0; i < sizeof(input); i++) {
        input[i] = i + SPIFLASH_BASE_VALUE;
    }

    printf("flash erase sector successfully\n");
    memset(output, 0xfb, sizeof(output));

    w25q64flash_program_data(spi_handle_t, OPERATE_ADDR, input, OPERATE_LEN);
    w25q64flash_read_data(spi_handle_t, OPERATE_ADDR, output, OPERATE_LEN);

    printf("program data and then read\n");

    for (i = 0; i < OPERATE_LEN; i++) {
        if ((i % 10) == 0) {
            printf("output[%d]", i);
        }

        printf("%x ", output[i]);

        if (((i + 1) % 10) == 0) {
            printf("\n");
        }

        if (output[i] != input[i]) {
            program_read_flag = 1;
            break;
        }
    }

    printf("\n");

    if (program_read_flag == 1) {
        printf("flash program and read check error\n");
        return -1;
    }

    printf("flash program data successfully\n");

    ret = csi_spi_uninitialize(spi_handle_t);
    return 0;

}

int example_spi(int32_t idx)
{
    int ret;
    ret = test_spi_eeprom(idx);

    if (ret < 0) {
        printf("test spi eeprom failed\n");
        return -1;
    }

    printf("test spi eeprom successfully\n");

    return 0;
}

int app_main(int argc, char *argv[])
{
    board_yoc_init();
    return example_spi(EXAMPLE_SPI_IDX);
}
