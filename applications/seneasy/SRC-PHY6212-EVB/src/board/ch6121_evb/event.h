#include <aos/kernel.h>
#include <aos/types.h>
#include <aos/log.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    APP_EVENT_IO        = 0x00000001,
    APP_EVENT_BT        = 0x00000002,
    APP_EVENT_WAKEUP    = 0x00000004,
    APP_EVENT_KEYSCAN   = 0x00000008,
    APP_EVENT_ERR       = 0x80000000
}app_event_t;

int32_t app_event_init();

int32_t app_event_set(app_event_t event);

int32_t app_event_get(uint32_t require_flags, uint32_t *actl_flags, uint32_t timeout);