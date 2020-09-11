/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include <yoc/netio.h>
#include <yoc/fota.h>
#include <yoc/network.h>
#include <aos/kv.h>
#include <aos/version.h>
#include <yoc/sysinfo.h>
#include "../http/http.h"
#include <aos/log.h>

#define TAG "fota_serial"

static int serial_init(void) {
    return 0;
}

static int version_check(fota_info_t *info) {
    if (info == NULL) {
        return -1;
    }

    if (info->fota_url) {
        free(info->fota_url);
        info->fota_url = NULL;
    }
    info->fota_url = strdup("serial://xxx");
    LOGD(TAG, "get url: %s", info->fota_url);
    return 0;
}
static int serial_finish(void)
{
    aos_kv_del(FOTA_FW_OFFSET_RECORD);
    printf("OTA Reboot now!\n");
    aos_reboot();
    return 0;
}

static int serial_fail(void) {
    return 0;
}

const fota_cls_t fota_serial_cls = {
    "serial",
    serial_init,
    version_check,
    serial_finish,
    serial_fail,
};

int fota_register_serial(void)
{
    return fota_register(&fota_serial_cls);
}
