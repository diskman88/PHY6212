
#include "event.h"

static aos_event_t app_event_flags;

int32_t app_event_init()
{
    return aos_event_new(&app_event_flags, 0);
}

int32_t app_event_set(app_event_t event)
{
    return aos_event_set(&app_event_flags, event, AOS_EVENT_OR);
}

int32_t app_event_get(uint32_t require_flags, uint32_t *actl_flags, uint32_t timeout)
{
    return aos_event_get(&app_event_flags, require_flags, AOS_EVENT_OR_CLEAR, actl_flags, timeout);
}