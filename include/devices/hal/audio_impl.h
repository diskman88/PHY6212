/*
 * Copyright (C) 2017 C-SKY Microsystems Co., Ltd. All rights reserved.
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

#ifndef HAL_AUDIO_IMPL_H
#define HAL_AUDIO_IMPL_H

#include <stdint.h>
#include <devices/driver.h>

enum {
    WRITE_CON,
    READ_CON,

    TYPE_END
};

typedef struct {
    int type;
    int sample_rate;
    int sample_width;
    int sound_track;
} audio_config_t;

typedef void (*audio_event)(dev_t *dev, int event_id, void *priv);

typedef struct {
    driver_t drv;
    int (*config)(dev_t *dev, audio_config_t *config);
    int (*get_buffer_size)(dev_t *dev, int type);
    int (*send)(dev_t *dev, const void *data, uint32_t size);
    int (*recv)(dev_t *dev, void *data, uint32_t size);
    void (*set_event)(dev_t *dev, audio_event evt_cb, void *priv);
    int (*set_gain)(dev_t *dev, int l, int r);

    /* only for output */
    int (*start)(dev_t *dev);
    int (*pause)(dev_t *dev);
} audio_driver_t;


#endif
