/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <assert.h>
#include <zephyr.h>

#include <aos/kv.h>
#include <errno.h>
#include "settings/settings.h"
#include "settings/settings_kv.h"
#include "settings_priv.h"
#include <aos/log.h>
#define BT_DBG_ENABLED 0
#include "common/log.h"

static int settings_kv_load(struct settings_store *cs, load_cb cb,
                            void *cb_arg);
static int settings_kv_save(struct settings_store *cs, const char *name,
                            const char *value);

static struct settings_store_itf settings_kv_itf = {
    .csi_load = settings_kv_load,
    .csi_save = settings_kv_save,
};

int settings_kv_src(struct settings_kv *cf)
{
    if (!cf->name) {
        return -EINVAL;
    }

    cf->cf_store.cs_itf = &settings_kv_itf;
    settings_src_register(&cf->cf_store);

    return 0;
}

int settings_kv_dst(struct settings_kv *cf)
{
    if (!cf->name) {
        return -EINVAL;
    }

    cf->cf_store.cs_itf = &settings_kv_itf;
    settings_dst_register(&cf->cf_store);

    return 0;
}

struct settings_kv_func_t {
    load_cb func;
    void *arg;
};

static void _settings_kv_load(char *key, char *val, uint16_t val_size, void *arg)
{
    char val_buf[SETTINGS_MAX_VAL_LEN] = {0};
    char name_buf[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN] = {0};
    const char *kv_prefix = "bt/";

    struct settings_kv_func_t *func = arg;
    if (0 == strncmp(kv_prefix, key, 3)) {
        if (func && func->func) {
            int len = MIN(SETTINGS_MAX_VAL_LEN -1,val_size);
            memcpy(val_buf, val, len);
            val_buf[len] = '\0';
            len = MIN(SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN - 1,strlen(key));
            memcpy(name_buf, key, len);
            name_buf[len] = '\0';
            BT_DBG("load %s=%s",name_buf,val_buf);
            func->func(name_buf, val_buf, func->arg);
        }
    }
}

/*
 * Called to load configuration items. cb must be called for every configuration
 * item found.
 */
static int settings_kv_load(struct settings_store *cs, load_cb cb,
                            void *cb_arg)
{
    struct settings_kv_func_t arg;
    arg.func = cb;
    arg.arg = cb_arg;

    extern void __kv_foreach(void (*func)(char *key, char *val, uint16_t val_size, void *arg), void *arg);
    __kv_foreach(_settings_kv_load, &arg);

    return 0;
}

/*
 * Called to save configuration.
 */
static int settings_kv_save(struct settings_store *cs, const char *name,
                            const char *value)
{
    char val_buf[SETTINGS_MAX_VAL_LEN] = {0};
    int rc;

    if (!name) {
        return -EINVAL;
    }

    rc = aos_kv_getstring(name, val_buf, sizeof(val_buf));

    //add new key
    if (rc < 0) {
        if (value != NULL) {
            rc = aos_kv_setstring(name, value);

            if (rc < 0) {
                BT_ERR("kv setting set value err %d", rc);
                return -EIO;
            }

            BT_DBG("add new key %s,%s", name, value);
        }
        return 0;
    }

    //delete key
    if (value == NULL) {
        rc = aos_kv_del(name);
        if (rc < 0) {
            BT_ERR("kv setting del value err %d", rc);
            return -EIO;
        }
        return 0;
    }

    if (0== strncmp(val_buf, value, sizeof(val_buf))) {
        BT_DBG("same key %s,%s", name, value);
        return 0;
    }

    rc = aos_kv_setstring(name, value);

    if (rc < 0) {
        BT_ERR("update key fail %s,%s", name, value);
        return -EIO;
    }

    BT_DBG("setting update %s, %s", name, value);

    return 0;
}
