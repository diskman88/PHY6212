/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include <string.h>
#include <aos/list.h>
#include <aos/kernel.h>
#include <aos/kv.h>
#include <yoc/netio.h>
#include <yoc/fota.h>
#include <aos/log.h>

#define TAG "fota"

#ifndef CONFIG_FOTA_STACK_SIZE
/* since printf need more stack size, if CONFIG_DEBUG enabled, ch6121 will not compile successfully */
#define CONFIG_FOTA_STACK_SIZE 1024
#endif
static uint32_t fota_task_stack[CONFIG_FOTA_STACK_SIZE / 4] __attribute__((section(".data")));

typedef struct fota_netio_list {
    slist_t next;
    const fota_cls_t *cls;
} fota_cls_node_t;

static AOS_SLIST_HEAD(fota_cls_list);
static fota_info_t fota_info = {NULL, FOTA_STATE_STOP};

int fota_register(const fota_cls_t *cls)
{
    if (cls == NULL ||
        cls->init == NULL ||
        cls->name == NULL ||
        cls->fail == NULL ||
        cls->version_check == NULL ||
        cls->finish == NULL) {
        return -1;
    }

    fota_cls_node_t *node = malloc(sizeof(fota_cls_node_t));

    if (node) {
        node->cls = cls;
        slist_add_tail(&node->next, &fota_cls_list);
        return 0;
    }

    return -1;
}

fota_t *fota_open(const char *fota_name, const char *dst, fota_event_cb_t event_cb)
{
    fota_cls_node_t *node;
    fota_t *fota = NULL;
    if (fota_name == NULL || dst == NULL) {
        return NULL;
    }

    slist_for_each_entry(&fota_cls_list, node, fota_cls_node_t, next) {
        if (strcmp(node->cls->name, fota_name) == 0) {
            fota = aos_zalloc_check(sizeof(fota_t));
            fota->to_path = strdup(dst);
            fota->cls = node->cls;
            fota->event_cb = event_cb;
            if (fota->cls->init)
                fota->cls->init();
            //LOGD(TAG, "fota open: %x path:%s", fota, fota->to_path);
            break;
        }
    }

    return fota;
}

static int fota_version_check(fota_t *fota, fota_info_t *info) {
    if (fota == NULL || info == NULL) {
        return -1;
    }

    if (fota->cls->version_check)
        return fota->cls->version_check(info);

    return -1;
}

static int fota_upgrade(fota_t *fota)
{
    if (fota == NULL || fota->running || fota->status == FOTA_DOWNLOAD) //?
        return -1;

    fota->quit = 0;
    fota->buffer = aos_malloc(CONFIG_FOTA_BUFFER_SIZE);
    fota->from = netio_open(fota_info.fota_url);
    fota->to = netio_open(fota->to_path);

    if (fota->buffer == NULL || fota->from == NULL || fota->to == NULL) {
        if (fota->buffer == NULL) {
            LOGE(TAG, "fota->buffer e");
        } else if (fota->from == NULL) {
            LOGE(TAG, "fota->from e");
        } else if (fota->to == NULL) {
            LOGE(TAG, "fota->to e");
        }
        goto error;
    }

    if (aos_kv_getint(FOTA_FW_OFFSET_RECORD, &fota->offset) < 0) {
        aos_kv_setint(FOTA_FW_OFFSET_RECORD, 0);
        fota->offset = 0;
    }

    LOGD(TAG, "FOTA seek %d", fota->offset);

    if (netio_seek(fota->from, fota->offset, SEEK_SET) != 0) {
        LOGE(TAG, "from seek error");
        goto error;
    }

    if (netio_seek(fota->to, fota->offset, SEEK_SET) != 0) {
        LOGE(TAG, "to seek error");
        goto error;
    }

    fota->status = FOTA_DOWNLOAD;
    return 0;

error:
    if (fota->buffer) {
        aos_free(fota->buffer);
        fota->buffer = NULL;
    }
    if (fota->from) {
        netio_close(fota->from);
        fota->from = NULL;
    }
    if (fota->to) {
        netio_close(fota->to);
        fota->to = NULL;
    }

    return -1;
}

static void fota_task(void *arg)
{
    fota_t *fota = (fota_t *)arg;
    int retry;

    if (arg == NULL) {
        return;
    }

    retry = fota->retry_count;
    fota->quit = 0;
    fota_info.state = FOTA_STATE_RUNNING;
    // LOGD(TAG, "fota_task start: %s", fota->to_path);
    while (!fota->quit) {
        if (fota->status == FOTA_INIT) {
            LOGD(TAG, "fota_task FOTA_INIT!");
            if (fota_version_check(fota, &fota_info) == 0) {
                if (fota->event_cb && fota->event_cb(arg, FOTA_EVENT_VERSION) != 0) {
                    LOGD(TAG, "fota_upgrade 1!");
                } else {
                    LOGD(TAG, "fota_upgrade 2!");
                    if (fota_upgrade(fota) < 0) {
                        LOGE(TAG, "fota_upgrade fail");
                        aos_msleep(1000);
                        if (--retry < 0) {
                            fota->quit = 1;
                        }
                    } else {
                        if (fota->running == 0) {
                            fota->running = 1;
                        }
                    }
                }
                continue;
            }
            aos_msleep(1000);
        } else if (fota->status == FOTA_DOWNLOAD) {
            LOGD(TAG, "fota_task FOTA_DOWNLOAD! f:%d t:%d", fota->from->size, fota->to->offset);
            int size = netio_read(fota->from, fota->buffer, CONFIG_FOTA_BUFFER_SIZE, 3000);
            LOGD(TAG, "read: %d", size);
            if (size < 0) {
                // LOGD(TAG, "read size < 0 %d", size);
                if (fota->event_cb && fota->event_cb(arg, FOTA_EVENT_FAIL) != 0) {
                    // fota->status = FOTA_STOP;
                } else {
                    fota->status = FOTA_STOP;
                }
                LOGE(TAG, "FOTA_STOP!");
                continue;
            } else if (size == 0) { // finish
                if (fota->event_cb && fota->event_cb(arg, FOTA_EVENT_FINISH) != 0) {
                    LOGE(TAG, "FOTA_FINISH 1!");
                } else {
                    LOGE(TAG, "FOTA_FINISH 2!");
                    fota->status = FOTA_FINISH;
                    fota_finish(fota);
                    /* wait for reboot */
                    continue;
                }
            }

            size = netio_write(fota->to, fota->buffer, size, -1);
            // TODO: verify data.
            //LOGI(TAG, "write: %d", size);
            if (size > 0) {
                fota->offset += size;
                aos_kv_setint(FOTA_FW_OFFSET_RECORD, fota->offset);
                retry = fota->retry_count;
            } else { // flash write error
                if (fota->event_cb && fota->event_cb(arg, FOTA_EVENT_FAIL) != 0) {
                    LOGE(TAG, "FOTA_STOP 3!");
                } else {
                    LOGE(TAG, "write size < 0 %d", size);
                    fota->status = FOTA_STOP;
                }
            }
        } else if (fota->status == FOTA_STOP) {
            if (retry > 0) {
                LOGW(TAG, "fota retry: %x!", retry);
                aos_msleep(fota->sleep_time);
                retry--;
                fota->status = FOTA_DOWNLOAD;
            } else {
                LOGW(TAG, "exit fota_task!");
                break;
            }
        }
    }

    fota->running = 0;
    fota->status = FOTA_UNINIT;
    fota_info.state = FOTA_STATE_STOP;
    aos_sem_signal(&fota->sem);
    fota_fail(fota);
}

int fota_start(fota_t *fota)
{
    if (fota == NULL) {
        return -1;
    }

    if (fota->status == FOTA_UNINIT) {
        fota->status = FOTA_INIT;

        return aos_task_new_ext(&fota->task, "fota", fota_task, fota, fota_task_stack, CONFIG_FOTA_STACK_SIZE, AOS_DEFAULT_APP_PRI + 13);
    }

    return 0;
}

int fota_stop(fota_t *fota)
{
    if (fota == NULL) {
        return -1;
    }

    fota->quit = 1;

    return 0;
}

int fota_finish(fota_t *fota)
{
    if (fota == NULL) {
        return -1;
    }

    fota_info.state = FOTA_STATE_FINISH;

    if (fota->cls->finish)
        fota->cls->finish();
    return 0;
}

int fota_fail(fota_t *fota)
{
    if (fota == NULL) {
        return -1;
    }

    if (fota->cls->fail)
        fota->cls->fail();

    if (fota->from_path) {
        aos_free(fota->from_path);
        fota->from_path = NULL;
    }

    if (fota->buffer) {
        aos_free(fota->buffer);
        fota->buffer = NULL;
    }

    if (fota->private) {
        aos_free(fota->private);
        fota->private = NULL;
    }

    if (fota->from) {
        netio_close(fota->from);
        fota->from = NULL;
    }

    if (fota->to) {
        netio_close(fota->to);
        fota->to = NULL;
    }
    fota_info.state = FOTA_STATE_STOP;
    return 0;
}

int fota_close(fota_t *fota) {
    if (fota == NULL) {
        return -1;
    }

    aos_sem_free(&fota->sem);

    if (fota->from_path) aos_free(fota->from_path);
    if (fota->to_path) aos_free(fota->to_path);
    if (fota->buffer) aos_free(fota->buffer);
    if (fota->private) aos_free(fota->private);
    if (fota->from) netio_close(fota->from);
    if (fota->to) netio_close(fota->to);

    aos_free(fota);
    fota_info.state = FOTA_STATE_STOP;

    return 0;
}

fota_state_e fota_state(void)
{
    return fota_info.state;
}

// demo

#if 0
static int fota_event_cb(void *arg, fota_event_e event) //return 0: still do the default handle      not zero: only do the user handle
{
    fota_t *fota = (fota_t *)arg;
    switch (event) {
        case FOTA_EVENT_START:
            LOGD(TAG, "FOTA START :%x", fota->status);
            break;

        case FOTA_EVENT_FAIL:
            LOGD(TAG, "FOTA FAIL :%x", fota->status);
            break;

        case FOTA_EVENT_FINISH:
            LOGD(TAG, "FOTA FINISH :%x", fota->status);
            break;

        default:
            break;
    }
    return 0;
}

int event_cb(void *arg, fota_event_e event){
    fota_t *fota = (fota_t *)arg;
    switch (event) {
        case FOTA_EVENT_START:
            fota_upgrade(fota);
            fota_stop(fota);

            return 0;

        case FOTA_EVENT_FAIL:
            aos_msleep(3000);
            fota_start(fota);
            break;

        case FOTA_EVENT_FINISH:
            aos_reboot();

        default:
            return 0;
    }
}

#endif
