#ifndef __TRCTYPES_H__
#define __TRCTYPES_H__

#include <stdint.h>
#include <trcConfig.h>

typedef uint16_t traceLabel;
typedef uint16_t trace_label_t;

typedef uint8_t UserEventChannel;
typedef uint8_t user_event_channel_t;

#if (USE_16BIT_OBJECT_HANDLES == 1)
typedef uint16_t object_handle_t;
#else
typedef uint8_t object_handle_t;
#endif

typedef uint8_t traceObjectClass;
typedef uint8_t trace_object_class_t;

#endif
