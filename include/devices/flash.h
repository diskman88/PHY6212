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

#ifndef DEVICE_FLASH_PAI_H
#define DEVICE_FLASH_PAI_H

#include "hal/flash_impl.h"

#define flash_open(name) device_open(name)
#define flash_open_id(name, id) device_open_id(name, id)
#define flash_close(dev) device_close(dev)

/**
  \brief       Notify a flash to read data.
  \param[in]   dev      Pointer to device object.
  \param[in]   addroff  address to read flash.
  \param[out]  buff     buffer address to store data read.
  \param[in]   bytesize data length expected to read.
  \return      0 on success, -1 on fail.
*/
int flash_read(dev_t *dev, int32_t addroff, void *buff, int32_t bytesize);

/**
  \brief       Program flash at specified address.
  \param[in]   dev      Pointer to device object.
  \param[in]   dstaddr  address to program flash.
  \param[in]   srcbuf   buffer storing data to be programmed.
  \param[in]   bytesize data length to be programmed.
  \return      0 on success, -1 on fail.
*/
int flash_program(dev_t *dev, int32_t dstaddr, const void *srcbuf, int32_t bytesize);

/**
  \brief       Notify a flash to fetch data.
  \param[in]   dev      Pointer to device object.
  \param[in]   addroff  a flash section including the address will be erased.
  \param[in]   blkcnt   erased block count
  \return      0 on success, -1 on fail.
*/
int flash_erase(dev_t *dev, int32_t addroff, int32_t blkcnt);

/**
  \brief       Get info from a flash device.
  \param[in]   dev  Pointer to device object.
  \param[out]  info return flash info
  \return      0 on success, -1 on fail.
*/
int flash_get_info(dev_t *dev, flash_dev_info_t *info);

#endif
