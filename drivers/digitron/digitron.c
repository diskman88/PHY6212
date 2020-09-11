/******************************************************************************
 * @file     app_main.c
 * @brief    the main function for the digitron driver
 * @version  V1.0
 * @date     07. June 2020
 ******************************************************************************/
#include <stdio.h>
#include <aos/kernel.h>
#include "dw_gpio.h"
#include "drv_gpio.h"
#include "pin_name.h"
#include "pinmux.h"

#define MAX_IO      7
#define DATA_NULL_IDX 11

gpio_pin_handle_t pin[MAX_IO] = {0};
static uint8_t dig_init = 0;
static uint8_t show_flag = 1;

extern void udelay(uint32_t us);
struct dig_item {
    uint8_t h;
    uint8_t l;
};
static struct dig_item dig_map[32] = {
//   A     B     C     D     E     F     G     DP
    {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {2, 1}, {1, 4},
    {3, 2}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6}, {1, 2}, {5, 1},
    {0, 1}, {4, 2}, {5, 2}, {6, 2}, {4, 3}, {5, 3}, {6, 3}, {0xff, 0xff},
    {2, 3}, {2, 4}, {2, 5}, {2, 6}, {3, 4}, {3, 5}, {3, 6}, {0xff, 0xff},
};

static uint8_t map_table[12] = {
// 	 0     1     2     3     4     5     6     7     8      9       NULL
    0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0xff,0x00
};


static int init_digitron_pin(uint8_t *io,uint8_t size)
{
    for (int i = 0; i < size; i++) {
        drv_pinmux_config(io[i], 99);
        pin[i] = csi_gpio_pin_initialize(io[i], NULL);

        int ret = csi_gpio_pin_config_direction(pin[i], GPIO_DIRECTION_INPUT);

        if (ret != 0) {
            printf("%s, %d, error\n", __func__, __LINE__);
        }

        ret = csi_gpio_pin_config_mode(pin[i], GPIO_MODE_PULLNONE);

        if (ret != 0) {
            printf("%s, %d, error\n", __func__, __LINE__);
            return ret;
        }
    }
    return 0;
}

static void set_pin_floating(int p)
{
    int ret = csi_gpio_pin_config_direction(pin[p], GPIO_DIRECTION_INPUT);

    if (ret != 0) {
        printf("%s, %d, error\n", __func__, __LINE__);
    }

    ret = csi_gpio_pin_config_mode(pin[p], GPIO_MODE_PULLNONE);

    if (ret != 0) {
        printf("%s, %d, error\n", __func__, __LINE__);
    }
}

static void set_pin_high(int p)
{
    int ret = csi_gpio_pin_config_direction(pin[p], GPIO_DIRECTION_OUTPUT);

    if (ret != 0) {
        printf("%s, %d, error\n", __func__, __LINE__);
    }

    csi_gpio_pin_write(pin[p], 1);
}

static void set_pin_low(int p)
{
    int ret = csi_gpio_pin_config_direction(pin[p], GPIO_DIRECTION_OUTPUT);

    if (ret != 0) {
        printf("%s, %d, error\n", __func__, __LINE__);
    }

    csi_gpio_pin_write(pin[p], 0);
}


void digitron_update(uint8_t data[])
{
    uint8_t i = 0;

    uint32_t led_status = ((map_table[data[0]] << 24) | (map_table[data[1]] << 16) | (map_table[data[2]] << 8) | map_table[data[3]]);
    for (i = 0; i < 32; i++) {
        struct dig_item item = dig_map[i];

        if (item.h == 0xff) {
            continue;
        }

        if (led_status & (1 << i)) {
            set_pin_high(item.h);
            set_pin_low(item.l);
            aos_msleep(1);
            //udelay(1000);
            set_pin_floating(item.l);
            set_pin_floating(item.h);
        }
    }
}

int digitron_init(uint8_t *digitron_io_list, uint8_t digitron_io_num)
{
    int ret = 0;
    if(!digitron_io_list || digitron_io_num != MAX_IO) {
        return -1;
    }
    ret = init_digitron_pin(digitron_io_list,digitron_io_num);
    if(ret) {
        return -1;
    }
    dig_init = 1;
    return 0;
}


int digitron_show(uint32_t data)
{
    if(!dig_init) {
        return -1;
    }
    show_flag = 1;
    uint8_t data_show[4] = {DATA_NULL_IDX,DATA_NULL_IDX,DATA_NULL_IDX,DATA_NULL_IDX};

    if(data) {
        data_show[3] = (data / 1000) % 10;
        data_show[2] = (data / 100) % 10;
        data_show[1] = (data / 10) % 10;
        data_show[0] = data % 10;

        for(int i = 3 ; i >= 0 ; i--) {
            if(data_show[i] == 0) {
                data_show[i] = DATA_NULL_IDX;
            } else {
                break;
            }
        }
    }else {
        data_show[0] = 0;
    }

    while(show_flag) {
        digitron_update(data_show);
    }

    return 0;
}

void digitron_show_off()
{
    show_flag = 0;
}



