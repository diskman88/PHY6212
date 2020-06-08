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

#include "byte_rw.h"

uint16_t byte_r16be(const uint8_t *buf)
{
    if (buf) {
        return (buf[0] << 8) | buf[1];
    }
    return -1;
}

uint32_t byte_r24be(const uint8_t *buf)
{
    uint32_t x = -1;

    if (buf) {
        x = buf[0];
        x = (x << 8) | buf[1];
        x = (x << 8) | buf[2];
    }

    return x;
}

uint32_t byte_r32be(const uint8_t *buf)
{
    uint32_t x = -1;

    if (buf) {
        x = buf[0];
        x = (x << 8) | buf[1];
        x = (x << 8) | buf[2];
        x = (x << 8) | buf[3];
    }

    return x;
}

uint64_t byte_r64be(const uint8_t *buf)
{
    uint64_t x = -1;

    if (buf) {
        x = buf[0];
        x = (x << 8) | buf[1];
        x = (x << 8) | buf[2];
        x = (x << 8) | buf[3];
        x = (x << 8) | buf[4];
        x = (x << 8) | buf[5];
        x = (x << 8) | buf[6];
        x = (x << 8) | buf[7];
    }

    return x;
}

uint16_t byte_r16le(const uint8_t *buf)
{
    if (buf) {
        return buf[0] | (buf[1] << 8);
    }
    return -1;
}

uint32_t byte_r24le(const uint8_t *buf)
{
    uint32_t x = -1;

    if (buf) {
        x = buf[0];
        x = x | (buf[1] << 8);
        x = x | (buf[2] << 16);
    }

    return x;
}

uint32_t byte_r32le(const uint8_t *buf)
{
    uint32_t x = -1;

    if (buf) {
        x = buf[0];
        x = x | (buf[1] << 8);
        x = x | (buf[2] << 16);
        x = x | (buf[3] << 24);
    }

    return x;
}

uint64_t byte_r64le(const uint8_t *buf)
{
    uint64_t x = -1, y;

    if (buf) {
        x = buf[0];
        x = x | (buf[1] << 8);
        x = x | (buf[2] << 16);
        x = x | (buf[3] << 24);

        y = buf[4];
        y = y | (buf[5] << 8);
        y = y | (buf[6] << 16);
        y = y | (buf[7] << 24);

        x = x | (y << 32);
    }

    return x;
}


