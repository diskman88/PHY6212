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

#ifndef HAL_FLASH_H
#define HAL_FLASH_H

#include <stdint.h>

#include <devices/driver.h>

typedef struct {
    uint32_t    start_addr;
    uint32_t    block_size;
    uint32_t    block_count;
} flash_dev_info_t;

typedef struct flash_driver {
    driver_t    drv;
    int         (*read)(dev_t *dev, uint32_t addroff, void *buff, int32_t bytesize);
    int         (*program)(dev_t *dev, uint32_t dstaddr, const void *srcbuf, int32_t bytesize);
    int         (*erase)(dev_t *dev, int32_t addroff, int32_t blkcnt);
    int         (*get_info)(dev_t *dev, flash_dev_info_t *info);
} flash_driver_t;

#endif
