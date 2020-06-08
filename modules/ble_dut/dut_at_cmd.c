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
 * @file     dut_test.c
 * @brief    DUT Test Source File
 * @version  V1.0
 * @date     10. Dec 2019
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 */
#include "log.h"
#include "clock.h"
#include "drv_usart.h"
#include "pinmux.h"
#include <yoc/partition.h>
#include <math.h>
#include <gpio.h>
#include <drv_wdt.h>
#include <aos/aos.h>
#include <aos/kernel.h>
#include <aos/ble.h>
#include "common.h"
#include "dtm_test.h"
#include "dut_rf_test.h"
#include "dut_uart_driver.h"
#include "dut_utility.h"
#include "rf_phy_driver.h"

/***************************************************************/
#define FREQ_FLASH_OFFSET  0x20
#define FREQ_LEN           4
#define FLASH_RSV_LEN           96


extern dut_uart_cfg_t g_dut_uart_cfg;
static volatile uint8_t rx_scan_flag = 0;
static partition_t handle = -1;
static hal_logic_partition_t *lp;

static int ewrite_flash(partition_t handle, const uint8_t *buffer, int length, int offset)
{
    lp = hal_flash_get_info(handle);

    if (partition_erase(handle, offset, (1 + length / lp->sector_size)) < 0) {
        printf("erase addr:%x\r\n", offset);
        return -1;
    }

    if (partition_write(handle, offset, (void *)buffer, length) >= 0) {
        return length;
    }

    printf("write fail addr:%x length:%x\r\n", offset, length);
    return -1;
}

void dut_at_cmd_init(void)
{
    rx_scan_flag = 0;
}

int dut_cmd_rx_mode(int argc, char *argv[])
{
    int sleep_time = 0;

    if (argc > 2) {
        return -1;
    }

    sleep_time = atoi(argv[1]);

    if ((sleep_time > 1800) || (sleep_time < 0)) {
        return -1;
    }

    if (sleep_time < 10) {
        sleep_time = 10;
    }

    enableSleepInPM(0xFF);

    rf_phy_dtm_init(NULL);

    rf_phy_set_dtmModeType(RF_PHY_DTM_MODE_RESET);

    rf_phy_dtm_trigged();

    rf_phy_set_dtmModeType(RF_PHY_DTM_MODE_RX_PER);
    rf_phy_set_dtmChan(0);
    rf_phy_set_dtmFreqOffSet(0);
    rf_phy_set_dtmLength(0x10);
    rf_phy_dtm_trigged();

    aos_msleep(sleep_time);

    rf_phy_dtm_stop();

    disableSleepInPM(0xFF);
    boot_wdt_close();
    dut_uart_init(&g_dut_uart_cfg);
    return OK;
}

int dut_cmd_ftest(int argc, char *argv[])
{
    // printf("cmd ftest ");
    return OK;
}

int dut_cmd_sleep(int argc, char *argv[])
{
    enableSleepInPM(0xFF);
    printf("OK\r\n");
    //for P16,P17
    subWriteReg(0x4000f01c, 6, 6, 0x00); //disable software control
    rf_phy_sleep();
    return OK;
}

int dut_cmd_ireboot(int argc, char *argv[])
{
    if (argc > 2) {
        return -1;
    }

    if (argc == 2) {
        if ((atoi(argv[1]) != 0) && (atoi(argv[1]) != 1)) {
            return -1;
        }
    }

    printf("OK\r\n");
    aos_reboot();
    return OK;
}

int  fdut_cmd_opt_mac(int argc, char *argv[])
{
    int ret;
    //int err;
    uint8_t addr[6];

    ret = partition_init();

    if (ret <= 0) {
        return ret;
    }

    handle = partition_open("FCDS");

    if (argc == 1) {
        partition_read(handle, 0, addr, sizeof(addr));
        printf("mac:%02x:%02x:%02x:%02x:%02x:%02x\r\n", addr[4], addr[5], addr[0], addr[1], addr[2], addr[3]);
    }

    return OK;
}

int  dut_cmd_freq_off(int argc, char *argv[])
{
    int ret;
    char  freq[FLASH_RSV_LEN];
    int16_t wfreq = 0;

    ret = partition_init();

    if (ret <= 0) {
        return ret;
    }

    handle = partition_open("FCDS");

    memset(freq, 0, sizeof(freq));
    partition_read(handle, 0, freq, sizeof(freq));

    if (argc == 1) {
        if (freq[FREQ_FLASH_OFFSET] == 0xff) {
            printf("Not Set Freq_Off\r\n");
            return -1;
        }

        printf("+FREQ_OFF=%d Khz\r\n", ((int8_t)(freq[FREQ_FLASH_OFFSET])) * 4);
    }

    if (argc == 2) {
        if ((strlen(argv[1]) > FREQ_LEN) && (int_num_check(argv[1]) == -1)) {
            return -1;
        }

        freq[FREQ_FLASH_OFFSET] = 0;
        wfreq = asciitohex((char *)(argv[1]));

        if (wfreq < -200) {
            wfreq = -200;
        } else if (wfreq > 200) {
            wfreq = 200;
        } else {}

        wfreq = wfreq - wfreq % 20;
        freq[FREQ_FLASH_OFFSET] = (char)(wfreq / 4);

        if (ewrite_flash(handle, (uint8_t *)(freq), sizeof(freq), 0) < 0) {
            return -1;
        }
    }

    partition_close(handle);
    return  OK;
}

int  fdut_cmd_freq_off(int argc, char *argv[])
{
    int ret;
    char  freq[FREQ_LEN];

    ret = partition_init();

    if (ret <= 0) {
        return ret;
    }

    handle = partition_open("FCDS");

    memset(freq, 0, sizeof(freq));
    partition_read(handle, FREQ_FLASH_OFFSET, freq, sizeof(freq));

    if (argc == 1) {
        printf("+FREQ_OFF=%d Khz\r\n", ((int8_t)(freq[0])) * 4);
    }

    partition_close(handle);

    return  0;
}

int  dut_cmd_opt_mac(int argc, char *argv[])
{
    int ret;
    int err;
    uint8_t addr[6];
    char  temp_data[FLASH_RSV_LEN];

    ret = partition_init();

    if (ret <= 0) {
        return ret;
    }

    handle = partition_open("FCDS");

    partition_read(handle, 0, temp_data, sizeof(temp_data));

    if (argc == 1) {
        printf("mac:%x:%x:%x:%x:%x:%x\r\n", temp_data[4], temp_data[5], temp_data[0], temp_data[1], temp_data[2], temp_data[3]);
    }

    if (argc == 2) {
        printf("%s\r\n", argv[1]);
        err = str2_char(argv[1], addr);

        if (err < 0) {
            return err;
        }

        if (err) {
            printf("Invalid address\n");
            return err;
        }

        temp_data[0] = addr[2];
        temp_data[1] = addr[3];
        temp_data[2] = addr[4];
        temp_data[3] = addr[5];
        temp_data[4] = addr[0];
        temp_data[5] = addr[1];

        if (ewrite_flash(handle, (uint8_t *)temp_data, sizeof(temp_data), 0) < 0) {
            return -1;
        }
    }

    partition_close(handle);

    return  0;
}

