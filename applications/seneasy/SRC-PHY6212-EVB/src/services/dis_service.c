
#include "dis_service.h"

static pnp_id_t pnp_id = {
    VEND_ID_SOURCE_USB,
    0x1915,
    0xEEEE,
    0x0001,
};

static dis_info_t dis_info = {
    MANUFACTURER_NAME,
    MODEL_NUMBER,
    SERIAL_NUMBER,
    HW_REV,
    FW_REV,
    SW_REV,
    NULL,
    NULL,
    &pnp_id,
};

static dis_handle_t g_dis_handle = NULL;

int dis_service_init()
{
    g_dis_handle = dis_init(&dis_info);

    if (g_dis_handle == NULL) {
        LOGE("DIS", "DIS init FAIL!!!!");
        return -1;
    }

    return 0;
}