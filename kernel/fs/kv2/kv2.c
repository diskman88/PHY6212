#include <stdio.h>
#include <stdlib.h>

#include <devices/flash.h>
#include <yoc/partition.h>
#include <aos/debug.h>

#include "block.h"
#include "kvset.h"

static kv_t g_kv = {NULL, 0, 0, 0, -1, NULL, NULL};

static int kv_flash_erase(kvblock_t *block, int pos, int size)
{
    int ret = pos + size <= block->size;
    int err;
    if (ret)
    {
        err = partition_erase(block->kv->handle, block->id * block->size + pos, 1);
        if (err)
        {
            return -1;
        }
    }

    return ret;
}

static int kv_flash_write(kvblock_t *block, int pos, void *data, int size)
{
    int ret = pos + size <= block->size;
    int err;
    if (ret) {
       err = partition_write(block->kv->handle, block->id * block->size + pos, data, size);
       if (err) {
          return -1;
       }
    }
    return ret;
}

static int kv_flash_read(kvblock_t *block, int pos, void *data, int size)
{
    int ret;
    ret = partition_read(block->kv->handle, block->id * block->size + pos, data, size);
    return ret;
}

static flash_ops_t flash_ops = {
    .write = kv_flash_write,
    .erase = kv_flash_erase,
    .read = kv_flash_read
};

int kv2x_init(kv_t *kv, const char *partition)
{
    memset(kv, 0, sizeof(kv_t));
    kv->handle = partition_open(partition);
    kv->ops    = &flash_ops;

    if (kv->handle >= 0) {
        hal_logic_partition_t *lp = hal_flash_get_info(kv->handle);
        aos_assert(lp);

        uint8_t *mem        = (uint8_t *)(lp->start_addr + lp->base_addr);
        int      block_size = lp->sector_size;
        int      block_num  = lp->length / lp->sector_size;

        return kv_init(kv, mem, block_num, block_size);
    }

    return -1;
}

static aos_mutex_t kv_lock;
int aos_kv_init(const char *partname)
{
    aos_mutex_new(&kv_lock);
    return kv2x_init(&g_kv, partname);
}

int __kv_setdata(char *key, char *buf, int bufsize)
{
    if (g_kv.handle < 0) {
        return -1;
    }

    aos_mutex_lock(&kv_lock, -1);
    int ret = kv_set(&g_kv, key, buf, bufsize) >= 0 ? 0 : -1;
    aos_mutex_unlock(&kv_lock);

    return ret;
}

int __kv_getdata(char *key, char *buf, int bufsize)
{
    if (g_kv.handle < 0) {
        return -1;
    }

    if (key == NULL || buf == NULL || bufsize <= 0)
        return -1;
    aos_mutex_lock(&kv_lock, -1);
    int ret = kv_get(&g_kv, key, buf, bufsize);
    aos_mutex_unlock(&kv_lock);

    return ret;
}

int __kv_del(char *key)
{
    if (g_kv.handle < 0) {
        return -1;
    }

    aos_mutex_lock(&kv_lock, -1);
    int ret = kv_rm(&g_kv, key);
    aos_mutex_unlock(&kv_lock);

    return ret;
}

int __kv_reset(void)
{
    if (g_kv.handle < 0) {
        return -1;
    }

    aos_mutex_lock(&kv_lock, -1);
    int ret = kv_reset(&g_kv);
    aos_mutex_unlock(&kv_lock);

    return ret;
}

void __kv_dump()
{
    if (g_kv.handle < 0) {
        return;
    }

    aos_mutex_lock(&kv_lock, -1);
    kv_dump(&g_kv);
    aos_mutex_unlock(&kv_lock);
}

void __show_data()
{
    if (g_kv.handle < 0) {
        return;
    }

    aos_mutex_lock(&kv_lock, -1);
    kv_show_data(&g_kv);
    aos_mutex_unlock(&kv_lock);
}

static int _iter_list(kvnode_t *node, void *p)
{
    printf("%s: %s\n", \
            KVNODE_OFFSET2CACHE(node, head_offset),
            KVNODE_OFFSET2CACHE(node, value_offset)
    );
    return 0;
}

struct kv_foreach_func_t {
    void (*func)(char *key, char *val, uint16_t val_size, void *arg);
    void *arg;
};

static int _iter_foreach(kvnode_t *node, void *p)
{
    struct kv_foreach_func_t *func = p;

    if (func && func->func)
    {
        func->func((char *)KVNODE_OFFSET2CACHE(node, head_offset), (char *)KVNODE_OFFSET2CACHE(node, value_offset), node->val_size, func->arg);
    }
    return 0;
}

void __kv_foreach(void (*func)(char *key, char *val, uint16_t val_size, void *arg), void *arg)
{
    if (g_kv.handle < 0) {
        return;
    }

    struct kv_foreach_func_t cb_func = {0};
    cb_func.func = func;
    cb_func.arg = arg;

    aos_mutex_lock(&kv_lock, -1);
    kv_iter(&g_kv, _iter_foreach, &cb_func);
    aos_mutex_unlock(&kv_lock);
}

void __kv_list()
{
    if (g_kv.handle < 0) {
        return;
    }

    aos_mutex_lock(&kv_lock, -1);
    kv_iter(&g_kv, _iter_list, NULL);
    aos_mutex_unlock(&kv_lock);
}

/**
 * This function will get data from the factory setting area.
 *
 * @param[in]   key   the data pair of the key, less than 64 bytes
 * @param[in]   size  the size of the buffer
 * @param[out]  buf   the buffer that will store the data
 * @return  the length of the data value, error code otherwise
 */
int nvram_get_val(const char *key, void *buf, int size)
{
    memset(buf, 0, size);

    return __kv_getdata((char *)key, buf, size - 1);
}

/**
 * This function will set data to the factory setting area.
 *
 * @param[in]   key   the data pair of the key, less than 64 bytes
 * @param[in]   value the data pair of the value, delete the pair if value == NULL
 * @return  the length of the data value, error code otherwise
 */
int nvram_set_val(const char *key, char *value)
{
    aos_mutex_lock(&kv_lock, -1);
    int ret = kv_set(&g_kv, key, value, strlen(value));
    aos_mutex_unlock(&kv_lock);

    return ret;
}
