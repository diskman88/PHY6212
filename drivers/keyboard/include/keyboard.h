#ifndef __KEY_BOARD_H
#define __KEY_BOARD_H
#include <gpio.h>
#include <drv_gpio.h>
#include <pinmux.h>
#include <aos/kernel.h>
#include <aos/log.h>
#include "drv_pmu.h"


#define KEY_SCAN_INTERVAL 80//ms
#define KEY_MAX_IDLE_TIME 20//s
#define KEY_SCAN_BUFFER_SZIE 20
#define KEY_SCAN_REPORT_THRESHOLD 1

typedef enum {
    RELEASE,
    PRESS,
} KEY_STATE;

typedef struct _key_io {
    GPIO_Pin_e pin;
    gpio_pin_handle_t handler;
} key_io;

typedef struct _key_map {
    uint8_t func;
    KEY_STATE state;
} key_map;

typedef struct _keyboard {
    uint8_t row_num;
    uint8_t column_num;
    key_io *key_row;
    key_io *key_column;
    key_map *map;
    uint8_t press_num;
} keyboard;

typedef enum {
    DATA_RECV,
} key_scan_event_e;

typedef void (*keyboard_cb)(key_scan_event_e event, uint8_t len);

int  keyboard_init(keyboard *key, keyboard_cb cb);
void keyboard_start_scan();
void keyboard_enable_standby();
void keyboard_disable_standby();
int  keyboard_get_ranks_data(uint8_t *data,uint8_t len);


#endif
