/**
 * @file eftl_si.h
 * @brief Public APIs of EFC drivers
 *
 * Copyright (C) 2017 Sanechips Technology Co., Ltd.
 * @author Hui Wu <wu.hui1@sanechips.com.cn>
 * @ingroup si_id
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef DEVICE_EFTL_SI_H
#define DEVICE_EFTL_SI_H

/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define EFTL_NO_PWR_DOWN_PROT   (0)
#define EFTL_PWR_DOWN_PROT      (1)

#define EFTL_PAGE_NIL           (0xFFFF)
#define EFTL_HEAD               "ftl"
#define EFTL_HEAD_BYTES         (3)
#define EFTL_PAGE_SIZE          (512)

/* these info are used in BlockTable */
#define EFTL_PAGE_FREE          (0x00)
#define EFTL_PAGE_USED          (0x55)
#define EFTL_PAGE_DIRTY         (0x11)
#define EFTL_PARTITIONS_MAX     (0x3)

#define EFTL_SHARE_BUF
/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
struct eftl_page_tag {
    uint8_t head[3];
    uint8_t is_used;
    uint32_t ver;
    uint32_t logic_page;
};

struct eftl_info {
    uint32_t page_size;
    uint32_t page_data_size;
    uint32_t page_shift;
    uint32_t first_page_base;
    uint32_t first_page_shift;
    uint32_t tag_offset;
    uint32_t pages_tnr;
    uint32_t pages_vnr;
    uint32_t attr;
};

struct eftl_map {
    struct eftl_info info;

    uint32_t *cur_ver_tbl;
    uint16_t *map_tbl;
    uint8_t  *page_stat_tbl;
    uint8_t  *page_buf;
    uint32_t last_free_page;
};

struct eftl_cfg {
    uint32_t addr;
    uint32_t vlen;
    uint32_t tlen;
    uint32_t attr;
};

#define EFLASH1_MAIN_START                  (EFLASH1_BASEADDR)
#define EFLASH2_MAIN_START                  (EFLASH2_BASEADDR)
#define EFLASH1_INFO_START                  (EFLASH1_INFO_BASEADDR)
#define EFLASH2_INFO_START                  (EFLASH2_INFO_BASEADDR)
#define EFLASH1_MAIN_SIZE                   (1216*1024)
#define EFLASH2_MAIN_SIZE                   (256*1024)
#define EFLASH1_INFO_SIZE                   (2*2048)
#define EFLASH2_INFO_SIZE                   (2*512)

#define IS_EFC1_MAIN_ADDR(addr) \
    ((addr >= EFLASH1_MAIN_START)\
     &&(addr < EFLASH1_MAIN_START + EFLASH1_MAIN_SIZE))

#define IS_EFC1_INFO_ADDR(addr) \
    ((addr >= EFLASH1_INFO_START)\
     &&(addr < EFLASH1_INFO_START+EFLASH1_INFO_SIZE))

#define IS_EFC2_MAIN_ADDR(addr) \
    ((addr >= EFLASH2_MAIN_START)\
     &&(addr < EFLASH2_MAIN_START+EFLASH2_MAIN_SIZE))

#define IS_EFC2_INFO_ADDR(addr) \
    ((addr >= EFLASH2_INFO_START)\
     &&(addr < EFLASH2_INFO_START+EFLASH2_INFO_SIZE))

#define is_efc1_addr(addr)  (IS_EFC1_MAIN_ADDR(addr) || IS_EFC1_INFO_ADDR(addr))

#define is_efc2_addr(addr)  (IS_EFC2_MAIN_ADDR(addr) || IS_EFC2_INFO_ADDR(addr))

/*******************************************************************************
 *                       Global variable declarations                          *
 ******************************************************************************/

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
/**
 * @brief Read data.
 *
 * @param addr Read begining addr.
 * @param data Pointer to the dest data.
 * @param size Read size in bytes.
 *
 * @return Standard errno
 */
int ftl_read(uint32_t addr, uint8_t *data, uint32_t size);

/**
 * @brief Read data in poll mode, just for psm.
 *
 * @param addr Read begining addr.
 * @param data Pointer to the dest data.
 * @param size Read size in bytes.
 *
 * @return Standard errno
 */
int ftl_read_poll(uint32_t addr, uint8_t *data, uint32_t size);

/**
 * @brief Write data.
 *
 * @param addr Write begining addr.
 * @param data Pointer to the src data.
 * @param size Write size in bytes.
 *
 * @return Standard errno
 */
int ftl_write(uint32_t addr, const uint8_t *data, uint32_t size);

/**
 * @brief Write data in poll mode,just for psm.
 *
 * @param addr Write begining addr.
 * @param data Pointer to the src data.
 * @param size Write size in bytes.
 *
 * @return Standard errno
 */
int ftl_write_poll(uint32_t addr, const uint8_t *data, uint32_t size);

/**
 * @brief Erase data.
 *
 * @param addr Erase begining addr.
 * @param size Erase size in bytes.
 *
 * @return Standard errno
 */
int ftl_erase(uint32_t addr, uint32_t size);

/**
 * @brief Erase data,just for psm..
 *
 * @param addr Erase begining addr.
 * @param size Erase size in bytes.
 *
 * @return Standard errno
 */
int ftl_erase_poll(uint32_t addr, uint32_t size);

/**
 * @brief Init the ftl region.
 *
 * @param cfg Pointer to ftl cfg structure.
 * @param nr  Ftl region num.
 *
 * @return Standard errno
 */
int ftl_init(const struct eftl_cfg *cfg, uint32_t nr);

/*******************************************************************************
 *                      Inline function implementations                        *
 ******************************************************************************/


#endif /* #ifndef _EFTL_SI_H_ */

