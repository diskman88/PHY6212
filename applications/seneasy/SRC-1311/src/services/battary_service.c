#include "battary_service.h"

static bas_handle_t g_bas_handle = NULL;
static bas_t g_bas;

int battary_service_init()
{
    g_bas_handle = bas_init(&g_bas);

    if (g_bas_handle == NULL) {
        LOGE("BAT", "BAS init FAIL!!!!");
        return -1;
    }

    return 0;
}

