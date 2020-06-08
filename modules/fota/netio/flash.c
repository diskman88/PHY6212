/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include <aos/aos.h>
#include <yoc/netio.h>
#include <yoc/partition.h>
#include <stdio.h>
#include <aos/log.h>

#define TAG "fota"

static int g_fota_first_write = 1;
static char g_scramble[4];

static int flash_open(netio_t *io, const char *path)
{
    partition_t handle = partition_open(path + sizeof("flash://") - 1);

    if (handle >= 0) {
        hal_logic_partition_t *lp = hal_flash_get_info(handle);
        aos_assert(lp);

        io->size = lp->length;
        io->block_size = lp->sector_size;

        io->private = (void *)handle;

        return 0;
    }

    return -1;
}

static int flash_close(netio_t *io)
{
    partition_t handle = (partition_t)io->private;
    partition_close(handle);

    return 0;
}

static int flash_read(netio_t *io, uint8_t *buffer, int length, int timeoutms)
{
    partition_t handle = (partition_t)io->private;

    if (io->size - io->offset < length) {
        length = io->size - io->offset;
    }

    if (partition_read(handle, io->offset, buffer, length) >= 0) {
        io->offset += length;
        return length;
    }

    return -1;
}

static int fota_flash_erase(partition_t partition, off_t off_set, uint32_t block_count)
{
    char read_buffer[512];
    int i, ret = 0;

    if (g_fota_first_write == 0) {
        if (partition_read(partition, off_set, read_buffer, 512) < 0) {
            LOGD(TAG, "0 read addr:%x length:%x\n", off_set, 512);
            return -1;
        }

        for (i = 0; i < 512 / 4; i++) {
            if (memcmp(read_buffer + i * 4, g_scramble, 4) != 0) {
                LOGD(TAG, "0 read check fail %x %x %x %x\n",
                     *(char *)(read_buffer + i * 4), *(char *)(read_buffer + i * 4 + 1), *(char *)(read_buffer + i * 4 + 2), *(char *)(read_buffer + i * 4 + 3));
                ret = -1;
                break;
            }
        }

        if (ret != 0) {
            if (partition_erase(partition, off_set, block_count) < 0) {
                LOGD(TAG, "0 erase addr:%x length:%x\n", off_set, block_count);
                return -1;
            }
        }
    } else {
        if (partition_erase(partition, off_set, block_count) < 0) {
            LOGD(TAG, "1 erase addr:%x length:%x\n", off_set, block_count);
            return -1;
        }

        if (partition_read(partition, off_set, read_buffer, 512) < 0) {
            LOGD(TAG, "1 read addr:%x length:%x\n", off_set, 512);
            return -1;
        }

        memcpy(g_scramble, read_buffer, 4);
        LOGD(TAG, "first read scramble: %x %x %x %x\n", g_scramble[0], g_scramble[1], g_scramble[2], g_scramble[3]);
        g_fota_first_write = 0;
    }

    return 0;
}

static int flash_write(netio_t *io, uint8_t *buffer, int length, int timeoutms)
{
    partition_t handle = (partition_t)io->private;

    if (io->size - io->offset < length) {
        length = io->size - io->offset;
    }

    // LOGD(TAG, "length %d\n", length);
    if (fota_flash_erase(handle, io->offset + (io->block_size << 1), (length + io->block_size - 1) / io->block_size) < 0) {
        LOGD(TAG, "erase addr:%x length:%x\n", io->offset + (io->block_size << 1), (length + io->block_size - 1) / io->block_size);

        return -1;
    }

    if (partition_write(handle, io->offset + (io->block_size << 1), buffer, length) >= 0) {
        LOGD(TAG, "=====>%x, %d\n", io->offset, length);
        io->offset += length;
        return length;
    }

    LOGD(TAG, "write fail addr:%x length:%x\n", io->offset + (io->block_size << 1), length);
    return -1;
}

static int flash_seek(netio_t *io, size_t offset, int whence)
{
    // partition_t handle = (partition_t)io->private;

    switch (whence) {
        case SEEK_SET:
            io->offset = offset;
            return 0;

        case SEEK_CUR:
            io->offset += offset;
            return 0;

        case SEEK_END:
            io->offset = io->size - offset;
            return 0;
    }

    return -1;
}

const netio_cls_t flash = {
    .name = "flash",
    .open = flash_open,
    .close = flash_close,
    .write = flash_write,
    .read = flash_read,
    .seek = flash_seek,
};

int netio_register_flash(void)
{
    return netio_register(&flash);
}