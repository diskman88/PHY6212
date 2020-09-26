#ifndef __APP_INIT_H
#define __APP_INIT_H

void board_yoc_init(void);

enum {
    ON,
    OFF,
    BLINK_FAST,
    BLINK_SLOW,
};

void led_set_status(int status);

#endif
