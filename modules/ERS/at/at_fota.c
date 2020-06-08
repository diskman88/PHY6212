/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <yoc_config.h>
#include <aos/log.h>
#include <aos/kv.h>
#include <yoc/fota.h>
#include <yoc/at_cmd.h>
#include <aos/version.h>
#include "at_internal.h"

#define TAG "at_fota"

static fota_t *g_fota_handle = NULL;
static int init_offset(void)
{
    int remainder;
    int offset;

    if (aos_kv_getint(FOTA_FW_OFFSET_RECORD, &offset) < 0) {
        offset = 0;
    }

    remainder = offset / CONFIG_FOTA_PIECE_SIZE;
    offset = remainder * CONFIG_FOTA_PIECE_SIZE;
    aos_kv_setint(FOTA_FW_OFFSET_RECORD, offset);
    return offset;
}

void at_cmd_otagetinfo(char *cmd, int type, char *data)
{
    int offset = 0;
    char buf[128];

    LOGD(TAG, "at_cmd_otagetinfo");
    offset = init_offset();

    snprintf(buf, 128, "\r\n%s\r\n%d\r\n%d\r\n%s\r\n" ,
                                    aos_get_app_version(),
                                    offset,
                                    CONFIG_FOTA_PIECE_SIZE,
                                    "OK");
    atserver_send(buf);
}

void at_cmd_otastart(char *cmd, int type, char *data)
{
    int ret;

    LOGD(TAG, "at_cmd_otastart");
    if (type == EXECUTE_CMD) {
        if (g_fota_handle == NULL) {
            fota_register_serial();
            netio_register_serial();
            netio_register_flash();

            g_fota_handle = fota_open("serial", "flash://misc", /*fota_event_cb*/NULL);
            if (NULL == g_fota_handle) {
                AT_BACK_ERR();
                return;
            }
            ret = aos_kv_getint("fota_cycle", &(g_fota_handle->sleep_time));
            if (ret != 0 || g_fota_handle->sleep_time < 100) {
                g_fota_handle->sleep_time = 100;
            }
            g_fota_handle->timeoutms = 3000;
            g_fota_handle->retry_count = 0;
            aos_sem_new(&g_fota_handle->sem, 0);
            g_fota_handle->status = FOTA_UNINIT;
        }
        init_offset();
        fota_start(g_fota_handle);

        AT_BACK_OK();
    } else {
        LOGE(TAG,"run %s %d", cmd, type);
    }
}

void at_cmd_otastop(char *cmd, int type, char *data)
{
    LOGD(TAG, "at_cmd_otastop");
    if (g_fota_handle) {
        fota_stop(g_fota_handle);
        AT_BACK_OK();
    } else {
        AT_BACK_ERR();
    }
}

void at_cmd_otafinish(char *cmd, int type, char *data)
{
    LOGD(TAG, "at_cmd_otafinish");
    if (g_fota_handle) {
        AT_BACK_OK();
        fota_finish(g_fota_handle);
    } else {
        AT_BACK_ERR();
    }
}

