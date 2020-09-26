/*
 * Copyright (C) 2018 C-SKY Microsystems Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __FOTA_H__
#define __FOTA_H__

#include <yoc_config.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <aos/list.h>
#include <yoc/netio.h>

#define BUFFER_SIZE 2048
#define FW_URL_KEY  "FW_URL_KEY"

#ifndef CONFIG_FOTA_PIECE_SIZE
#define CONFIG_FOTA_PIECE_SIZE 128
#endif

#ifndef CONFIG_FOTA_BUFFER_SIZE
#define CONFIG_FOTA_BUFFER_SIZE CONFIG_FOTA_PIECE_SIZE + 128
#endif

#define FOTA_FW_OFFSET_RECORD  "fota_offset"

typedef struct fota_config {
    int timeout_ms;
    int retry_time;
} fota_config_t ;

typedef enum fota_event {
    FOTA_EVENT_VERSION = 0,
    FOTA_EVENT_START = 1,
    FOTA_EVENT_FAIL = 2,
    FOTA_EVENT_FINISH = 3,
} fota_event_e;

typedef enum fota_status {
    FOTA_UNINIT = 0,
    FOTA_INIT = 1,
    FOTA_DOWNLOAD = 2,
    FOTA_STOP = 3,
    FOTA_FINISH = 4,
} fota_status_e;

typedef enum fota_state {
    FOTA_STATE_STOP = 0,
    FOTA_STATE_RUNNING,
    FOTA_STATE_FINISH
} fota_state_e;

typedef struct fota_data {
    char *url;
    void *data;
}fota_data_t;

typedef struct {
    char *fota_url;
    fota_state_e state;
} fota_info_t;

typedef struct fota_cls {
    const char *name;
    int (*init)(void);
    int (*version_check)(fota_info_t *info);
    int (*finish)(void);
    int (*fail)(void);
} fota_cls_t;

typedef int (*fota_event_cb_t)(void *fota, fota_event_e event);   ///< fota Event call back.

typedef struct fota {
    const fota_cls_t *cls;

    netio_t *from;
    netio_t *to;
    fota_status_e status;
    char *from_path;
    char *to_path;
    uint8_t *buffer;
    int offset;
    int quit;
    aos_task_t task;
    aos_sem_t sem;
    fota_event_cb_t event_cb;
    int running;
    int timeoutms;
    int retry_count;
    int sleep_time;
    void *private;
} fota_t;

int fota_start(fota_t *fota);

int fota_stop(fota_t *fota);

int fota_finish(fota_t *fota);

int fota_fail(fota_t *fota);

int fota_close(fota_t *fota);

fota_t *fota_open(const char *fota_name, const char *dst, fota_event_cb_t event_cb);

int fota_register_cop(void);
int fota_register_coap(void);
int fota_register_serial(void);

int fota_register(const fota_cls_t *cls);

/*
  FOTA_STATE_STOP means fota task is not running
  FOTA_STATE_FINISH means OTA finished, wait for reboot
*/
fota_state_e fota_state(void);

#endif
