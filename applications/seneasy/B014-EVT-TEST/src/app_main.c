/*
 * Copyright (C) 2017 C-SKY Microsystems Co., All rights reserved.
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

#include <yoc_config.h>
#include <aos/log.h>
#include <aos/kernel.h>
#include <aos/ble.h>
#include <app_init.h>
#include <yoc/cli.h>
#include <aos/cli.h>
#include <devices/devicelist.h>
#include <stdint.h>
#include <stdbool.h>
#include "pm.h"

#include "ble_dut_test.h"

#include "pinmux.h"
#include "drv_gpio.h"
#include "drv_adc.h"

#include "drivers/ir_nec.h"

#define TAG  "APP"

static gpio_pin_handle_t led_w;
static gpio_pin_handle_t led_r;
static gpio_pin_handle_t button;
static gpio_pin_handle_t bat_vol_en;
static gpio_pin_handle_t boost_en;
static gpio_pin_handle_t sw_en;
static gpio_pin_handle_t ir_int1;
static gpio_pin_handle_t ir_int2;

void board_init()
{
    drv_pinmux_config(GPIO_P33, PIN_FUNC_GPIO);
    drv_pinmux_config(GPIO_P32, PIN_FUNC_GPIO);
    drv_pinmux_config(GPIO_P31, PIN_FUNC_GPIO);
    drv_pinmux_config(GPIO_P25, PIN_FUNC_GPIO);
    drv_pinmux_config(GPIO_P24, PIN_FUNC_GPIO);
    drv_pinmux_config(GPIO_P20, PIN_FUNC_GPIO);
    drv_pinmux_config(GPIO_P17, PIN_FUNC_GPIO);
    drv_pinmux_config(GPIO_P16, PIN_FUNC_GPIO);    


    led_w = csi_gpio_pin_initialize(GPIO_P20, NULL);
    led_r = csi_gpio_pin_initialize(GPIO_P24, NULL);
    button = csi_gpio_pin_initialize(GPIO_P25, NULL);
    bat_vol_en = csi_gpio_pin_initialize(GPIO_P33, NULL);
    boost_en = csi_gpio_pin_initialize(GPIO_P31, NULL);
    sw_en = csi_gpio_pin_initialize(GPIO_P32, NULL);
    ir_int1 = csi_gpio_pin_initialize(GPIO_P16, NULL);
    ir_int2 = csi_gpio_pin_initialize(GPIO_P17, NULL);

    csi_gpio_pin_config_direction(led_w, GPIO_DIRECTION_OUTPUT);
    csi_gpio_pin_config_mode(led_w, GPIO_MODE_PUSH_PULL);
    csi_gpio_pin_write(led_w, false);

    csi_gpio_pin_config_direction(led_r, GPIO_DIRECTION_OUTPUT);
    csi_gpio_pin_config_mode(led_r, GPIO_MODE_PUSH_PULL);
    csi_gpio_pin_write(led_r, false);

    csi_gpio_pin_config_direction(bat_vol_en, GPIO_DIRECTION_OUTPUT);
    csi_gpio_pin_config_mode(bat_vol_en, GPIO_MODE_PUSH_PULL);
    csi_gpio_pin_write(bat_vol_en, false);

    csi_gpio_pin_config_direction(boost_en, GPIO_DIRECTION_OUTPUT);
    csi_gpio_pin_config_mode(boost_en, GPIO_MODE_PUSH_PULL);
    csi_gpio_pin_write(boost_en, false);

    csi_gpio_pin_config_direction(sw_en, GPIO_DIRECTION_OUTPUT);
    csi_gpio_pin_config_mode(sw_en, GPIO_MODE_PUSH_PULL);
    csi_gpio_pin_write(sw_en, false);

    csi_gpio_pin_config_direction(button, GPIO_DIRECTION_INPUT);
    csi_gpio_pin_config_mode(button, GPIO_MODE_PULLUP);

    csi_gpio_pin_config_direction(ir_int1, GPIO_DIRECTION_INPUT);
    csi_gpio_pin_config_mode(ir_int1, GPIO_MODE_PULLUP);

    csi_gpio_pin_config_direction(ir_int2, GPIO_DIRECTION_INPUT);
    csi_gpio_pin_config_mode(ir_int2, GPIO_MODE_PULLUP);
}

static void cmd_set_led(char *wbuf, int wbuf_len, int argc, char **argv)
{
    int led;
    bool on_off;

    // printf(">:%s, argc=%d, arg1=%s, arg2=%s\r\n", argv[0], argc, argv[1], argv[2]);

    led = -1;
    if (argc == 3){
        if (strcmp(argv[1], "white") == 0) {
            led = 0;
        } else if (strcmp(argv[1], "red") == 0) {
            led = 1;
        } else {
            led = -1;
        }

        if (strcmp(argv[2], "off") == 0) {
            on_off = false;
        } else if (strcmp(argv[2], "on") == 0) {
            on_off = true;
        } else {
            on_off = false;
        }
    }

    if (led != -1) {
        if (led == 0) {
            csi_gpio_pin_write(led_w, on_off);
            // printf("set led white %s\r\n", (on_off) ? "on":"off");
        }

        if (led == 1) {
            csi_gpio_pin_write(led_r, on_off);
            // printf("set led red %s\r\n", (on_off) ? "on":"off");
        }
    }
}

static void cmd_boost_en(char *wbuf, int wbuf_len, int argc, char **argv)
{
    if (strcmp(argv[1], "on") == 0) {
        csi_gpio_pin_write(boost_en, true);
    } else if (strcmp(argv[1], "off") == 0) {
        csi_gpio_pin_write(boost_en, false);
    } else {
        csi_gpio_pin_write(boost_en, false);
    }

}

static void cmd_send_ir_nec(char *wbuf, int wbuf_len, int argc, char **argv)
{
    ir_nec_start_send(0xAA, 0x01);
    ir_nec_stop_send();
}

static void cmd_read_button(char *wbuf, int wbuf_len, int argc, char **argv)
{

}

int read_bat_voltage();

static void cmd_read_bat_voltage(char *wbuf, int wbuf_len, int argc, char **argv)
{
    read_bat_voltage();
}
static void cmd_recv_ir_nec(char *wbuf, int wbuf_len, int argc, char **argv)
{

}
static void cmd_enter_dut_test(char *wbuf, int wbuf_len, int argc, char **argv)
{
    ble_dut_start();
}

static const struct cli_command cmds[] =  
{   
    /**
     * @brief 1.LED TEST
     *  set_led <led> <on|off>
     * 
     */
    {
        .name = "set_led",
        .help = "set_led <led>(0-white, 1-red) <on|off>(0-off,1-on)",
        .function = cmd_set_led,
    },
    /**
     * @brief 2.boost_en
     *  boost_en <on|off>(0-off,1-on)
     */
    {
        .name = "boost_en",
        .help = "boost_en <on|off>(0-off,1-on)",
        .function = cmd_boost_en,
    },
    /**
     * @brief 3.send_ir_nec
     *  send_ir_nec <uint8>(custom code) <uint8>(key code)
     */
    {
        .name = "send_ir_nec",
        .help = "send_ir_nec <uint8>(custom code) <uint8>(key code)",
        .function = cmd_send_ir_nec,
    },
    /**
     * @brief 4.Button Test
     *  read_button
     * return: button=<0|1>
     */
    {
        .name = "read_button",
        .help = "read_button",
        .function = cmd_read_button,
    },
    /**
     * @brief 5.Battery Voltage Test
     *  read_bat_voltage
     * return: voltage=<uint8>
     */
    {
        .name = "read_bat_voltage",
        .help = "read_bat_voltage",
        .function = cmd_read_bat_voltage,
    },
    /**
     * @brief 6.recv_ir_nec
     *  recv_ir_nec
     * return: <ok|failed>
     */
    {
        .name = "recv_ir_nec",
        .help = "recv_ir_nec",
        .function = cmd_recv_ir_nec,
    },
    /**
     * @brief 7.enter_dut_test
     *  enter_dut_test
     */
    {
        .name = "enter_dut_test",
        .help = "enter_dut_test",
        .function = cmd_enter_dut_test,
    }
};

int read_bat_voltage(void)
{
    adc_conf_t sconfig;
    adc_handle_t hd;
    uint32_t data[16];
    uint32_t ch_array[1] = {7};
    int i = 0;
    int ret = 0;

    sconfig.mode = ADC_CONTINUOUS;
    sconfig.trigger = 0;
    sconfig.intrp_mode = 0;
    sconfig.channel_array = ch_array;
    sconfig.channel_nbr = 1;
    sconfig.conv_cnt = 16;
    sconfig.enable_link_internal_voltage = 0;

    drv_pinmux_config(GPIO_P15, ADCC);

    for (i = 0; i < 16; i++) {
        data[i] = 0x31415;
    }

    hd = drv_adc_initialize(0, NULL);

    if (!hd) {
        printf("adc initial failed\n\r");
        return -1;
    }

    ret = drv_adc_config(hd, &sconfig);

    if (ret == (CSI_DRV_ERRNO_ADC_BASE | DRV_ERROR_UNSUPPORTED)) {
        printf("continous mode unsupported\n");
        return -1;
    }

    if (ret != 0) {
        printf("adc config failed, %d\n\r", ret);
        return -1;
    }

    if (drv_adc_start(hd) != 0) {
        printf("adc start failed\n\r");
        return -1;
    }

    ret = drv_adc_read(hd, &data[0], 16);


    if (drv_adc_stop(hd) != 0) {
        printf("adc stop failed\n");
        return -1;
    }

    // if (drv_adc_uninitialize(hd) != 0) {
    //     printf("adc uninitial failed\n");
    //     return -1;
    // }

    uint32_t sum;
    sum = 0; 
    for (i = 0; i < 16; i++) {
        sum += data[i];
    }        
    sum = sum / 16;
    return sum;
}



int board_sleep()
{
    enableSleepInPM(0xFF);
    //for P16,P17
    subWriteReg(0x4000f01c, 6, 6, 0x00); //disable software control
    rf_phy_sleep();
}


#define TEST_FLAG_ERR_IR_RECV   0x80000


int app_main(int argc, char *argv[])
{
    uart_csky_register(0);

    spiflash_csky_register(0);

    board_yoc_init();

    board_init();

    bool is_key_down;
    csi_gpio_pin_read(button, &is_key_down);
    if (is_key_down == false) {
        ble_dut_start();
    }


    uint32_t test_flag = 0;
    /**
     * @brief LED亮灭指示: 
     *  1. 3.3V升压启动；
     *  2. LED white on 1S；
     *  3. LED red on 1s
     */
    uint16_t bat_vol1, bat_vol2;

    printf("\r\n=======================\r\n");
    printf("B014 test start\r\n\r\n");

    printf("1.led test....\r\n");
    csi_gpio_pin_write(boost_en, true);

    csi_gpio_pin_write(led_w, true);
    csi_gpio_pin_write(bat_vol_en, true);
    aos_msleep(100);
    bat_vol1 = read_bat_voltage();
    aos_msleep(900);
    csi_gpio_pin_write(led_w, false);
    aos_msleep(500);

    csi_gpio_pin_write(led_r, true);
    csi_gpio_pin_write(bat_vol_en, false);
    aos_msleep(100);
    bat_vol2 = read_bat_voltage();
    aos_msleep(900);
    csi_gpio_pin_write(led_r, false);
    aos_msleep(500);

    /**
     * @brief 判断电池电压读取是否正常
     * 
     */
    printf("2.battery voltage: vol1 = %dmV, vol2 = %dmV\r\n", bat_vol1, bat_vol2);
    
    /**
     * @brief 红外发码测试
     * 
     */
    printf("3.ir test, press the button to start\r\n");
    int cnt = 0;
    while (true)
    {
        csi_gpio_pin_read(button, &is_key_down);
        if (is_key_down == false) {
            aos_msleep(20);
            csi_gpio_pin_read(button, &is_key_down);
            if (is_key_down == false) {
                csi_gpio_pin_write(led_r, true);
                ir_nec_start_send(0xAA, 0x01);
                ir_nec_stop_send();
                while(ir_test_is_busy());
                csi_gpio_pin_write(led_r, false);
                cnt++;
                if (cnt >= 3) {
                    break;
                }

                while(is_key_down == false) {
                    csi_gpio_pin_read(button, &is_key_down);
                }
            }
        }
    }

    /**
     * @brief 红外接受测试
     * 
     */
    aos_msleep(108);
    bool is_ir_sig1, is_ir_sig2;
    ir_test_carry_on();
    aos_msleep(5);

    csi_gpio_pin_read(ir_int1, &is_ir_sig1);
    csi_gpio_pin_read(ir_int2, &is_ir_sig2);
    if (is_ir_sig1 == false || is_ir_sig2 == false) {
        test_flag |= TEST_FLAG_ERR_IR_RECV;
    }

    ir_test_carry_off();
    aos_msleep(5);
    csi_gpio_pin_read(ir_int1, &is_ir_sig1);
    csi_gpio_pin_read(ir_int2, &is_ir_sig2);
    if (is_ir_sig1 == true || is_ir_sig2 == true) {
        test_flag |= TEST_FLAG_ERR_IR_RECV;
    }

    if (test_flag != 0) {
        printf("\r\nperiphers test failed, flags = %4X\r\n", test_flag);
        while (1) {
            csi_gpio_pin_write(led_r, true);
            aos_msleep(10);
            csi_gpio_pin_write(led_r, false);
            aos_msleep(20);
        }
        
    } else {
        printf("4.enter sleep mode......\r\n");
        printf("\r\nperiphers test success\r\n");
        printf("=======================\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n");

        csi_gpio_pin_write(led_r, false);
        csi_gpio_pin_write(led_w, false);
        csi_gpio_pin_write(boost_en, false);
        csi_gpio_pin_write(sw_en, false);
        csi_gpio_pin_write(bat_vol_en, false);
        board_sleep();      
    }

    // cli_service_reg_cmds(cmds, ARRAY_SIZES(cmds));

    // ble_dut_start();
    return 0;
}
