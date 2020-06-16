
#ifndef _KEYSCAN_H
#define _KEYSCAN_H

#include <stdint.h>
#include <stdbool.h>

#include "../board/ch6121_evb/msg.h"
#include "../board/ch6121_evb/event.h"

enum {
    MSG_KSCAN_DOWN,
    MSG_KSCAN_RELEASE,
}MSG_KEYSCAN;

enum {
    MSG_EVENT_ONE_KEY,
    MSG_EVENT_HOLD_KEY,
    MSG_EVENT_COMBO_KEY
}EVENT_KEYSCAN;

#endif
