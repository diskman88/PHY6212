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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <aos/debug.h>

#include "ringbuffer.h"

#define MIN(a, b) (a)<(b)? (a) : (b)

int ringbuffer_create(ringbuffer_t *ringbuffer, char *buffer, int length)
{
    aos_check_param(ringbuffer && buffer);

    memset(buffer, 0, length);

    ringbuffer->length = length - 1;
    ringbuffer->head = 0;
    ringbuffer->tail = 0;
    ringbuffer->buffer = (uint8_t *)buffer;

    return 0;
}

void ringbuffer_destroy(ringbuffer_t *ringbuffer)
{
    aos_check_param(ringbuffer);

    ringbuffer->length = ringbuffer->head = ringbuffer->tail = 0;
}

int ringbuffer_available_read_space(ringbuffer_t *ringbuffer)
{
    aos_check_param(ringbuffer);

    uint32_t head = ringbuffer->head;
    uint32_t tail = ringbuffer->tail;

    if (head == tail) {
        return 0;
    } else if (head < tail) {
        return tail - head;
    } else {
        return ringbuffer->length - (head - tail - 1);
    }
}

int ringbuffer_write(ringbuffer_t *ringbuffer, uint8_t *data, uint32_t length)
{
    aos_check_param(ringbuffer && data);

    int i = 0;

    if (ringbuffer_available_write_space(ringbuffer) < length) {
        length = ringbuffer_available_write_space(ringbuffer);
    }

    for (i = 0; i < length; i++) {

        ringbuffer->buffer[ringbuffer->tail] = data[i];

        ringbuffer->tail++;

        if (ringbuffer->tail >= ringbuffer->length + 1) {
            ringbuffer->tail = 0;
        }
    }

    /* return real write len */
    return i;
}

int ringbuffer_read(ringbuffer_t *ringbuffer, uint8_t *target, uint32_t amount)
{
    aos_check_param(ringbuffer && target);

    int copy_sz = 0;
    int i;

    if (amount == 0) {
        return -1;
    }

    if (ringbuffer_empty(ringbuffer)) {
        return 0;
    }

    /* get real read size */
    int buffer_size = ringbuffer_available_read_space(ringbuffer);
    copy_sz = MIN(amount, buffer_size);

    /* cp data to user buffer */
    for (i = 0; i < copy_sz; i++) {
        target[i] = ringbuffer->buffer[ringbuffer->head];

        ringbuffer->head++;

        if (ringbuffer->head >= ringbuffer->length + 1) {
            ringbuffer->head = 0;
        }
    }

    return copy_sz;
}
