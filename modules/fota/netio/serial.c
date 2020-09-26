/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <yoc/fota.h>
#include <yoc/network.h>
#include "../http/http.h"
#include <yoc/netio.h>
#include <yoc/atserver.h>
#include <yoc/at_cmd.h>
#include <aos/log.h>
#include <aos/kernel.h>
#include <aos/debug.h>
#include <crc16.h>

#define TAG "fota-serial"

typedef struct {
    uint8_t *buffer;
    size_t buf_size;
    size_t recv_len;
    size_t len_need;
    size_t uart_recv_len;
} recv_t;

static recv_t g_recv;
static aos_sem_t sem;

#define AT_OK() atserver_send("\r\nOK\r\n")
#define AT_ERR() atserver_send("\r\nERROR\r\n")

#define AT_OTAREQ() atserver_send("\r\n+OTAREQ\r\n")

static void pass_th_cb(dev_t *dev)
{
    int recv_size, offset;
    int ret;

    if (g_recv.buffer == NULL ||
        g_recv.len_need == 0 ||
        dev == NULL) {
        /*can't passthrough, to exit pass mode */
        atserver_exit_passthrough();
        return;
    }
    recv_size = g_recv.len_need - g_recv.uart_recv_len;
    offset = g_recv.uart_recv_len;
    ret = uart_recv(dev, g_recv.buffer + offset, recv_size, 0);
    g_recv.uart_recv_len += ret;
    if (g_recv.uart_recv_len >= g_recv.len_need) {
        g_recv.recv_len = g_recv.len_need;
        atserver_exit_passthrough();
        aos_sem_signal(&sem);
        return;
    }
    return;
}

void at_cmd_otapost(char *cmd, int type, char *data)
{
    LOGD(TAG, "at_cmd_otapost");
    g_recv.len_need = 0;
    g_recv.uart_recv_len = 0;

    if (type == WRITE_CMD) {
        int len;
        atserver_scanf("%d", &len);
        LOGD(TAG, "pass len:%d", len);
        if (len > g_recv.buf_size) {
            AT_ERR();
            return;
        }
        g_recv.len_need = len;
        g_recv.recv_len = 0;
        atserver_passthrough_cb_register(pass_th_cb);
        AT_OK();
    }
}

static int serial_read(netio_t *io, uint8_t *buffer, int length, int timeoutms)
{
    int ret;
    uint16_t r_crc16, calc_crc16;
    int content_len = 0;
    aos_check_return_einval(io && buffer && length);
    LOGD(TAG, "serial_read, timeoutms:%d", timeoutms);
    g_recv.buffer = buffer;
    g_recv.buf_size = length;

    AT_OTAREQ();

    ret = aos_sem_wait(&sem, timeoutms);
    LOGD(TAG, "sem_wait ret:%d", ret);
    if (ret != 0) {
        LOGD(TAG, "recv timeout...");
        goto _err;
    }
    if (g_recv.recv_len > length) {
        LOGE(TAG, "size too long");
        goto _err;
    } else if (g_recv.recv_len <= 2) {
        LOGE(TAG, "size too short");
        goto _err;
    }
    content_len = g_recv.recv_len;
    LOGD(TAG, "content_len:%d", content_len);

    // dump_data(buffer, content_len);
    // crc16 verify
    r_crc16 = g_recv.buffer[g_recv.recv_len - 2];
    r_crc16 <<= 8;
    r_crc16 |= g_recv.buffer[g_recv.recv_len - 1];
    LOGD(TAG, "read crc16:0x%x", r_crc16);

    calc_crc16 = crc16(0, g_recv.buffer, g_recv.recv_len - 2);
    LOGD(TAG, "calc crc16:0x%x", calc_crc16);
    if (calc_crc16 != r_crc16) {
        LOGE(TAG, "crc verify error");
        goto _err;
    }
    content_len -= 2;   // remove crc16
    AT_OK();
    return content_len;
_err:
    g_recv.recv_len = 0;
    AT_ERR();
    return -1;
}

static int serial_open(netio_t *io, const char *path)
{
    aos_check_return_einval(io && path);
    LOGD(TAG, "serial_open");
    memset(&g_recv, 0, sizeof(recv_t));
    aos_sem_new(&sem, 0);
    return 0;
}

static int serial_seek(netio_t *io, size_t offset, int whence)
{
    LOGD(TAG, "serial_seek, %d", offset);
    aos_check_return_einval(io);
    io->offset = offset;

    return 0;
}

static int serial_close(netio_t *io)
{
    aos_check_return_einval(io);
    memset(&g_recv, 0, sizeof(recv_t));
    LOGD(TAG, "serial_close");
    aos_sem_free(&sem);
    atserver_exit_passthrough();
    return 0;
}

static int serial_write(netio_t *io, uint8_t *buffer, int length, int timeoutms)
{
    LOGD(TAG, "serial_write");
    return 0;
}

const netio_cls_t serial_cls = {
    .name = "serial",
    .read = serial_read,
    .seek = serial_seek,
    .open = serial_open,
    .close = serial_close,
    .write = serial_write
};

int netio_register_serial(void)
{
    return netio_register(&serial_cls);
}
