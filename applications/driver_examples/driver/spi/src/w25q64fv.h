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
/******************************************************************************
 * @file     w25q64fv.h
 * @brief    header file for w25q64fv driver
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/
#ifndef __W25Q64FV_H_
#define __W25Q64FV_H_

#define W25Q64FV_PAGE_SIZE 0x100

#define W25Q64FV_ADDR_START   0x0
#define W25Q64FV_ADDR_END     0x10000
#define W25Q64FV_CLK_RATE     20000000        /* 2MHZ */

#define IS_FLASH_ADDR(addr) \
    ((addr >= W25Q64FV_ADDR_START) && (addr < W25Q64FV_ADDR_END))

#endif /* __W25Q64FV_H_ */

