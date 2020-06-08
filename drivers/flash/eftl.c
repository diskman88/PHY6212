/**
 * @file eftl_si.c
 * @brief Implementation of Sanechips
 *
 * Copyright (C) 2017 Sanechips Technology Co., Ltd.
 * @author Hui Wu <wu.hui1@sanechips.com.cn>
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

/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
#include <yoc_config.h>

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <aos/kernel.h>

#include <devices/flash.h>

#include <devices/eftl.h>
#include "reg_define_si.h"
/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define SYS_LOG_DOMAIN "ftl"
#define TAG "EFTL"
#define CSI_FLASH_DEVNAME "eflash1"

#define ROUND_DOWN(x, align) ((unsigned long)(x) & ~((unsigned long)align - 1))

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

//#define CONFIG_EFTL_DEBUG
#ifdef CONFIG_EFTL_DEBUG
#define FTL_DBG(fmt, ...) LOGE(TAG, fmt, ##__VA_ARGS__)
#else
#define FTL_DBG(fmt, ...)
#endif

static inline unsigned int find_lsb_set(uint32_t op)
{
    return __builtin_ffs(op);
}

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
struct eftl_device {
    dev_t   *flash_dev;
    struct eftl_map   *map;
    aos_mutex_t  op_mutex;
    uint32_t          is_poll;
};

/*******************************************************************************
 *                        Local function declarations                          *
 ******************************************************************************/

/*******************************************************************************
 *                         Local variable definitions                          *
 ******************************************************************************/
static struct eftl_device eftl_devs[EFTL_PARTITIONS_MAX];

#ifdef EFTL_SHARE_BUF
static aos_mutex_t eftl_op_mutex;
#endif

/*******************************************************************************
 *                        Global variable definitions                          *
 ******************************************************************************/

/*******************************************************************************
 *                      Inline function implementations                        *
 ******************************************************************************/

/*******************************************************************************
 *                      Local function implementations                         *
 ******************************************************************************/

static uint32_t eftl_flash_get_addroff(struct eftl_device *dev, uint32_t addr)
{
    flash_dev_info_t info;

    flash_get_info(dev->flash_dev, &info);

    int blk_num = (addr - info.start_addr) / info.block_size;

    return blk_num * info.block_size + addr % info.block_size;
}

static int eftl_flash_erase_part(struct eftl_device *dev, uint32_t addr, uint32_t size)
{

    uint8_t tmp_page[EFTL_PAGE_SIZE];

    if (size > EFTL_PAGE_SIZE) {
        return -1;
    }

    memset(tmp_page, 0xFF, EFTL_PAGE_SIZE);

    return flash_program(dev->flash_dev, eftl_flash_get_addroff(dev, addr), tmp_page, size);
}

/**
 * @brief Collect the dirty pages,and erase them.
 *
 * @param dev Pointer to ftl dev structure.
 *
 * @return Standard errno.
 */
static int eftl_garbage_collect(struct eftl_device *dev)
{
    int ret;
    uint32_t page;
    struct eftl_map *map = dev->map;

    for (page = 0; page < map->info.pages_tnr; page++) {
        if (map->page_stat_tbl[page] == EFTL_PAGE_DIRTY) {

            FTL_DBG("erasepage map->page_stat_tbl[-%d-].\n", page);

            if (dev->is_poll) {
                ret = flash_erase(dev->flash_dev, eftl_flash_get_addroff(dev, ((map->info.first_page_shift + page)
                                      << map->info.page_shift)), 1);
            } else {
                ret = flash_erase(dev->flash_dev, eftl_flash_get_addroff(dev, ((map->info.first_page_shift + page)
                                      << map->info.page_shift)), 1);
            }

            if (ret) {
                FTL_DBG("flash_erase failed.\n");
                return ret;
            }

            map->page_stat_tbl[page] = EFTL_PAGE_FREE ;
        }
    }

    return 0;
}

/**
 * @brief Create map table ccording to the flash information.
 *
 * @param map Pointer to ftl map structure.
 *
 * @return Standard errno
 */
static int eftl_create_table(struct eftl_map *map)
{
    uint32_t page;
    uint32_t addr;
    struct eftl_page_tag *tag;

    for (page = 0; page < map->info.pages_tnr; page++) {
        addr = (map->info.first_page_shift + page) << map->info.page_shift;
        tag = (struct eftl_page_tag *)(addr + map->info.tag_offset);

        if (!memcmp(tag->head, EFTL_HEAD, EFTL_HEAD_BYTES)) {
            if ((tag->ver) > (map->cur_ver_tbl[tag->logic_page])) {
//the version in ram is smaller than that in eflash, find newer version, the old is dirty
                if (map->cur_ver_tbl[tag->logic_page] != 0) {
                    map->page_stat_tbl[map->map_tbl[tag->logic_page]] = EFTL_PAGE_DIRTY;
                }

                map->map_tbl[tag->logic_page] = page;
                map->cur_ver_tbl[tag->logic_page] = tag->ver;
                map->page_stat_tbl[page] = EFTL_PAGE_USED;
                map->last_free_page = page + 1;
            } else {
                map->page_stat_tbl[page] = EFTL_PAGE_DIRTY;
            }
        } else {
            map->page_stat_tbl[page] = EFTL_PAGE_DIRTY;
        }
    }

    return 0;
}

/**
 * @brief Find one free page.
 *
 * @param dev       Pointer to ftl dev structure.
 * @param free_page Pointer to the store free-page id addr.
 *
 * @return Standard errno
 */
static int eftl_find_free_page(struct eftl_device *dev, uint32_t *free_page)
{
    int ret;
    uint32_t free_page_max;
    uint32_t page;
    uint32_t find_nr = 2;
    struct eftl_map *map = dev->map;

    do {
        page = map->last_free_page;
        free_page_max = map->info.pages_tnr;

        for (; page < free_page_max; page++) {
            if (map->page_stat_tbl[page] == EFTL_PAGE_FREE) {
                *free_page = page;
                map->last_free_page = page + 1;
                FTL_DBG("map->page_stat_tbl[ -%d- ]-EFTL_PAGE_FREE -\n", page);
                return 0;
            }
        }

        free_page_max = map->last_free_page;
        page = 1;

        for (; page < free_page_max; page++) {
            if (map->page_stat_tbl[page - 1] == EFTL_PAGE_FREE) {
                *free_page = page - 1;
                map->last_free_page = page;
                FTL_DBG("map->page_stat_tbl[ -%d- ]-EFTL_PAGE_FREE -\n", page - 1);
                return 0;
            }
        }

        FTL_DBG("no free block --> eftl_garbage_collect()\n");

        ret = eftl_garbage_collect(dev);

        if (ret) {
            FTL_DBG("eftl_garbage_collect failed\n");
            return ret;
        }
    } while (--find_nr != 0);

    return -EIO;
}

/**
 * @brief Read data.
 *
 * @param map    Pointer to ftl map structure.
 * @param page   Page number.
 * @param offset Offset in one page.
 * @param data   Pointer to the dest data.
 * @param size   Read size in one page.
 *
 * @return void
 */
static int eftl_read_data(struct eftl_map *map,
                          uint32_t page, uint32_t offset,
                          uint8_t *data, uint32_t size)
{
    uint32_t phy_page;

    if (map->map_tbl[page] != EFTL_PAGE_NIL) {
        phy_page = map->map_tbl[page];

        FTL_DBG("phy_addr -0x%0x-, size -0x%0x-)\n",
                ((map->info.first_page_shift + phy_page)
                 << map->info.page_shift) + offset, size);

        memcpy(data, (void *)(((map->info.first_page_shift + phy_page)
                               << map->info.page_shift) + offset), size);
    } else {
        memset(data, 0xff, size);
        FTL_DBG("logic addr have no mapped phy addr,default oxff\n");
    }

    return 0;
}

/**
 * @brief Write empty page data.
 *
 * @param dev    Pointer to ftl dev structure.
 * @param page   Page number.
 * @param offset Offset in one page.
 * @param data   Pointer to the src data.
 * @param size   Write size in one page.
 *
 * @return Standard errno
 */
static int eftl_write_empty_page(struct eftl_device *dev,
                                 uint32_t page, uint32_t offset,
                                 const uint8_t *data, uint32_t size)
{
    int ret;
    uint32_t free_page;
    struct eftl_map *map = dev->map;
    struct eftl_page_tag *tag;

    memset(map->page_buf, 0xff, map->info.page_size);
    memcpy(map->page_buf + offset, data, size);

    ret = eftl_find_free_page(dev, &free_page);

    if (ret) {
        FTL_DBG("can not find free page\n");
        return ret;
    }

    if (dev->is_poll) {
        ret = flash_program(dev->flash_dev,
                                eftl_flash_get_addroff(dev, (map->info.first_page_shift + free_page)
                                        << map->info.page_shift), map->page_buf,
                                map->info.page_size);
    } else {
        ret = flash_program(dev->flash_dev,
                                eftl_flash_get_addroff(dev, (map->info.first_page_shift + free_page)
                                        << map->info.page_shift), map->page_buf,
                                map->info.page_size);
    }

    if (ret) {
        FTL_DBG("flash_write addr=%x ,page_buf=%x failed\n",
                (map->info.first_page_shift + free_page)
                << map->info.page_shift, (uint32_t)map->page_buf);

        return ret;
    }

    tag = (struct eftl_page_tag *)(map->page_buf + map->info.tag_offset);
    tag->logic_page = page;
    tag->is_used = 1;
    tag->ver = 1;
    memcpy(tag->head, EFTL_HEAD, EFTL_HEAD_BYTES);

    if (dev->is_poll) {
        ret = flash_program(dev->flash_dev,
                                eftl_flash_get_addroff(dev, ((map->info.first_page_shift + free_page)
                                        << map->info.page_shift) + map->info.tag_offset),
                                map->page_buf + map->info.tag_offset,
                                sizeof(struct eftl_page_tag));
    } else {
        ret = flash_program(dev->flash_dev,
                                eftl_flash_get_addroff(dev, ((map->info.first_page_shift + free_page)
                                        << map->info.page_shift) + map->info.tag_offset),
                                map->page_buf + map->info.tag_offset,
                                sizeof(struct eftl_page_tag));
    }

    if (ret) {
        FTL_DBG("flash_write addr=%x ,page_buf=%x failed\n",
                ((map->info.first_page_shift + free_page)
                 << map->info.page_shift) + map->info.tag_offset,
                (uint32_t)(map->page_buf + map->info.tag_offset));

        return ret;
    }

    FTL_DBG("tag->logic_id = 0x%x, free_page = 0x%x\n\r",
            tag->logic_page, free_page);

    /* renew page_table,page_st */
    map->map_tbl[tag->logic_page] = free_page;
    map->page_stat_tbl[free_page] = EFTL_PAGE_USED;

    return 0;
}

/**
 * @brief Update writen page data.
 *
 * @param dev    Pointer to ftl dev structure.
 * @param page   Page number.
 * @param offset Offset in one page.
 * @param data   Pointer to the src data.
 * @param size   Write size in one page.
 *
 * @return Standard errno
 */
static int eftl_update_page(struct eftl_device *dev,
                            uint32_t page, uint32_t offset,
                            const uint8_t *data, uint32_t size)
{
    int ret;
    uint32_t free_page;
    uint32_t last_phy_page;
    struct eftl_map *map = dev->map;
    struct eftl_page_tag *tag;
    struct eftl_page_tag *last_tag;

    last_phy_page = map->map_tbl[page];
    memcpy(map->page_buf, (void *)((map->info.first_page_shift + last_phy_page)
                                   << map->info.page_shift), map->info.page_size);
    memset(map->page_buf + map->info.tag_offset, 0xff,
           sizeof(struct eftl_page_tag));
    memcpy(map->page_buf + offset, data, size);

    ret = eftl_find_free_page(dev, &free_page);

    if (ret) {
        FTL_DBG("can not find free page\n");
        return ret;
    }

    if (dev->is_poll) {
        ret = flash_program(dev->flash_dev,
                                eftl_flash_get_addroff(dev, (map->info.first_page_shift + free_page)
                                        << map->info.page_shift),
                                map->page_buf, map->info.page_size);
    } else {
        ret = flash_program(dev->flash_dev,
                                eftl_flash_get_addroff(dev, (map->info.first_page_shift + free_page)
                                        << map->info.page_shift),
                                map->page_buf, map->info.page_size);
    }

    if (ret) {
        FTL_DBG("flash_write addr=%x ,page_buf=%x failed\n",
                (map->info.first_page_shift + free_page) << map->info.page_shift,
                (uint32_t)map->page_buf);

        return ret;
    }

    last_tag = (struct eftl_page_tag *)(((map->info.first_page_shift + last_phy_page)
                                         << map->info.page_shift)
                                        + map->info.tag_offset);
    tag = (struct eftl_page_tag *)(map->page_buf + map->info.tag_offset);
    tag->is_used = 1;
    tag->ver = last_tag->ver + 1;
    tag->logic_page = page;
    memcpy(tag->head, EFTL_HEAD, EFTL_HEAD_BYTES);

    if (dev->is_poll) {
        ret = flash_program(dev->flash_dev,
                                eftl_flash_get_addroff(dev, ((map->info.first_page_shift + free_page)
                                        << map->info.page_shift) + map->info.tag_offset),
                                map->page_buf + map->info.tag_offset,
                                sizeof(struct eftl_page_tag));
    } else {
        ret = flash_program(dev->flash_dev,
                                eftl_flash_get_addroff(dev, ((map->info.first_page_shift + free_page)
                                        << map->info.page_shift) + map->info.tag_offset),
                                map->page_buf + map->info.tag_offset,
                                sizeof(struct eftl_page_tag));
    }

    if (ret) {
        FTL_DBG("flash_write addr=%x ,page_buf=%x failed\n",
                ((map->info.first_page_shift + free_page)
                 << map->info.page_shift) + map->info.tag_offset,
                (uint32_t)(map->page_buf + map->info.tag_offset));

        return ret;
    }

    FTL_DBG("tag->logic_id = 0x%x, free_page = 0x%x\n\r",
            tag->logic_page, free_page);

    if (dev->is_poll) {
        ret = flash_erase(dev->flash_dev, eftl_flash_get_addroff(dev, (map->info.first_page_shift + last_phy_page)
                              << map->info.page_shift), 1);
    } else {
        ret = flash_erase(dev->flash_dev, eftl_flash_get_addroff(dev, (map->info.first_page_shift + last_phy_page)
                              << map->info.page_shift), 1);
    }

    if (ret) {
        FTL_DBG("flash_erase failed.\n\r");
        return ret;
    }

    /* renew page_table,page_st */
    map->map_tbl[tag->logic_page] = free_page;
    map->page_stat_tbl[free_page] = EFTL_PAGE_USED;
    map->page_stat_tbl[last_phy_page] = EFTL_PAGE_FREE;

    return 0;
}

/**
 * @brief Check the addr is invalid or not in power down protection mode.
 *
 * @param map  Pointer to ftl map structure.
 * @param addr Begining addr.
 * @param size Size in bytes.
 *
 * @return Standard errno
 */
static int eftl_prot_addr_check(struct eftl_map *map,
                                uint32_t addr, uint32_t size)
{
    uint32_t page_addr = ROUND_DOWN(addr, EFTL_PAGE_SIZE);

    if ((addr >= (map->info.first_page_base +
                  (map->info.pages_vnr) * map->info.page_size))
        || ((addr + size) > (page_addr + map->info.page_data_size))) {
        FTL_DBG("addr=%x size=%x cross page\n", addr, size);
        return -EINVAL;
    }

    return 0;
}

/**
 * @brief Check the addr is invalid or not in no power down protection mode.
 *
 * @param map  Pointer to ftl map structure.
 * @param addr Begining addr.
 * @param size Size in bytes.
 *
 * @return Standard errno
 */
static int eftl_no_prot_addr_check(struct eftl_map *map,
                                   uint32_t addr, uint32_t size)
{
    if ((addr + size) > (map->info.first_page_base +
                         (map->info.pages_vnr) * map->info.page_data_size)) {
        FTL_DBG("addr=%x,size=%x is out of range\n", addr, size);
        return -EINVAL;
    }

    return 0;
}


/**
 * @brief Read data with power down protection.
 *
 * @param dev  Pointer to ftl dev structure.
 * @param addr Read begining addr.
 * @param data Pointer to the dest data.
 * @param size Read size in bytes.
 *
 * @return Standard errno
 */
static int eftl_pwr_down_prot_read(struct eftl_device *dev,
                                   uint32_t addr, uint8_t *data,
                                   uint32_t size)
{
    int ret;
    uint32_t page;
    uint32_t offset;
    struct eftl_map *map = dev->map;

    if (eftl_prot_addr_check(map, addr, size) != 0) {
        return -EINVAL;
    }

    offset = addr & (map->info.page_size - 1);
    page = (addr >> map->info.page_shift) - (map->info.first_page_shift);

    ret = eftl_read_data(map, page, offset, data, size);

    if (ret) {
        FTL_DBG("eftl_read_data failed \n");
    }

    return ret;
}

/**
 * @brief Write data with power down protection.
 *
 * @param dev  Pointer to ftl dev structure.
 * @param addr Write begining addr.
 * @param data Pointer to the src data.
 * @param size Write size in bytes.
 *
 * return Standard errno
 */
static int eftl_pwr_down_prot_write(struct eftl_device *dev,
                                    uint32_t addr, const uint8_t *data,
                                    uint32_t size)
{
    int32_t ret;
    uint32_t page;
    uint32_t offset;
    struct eftl_map *map = dev->map;

    if (eftl_prot_addr_check(map, addr, size) != 0) {
        return -EINVAL;
    }

    offset = addr & (map->info.page_size - 1);
    page = (addr >> map->info.page_shift) - (map->info.first_page_shift);

    if (map->map_tbl[page] != EFTL_PAGE_NIL) {
        ret = eftl_update_page(dev, page, offset, data, size);

        if (ret) {
            FTL_DBG("eftl_update_page failed\n");
            return ret;
        }
    } else {
        ret = eftl_write_empty_page(dev, page, offset, data, size);

        if (ret) {
            FTL_DBG("eftl_write_empty_page failed\n");
            return ret;
        }
    }

    return 0;
}

/**
 * @brief Erase data with power down protection.
 *
 * @param dev   Pointer to ftl dev structure.
 * @param addr  Erase begining addr.
 * @param size  Erase size in bytes.
 *
 * @return Standard errno
 */
static int eftl_pwr_down_prot_erase(struct eftl_device *dev,
                                    uint32_t addr, uint32_t size)
{
    int ret;
    uint32_t page;
    uint32_t offset;
    uint32_t phy_page;
    uint32_t erase_size;
    struct eftl_map *map = dev->map;

    if ((addr + size) > (map->info.first_page_base +
                         (map->info.pages_vnr) * map->info.page_size)) {
        FTL_DBG("addr + size is out of erase range.\n");
        return -EINVAL;
    }

    while (size > 0) {
        offset = addr & (map->info.page_size - 1);
        page = (addr >> map->info.page_shift) - (map->info.first_page_shift);

        if (size <= (map->info.page_data_size - offset)) {
            erase_size = size;
        } else {
            erase_size = map->info.page_data_size - offset;
        }

        if (map->map_tbl[page] != EFTL_PAGE_NIL) {
            phy_page = map->map_tbl[page];

            if (erase_size == map->info.page_data_size) {
                if (dev->is_poll) {
                    ret = flash_erase(dev->flash_dev, eftl_flash_get_addroff(dev, (((map->info.first_page_shift + phy_page)
                                          << map->info.page_shift) + offset)), 1);
                } else {
                    ret = flash_erase(dev->flash_dev, eftl_flash_get_addroff(dev, (((map->info.first_page_shift + phy_page)
                                          << map->info.page_shift) + offset)), 1);
                }
            } else {
                if (dev->is_poll) {
                    ret = eftl_flash_erase_part(dev, ((map->info.first_page_shift + phy_page)
                                                      << map->info.page_shift) + offset,
                                                erase_size);
                } else {
                    ret = eftl_flash_erase_part(dev, ((map->info.first_page_shift + phy_page)
                                                      << map->info.page_shift) + offset,
                                                erase_size);
                }
            }

            if (ret) {
                FTL_DBG("flash erase failed.\n");
                return ret;
            }

            if (erase_size == map->info.page_data_size) {
                map->page_stat_tbl[phy_page] = EFTL_PAGE_FREE;
                map->map_tbl[page] = 0xffff;
            }
        }

        size -= erase_size;
        addr += erase_size + sizeof(struct eftl_page_tag);
    }

    return 0;
}

/**
 * @brief Read data with no power down protection.
 *
 * @param dev  Pointer to ftl dev structure.
 * @param addr Read begining addr.
 * @param data Pointer to the dest data.
 * @param size Read size in bytes.
 *
 * @return Standard errno
 */
static int eftl_no_pwr_down_prot_read(struct eftl_device *dev,
                                      uint32_t addr, uint8_t *data,
                                      uint32_t size)
{
    int ret;
    uint32_t page;
    uint32_t read_size;
    uint32_t offset;
    struct eftl_map *map = dev->map;

    if (eftl_no_prot_addr_check(map, addr, size) != 0) {
        return -EINVAL;
    }

    while (size > 0) {
        offset = (addr - map->info.first_page_base) % map->info.page_data_size;
        page = (addr - map->info.first_page_base) / map->info.page_data_size;

        if (size <= (map->info.page_data_size - offset)) {
            read_size = size;
        } else {
            read_size = map->info.page_data_size - offset;
        }

        ret = eftl_read_data(map, page, offset, data, read_size);

        if (ret) {
            FTL_DBG("eftl_read_data failed \n");
            return ret;
        }

        size -= read_size;
        addr += read_size;
        data += read_size;
    }

    return 0;
}

/**
 * @brief Write data with no power down protection.
 *
 * @param dev  Pointer to ftl dev structure.
 * @param addr Write begining addr.
 * @param data Pointer to the src data.
 * @param size Write size in bytes.
 *
 * @return Standard errno
 */
static int eftl_no_pwr_down_prot_write(struct eftl_device *dev,
                                       uint32_t addr, const uint8_t *data,
                                       uint32_t size)
{
    int32_t ret;
    uint32_t page;
    uint32_t offset;
    uint32_t write_size;
    struct eftl_map *map = dev->map;

    if (eftl_no_prot_addr_check(map, addr, size) != 0) {
        return -EINVAL;
    }

    while (size > 0) {
        offset = (addr - map->info.first_page_base) % map->info.page_data_size;
        page = (addr - map->info.first_page_base) / map->info.page_data_size;

        if (size <= (map->info.page_data_size - offset)) {
            write_size = size;
        } else {
            write_size = map->info.page_data_size - offset;
        }

        if (map->map_tbl[page] != EFTL_PAGE_NIL) {
            ret = eftl_update_page(dev, page, offset, data, write_size);

            if (ret) {
                FTL_DBG("eftl_update_page failed\n");
                return ret;
            }
        } else {
            ret = eftl_write_empty_page(dev, page, offset, data, write_size);

            if (ret) {
                FTL_DBG("eftl_write_empty_page failed\n");
                return ret;
            }
        }

        size -= write_size;
        addr += write_size;
        data += write_size;
    }

    return 0;
}

/**
 * @brief Erase data with no power down protection.
 *
 * @param dev   Pointer to ftl dev structure.
 * @param addr  Erase begining addr.
 * @param size  Erase size in bytes.
 *
 * @return Standard errno
 */
static int eftl_no_pwr_down_prot_erase(struct eftl_device *dev,
                                       uint32_t addr, uint32_t size)
{
    int ret;
    uint32_t page;
    uint32_t offset;
    uint32_t phy_page;
    uint32_t erase_size;
    struct eftl_map *map = dev->map;

    if ((addr + size) > (map->info.first_page_base +
                         (map->info.pages_vnr) * map->info.page_data_size)) {
        FTL_DBG("addr + size is out of erase range.\n");
        return -EINVAL;
    }

    while (size > 0) {
        offset = (addr - map->info.first_page_base) % map->info.page_data_size;
        page = (addr - map->info.first_page_base) / map->info.page_data_size;

        if (size <= (map->info.page_data_size - offset)) {
            erase_size = size;
        } else {
            erase_size = map->info.page_data_size - offset;
        }

        if (map->map_tbl[page] != EFTL_PAGE_NIL) {
            phy_page = map->map_tbl[page];

            if (erase_size == map->info.page_data_size) {
                if (dev->is_poll) {
                    ret = flash_erase(dev->flash_dev, eftl_flash_get_addroff(dev, ((map->info.first_page_shift + phy_page)
                                          << map->info.page_shift) + offset), 1);
                } else {
                    ret = flash_erase(dev->flash_dev, eftl_flash_get_addroff(dev, ((map->info.first_page_shift + phy_page)
                                          << map->info.page_shift) + offset), 1);
                }
            } else {
                if (dev->is_poll) {
                    ret = eftl_flash_erase_part(dev, ((map->info.first_page_shift + phy_page)
                                                      << map->info.page_shift) + offset,
                                                erase_size);
                } else {
                    ret = eftl_flash_erase_part(dev, ((map->info.first_page_shift + phy_page)
                                                      << map->info.page_shift) + offset,
                                                erase_size);
                }
            }

            if (ret) {
                FTL_DBG("flash erase failed.\n");
                return ret;
            }

            if (erase_size == map->info.page_data_size) {
                map->page_stat_tbl[phy_page] = EFTL_PAGE_FREE;
                map->map_tbl[page] = 0xffff;
            }
        }

        size -= erase_size;
        addr += erase_size;
    }

    return 0;
}

/**
 * @brief Check the addr is invalid,and position region.
 *
 * @param addr  Operation addr.
 *
 * @return Standard errno
 */
static struct eftl_device *eftl_get_device(uint32_t addr)
{
    uint32_t nr;
    struct eftl_map *map;

    for (nr = 0; nr < EFTL_PARTITIONS_MAX; nr++) {
        map = eftl_devs[nr].map;

        if ((addr >= map->info.first_page_base)
            && (addr < (map->info.first_page_base + \
                        map->info.pages_tnr * map->info.page_size))) {
            return &eftl_devs[nr];
        }
    }

    if (nr == EFTL_PARTITIONS_MAX) {
        FTL_DBG("addr is out of range\n");
    }

    return NULL;
}

/**
 * @brief Write data.
 *
 * @param dev  Pointer to ftl dev structure.
 * @param addr Write begining addr.
 * @param data Pointer to the src data.
 * @param size Write size in bytes.
 *
 * @return Standard errno
 */
static int eftl_si_write(struct eftl_device *dev,
                         uint32_t addr, const uint8_t *data, uint32_t size)
{
    int32_t ret;
    struct eftl_map *map = dev->map;

    if (data == NULL || size == 0) {
        FTL_DBG("data/size invalid.\n");
        return -EINVAL;
    }

    if (map->info.attr == EFTL_PWR_DOWN_PROT) {
        ret = eftl_pwr_down_prot_write(dev, addr, data, size);

        if (ret) {
            FTL_DBG("eftl_pwr_down_prot_write failed.\n");
        }
    } else if (map->info.attr == EFTL_NO_PWR_DOWN_PROT) {
        ret = eftl_no_pwr_down_prot_write(dev, addr, data, size);

        if (ret) {
            FTL_DBG("eftl_no_pwr_down_prot_write failed.\n");
        }
    } else {
        ret = -EINVAL;
        FTL_DBG("attr is invalid.\n");
    }

    return ret;

}

/**
 * @brief Read data.
 *
 * @param dev  Pointer to ftl dev structure.
 * @param addr Read begining addr.
 * @param data Pointer to the dest data.
 * @param size Read size in bytes.
 *
 * @return Standard errno
 */
static int eftl_si_read(struct eftl_device *dev,
                        uint32_t addr, uint8_t *data, uint32_t size)
{
    int32_t ret;
    struct eftl_map *map = dev->map;

    if (data == NULL || size == 0) {
        FTL_DBG("data/size invalid.\n");
        return -EINVAL;
    }

    if (map->info.attr == EFTL_PWR_DOWN_PROT) {
        ret = eftl_pwr_down_prot_read(dev, addr, data, size);

        if (ret != 0) {
            FTL_DBG("eftl_pwr_down_prot_read failed.\n");
        }
    } else if (map->info.attr == EFTL_NO_PWR_DOWN_PROT) {
        ret = eftl_no_pwr_down_prot_read(dev, addr, data, size);

        if (ret != 0) {
            FTL_DBG("eftl_no_pwr_down_prot_read failed.\n");
        }
    } else {
        FTL_DBG("attr is invalid.\n");
        ret = -EINVAL;
    }

    return ret;
}

/**
 * @brief Erase data.
 *
 * @param dev   Pointer to ftl dev structure.
 * @param addr  Erase begining addr.
 * @param size  Erase size in bytes.
 *
 * @return Standard errno
 */
static int eftl_si_erase(struct eftl_device *dev, uint32_t addr, uint32_t size)
{
    int ret;
    struct eftl_map *map = dev->map;

    if (size == 0) {
        FTL_DBG("data/size invalid.\n");
        return -EINVAL;
    }

    if (map->info.attr == EFTL_PWR_DOWN_PROT) {
        ret = eftl_pwr_down_prot_erase(dev, addr, size);

        if (ret != 0) {
            FTL_DBG("eftl_pwr_down_prot_erase failed.\n");
        }
    } else if (map->info.attr == EFTL_NO_PWR_DOWN_PROT) {
        ret = eftl_no_pwr_down_prot_erase(dev, addr, size);

        if (ret != 0) {
            FTL_DBG("eftl_no_pwr_down_prot_erase failed.\n");
        }
    } else {
        FTL_DBG("attr is invalid.\n");
        return -EINVAL;
    }

    return ret;
}

/**
 * @brief Init the ftl region.
 *
 * @param cfg Pointer to ftl cfg structure.
 * @param nr  Ftl region num.
 *
 * @return Standard errno
 */
static int eftl_init(const struct eftl_cfg *cfg, uint32_t nr)
{
    uint32_t i;
    uint8_t *buf;
    struct eftl_map *map;

    if (nr > EFTL_PARTITIONS_MAX) {
        FTL_DBG("eftl partitions out of range EFTL_PARTITIONS_MAX\n");
        return -EINVAL;
    }

    buf = (uint8_t *)aos_malloc(EFTL_PAGE_SIZE);

    if (!buf) {
        FTL_DBG("malloc buf failed\n");
        return -ENOMEM;
    }

    memset(buf, 0xff, EFTL_PAGE_SIZE);

    for (i = 0; i < nr; i++) {
        if (is_efc2_addr(cfg[i].addr)) {
            eftl_devs[i].flash_dev = flash_open(CSI_FLASH_DEVNAME);

            if (eftl_devs[i].flash_dev == NULL) {
                FTL_DBG("device_get_binding eflash1 dev failed\n");
                return -EINVAL;
            }
        }  else {
            FTL_DBG("ftl addr have no relative falsh device.\n");
            return -EINVAL;
        }

        eftl_devs[i].map = (struct eftl_map *)aos_zalloc(sizeof(struct eftl_map));

        //SYS_LOG_ERR("map_size(%d) = %d bytes\n", i, sizeof(struct eftl_map));
        if (eftl_devs[i].map == NULL) {
            FTL_DBG("malloc eftl_map_tbl[%d]failed\n", i);
            goto alloc_map_failed;
        }

        map = eftl_devs[i].map;
        map->info.page_size = EFTL_PAGE_SIZE;
        map->info.page_data_size = map->info.page_size - sizeof(struct eftl_page_tag);
        map->info.page_shift = find_lsb_set(map->info.page_size) - 1;
        map->info.first_page_base = cfg[i].addr;
        map->info.first_page_shift = cfg[i].addr >> map->info.page_shift;
        map->info.tag_offset = map->info.page_size - sizeof(struct eftl_page_tag);
        map->info.pages_tnr = cfg[i].tlen >> map->info.page_shift;
        map->info.pages_vnr = cfg[i].vlen / map->info.page_data_size;
        map->info.attr = cfg[i].attr;

        /* malloc cur_ver_tbl space,init as 0x00, the version smallest is 1*/
        map->cur_ver_tbl = (uint32_t *)aos_zalloc(map->info.pages_tnr * 4);

        //SYS_LOG_ERR("ver_tbl(%d) = %d bytes\n", i, map->info.pages_tnr * 4);
        if (!map->cur_ver_tbl) {
            FTL_DBG("malloc cur_ver_tbl failed\n");
            goto alloc_ver_tbl_failed;
        }

        /*malloc eftl_map space,init as 0xFF */
        map->map_tbl = (uint16_t *)aos_malloc(map->info.pages_tnr * 2);

        //SYS_LOG_ERR("map_tbl(%d) = %d bytes\n", i, map->info.pages_tnr * 2);
        if (!map->map_tbl) {
            FTL_DBG("malloc map_tbl failed\n");
            goto alloc_map_tbl_failed;
        }

        memset(map->map_tbl, 0xff, map->info.pages_tnr * 2);

        /* malloc page_stat_tbl space,init as 0xff  */
        map->page_stat_tbl = (uint8_t *)aos_malloc(map->info.pages_tnr * 1);

        //SYS_LOG_ERR("stat_tbl(%d) = %d bytes\n", i, map->info.pages_tnr * 1);
        if (!map->page_stat_tbl) {
            FTL_DBG("malloc page_stat_tbl failed\n");
            goto alloc_page_stat_tbl_failed;
        }

        memset(map->page_stat_tbl, 0xff, map->info.pages_tnr * 1);

        map->page_buf = buf;

        if (eftl_create_table(map) != 0) {
            FTL_DBG("eftl_create_table failed\n");
            goto create_ftl_tbl_failed;
        }

#ifndef EFTL_SHARE_BUF
        aos_mutex_new(&eftl_devs[i].op_mutex);
#endif

        eftl_devs[i].is_poll = 0;
    }

#ifdef EFTL_SHARE_BUF
    aos_mutex_new(&eftl_op_mutex);
#endif

    return 0;

create_ftl_tbl_failed:
    aos_free(map->page_stat_tbl);
alloc_page_stat_tbl_failed:
    aos_free(map->map_tbl);
alloc_map_tbl_failed:
    aos_free(map->cur_ver_tbl);
alloc_ver_tbl_failed:
    aos_free(map);
alloc_map_failed:
    aos_free(buf);

    return -EIO;
}

/**
 * @brief Lock eftl operation.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return Void
 */
static void eftl_lock(struct eftl_device *dev)
{
#ifdef EFTL_SHARE_BUF
    aos_mutex_lock(&eftl_op_mutex, AOS_WAIT_FOREVER);
#else
    aos_mutex_lock(&dev->op_mutex, AOS_WAIT_FOREVER);
#endif
}

/**
 * @brief unlock eftl operation.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return Void
 */
static void eftl_unlock(struct eftl_device *dev)
{
#ifdef EFTL_SHARE_BUF
    aos_mutex_unlock(&eftl_op_mutex);
#else
    aos_mutex_unlock(&dev->op_mutex);
#endif
}

/*******************************************************************************
 *                      Global function implementations                        *
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
int ftl_read(uint32_t addr, uint8_t *data, uint32_t size)
{
    int ret;
    struct eftl_device *dev;

    dev = eftl_get_device(addr);

    if (dev == NULL) {
        return -EINVAL;
    }

    eftl_lock(dev);
    ret = eftl_si_read(dev, addr, data, size);
    eftl_unlock(dev);

    return ret;
}

/**
 * @brief Read data in poll mode, just for psm.
 *
 * @param addr Read begining addr.
 * @param data Pointer to the dest data.
 * @param size Read size in bytes.
 *
 * @return Standard errno
 */
int ftl_read_poll(uint32_t addr, uint8_t *data, uint32_t size)
{
    int ret;
    struct eftl_device *dev;

    dev = eftl_get_device(addr);

    if (dev == NULL) {
        return -EINVAL;
    }

    dev->is_poll = 1;
    ret = eftl_si_read(dev, addr, data, size);
    dev->is_poll = 0;

    return ret;
}

/**
 * @brief Write data.
 *
 * @param addr Write begining addr.
 * @param data Pointer to the src data.
 * @param size Write size in bytes.
 *
 * @return Standard errno
 */
int ftl_write(uint32_t addr, const uint8_t *data, uint32_t size)
{
    int ret;
    struct eftl_device *dev;

    dev = eftl_get_device(addr);

    if (dev == NULL) {
        return -EINVAL;
    }

    eftl_lock(dev);
    ret = eftl_si_write(dev, addr, data, size);
    eftl_unlock(dev);

    return ret;
}

/**
 * @brief Write data in poll mode,just for psm.
 *
 * @param addr Write begining addr.
 * @param data Pointer to the src data.
 * @param size Write size in bytes.
 *
 * @return Standard errno
 */
int ftl_write_poll(uint32_t addr, const uint8_t *data, uint32_t size)
{
    int ret;
    struct eftl_device *dev;

    dev = eftl_get_device(addr);

    if (dev == NULL) {
        return -EINVAL;
    }

    dev->is_poll = 1;
    ret = eftl_si_write(dev, addr, data, size);
    dev->is_poll = 0;

    return ret;
}

/**
 * @brief Erase data.
 *
 * @param addr Erase begining addr.
 * @param size Erase size in bytes.
 *
 * @return Standard errno
 */
int ftl_erase(uint32_t addr, uint32_t size)
{
    int ret;
    struct eftl_device *dev;

    dev = eftl_get_device(addr);

    if (dev == NULL) {
        return -EINVAL;
    }

    eftl_lock(dev);
    ret = eftl_si_erase(dev, addr, size);
    eftl_unlock(dev);

    return ret;
}

/**
 * @brief Erase data,just for psm..
 *
 * @param addr Erase begining addr.
 * @param size Erase size in bytes.
 *
 * @return Standard errno
 */
int ftl_erase_poll(uint32_t addr, uint32_t size)
{
    int ret;
    struct eftl_device *dev;

    dev = eftl_get_device(addr);

    if (dev == NULL) {
        return -EINVAL;
    }

    dev->is_poll = 1;
    ret = eftl_si_erase(dev, addr, size);
    dev->is_poll = 0;

    return ret;
}

/**
 * @brief Init the ftl region.
 *
 * @param cfg Pointer to ftl cfg structure.
 * @param nr  Ftl region num.
 *
 * @return Standard errno
 */
int ftl_init(const struct eftl_cfg *cfg, uint32_t nr)
{
    return eftl_init(cfg, nr);
}

