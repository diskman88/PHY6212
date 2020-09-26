/*
 * Copyright (C) 2016 YunOS Project. All rights reserved.
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
#include <errno.h>
#include <stddef.h>

#include <zephyr.h>

#define BT_DBG_ENABLED 0

#include <common/log.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_driver.h>

#include <devices/device.h>
#include <devices/hci.h>
#include <hci_api.h>

#include <aos/kernel.h>

#define H4_NONE      0x00
#define H4_CMD       0x01
#define H4_ACL_UP    0x02
#define H4_SCO       0x03
#define H4_EVT       0x04
#define H4_ACL_DOWN  0x05

static int h4_open(void);
static int h4_send(struct net_buf *buf);

static struct bt_hci_driver drv = {
    .name       = "H4",
    .bus        = BT_HCI_DRIVER_BUS_VIRTUAL,
    .open       = h4_open,
    .send       = h4_send,
};

static struct {
    const struct bt_hci_driver *drv;
    char *dev_name;
    dev_t *dev;
} hci_h4 = {
    &drv,
    NULL,
    NULL,
};

static int h4_send(struct net_buf *buf)
{
    int ret;
    uint8_t type  = bt_buf_get_type(buf);

    if (type == BT_BUF_ACL_OUT) {
        net_buf_push_u8(buf, H4_ACL_DOWN);
        g_hci_debug_counter.acl_out_count++;
    } else if (type == BT_BUF_CMD) {
        net_buf_push_u8(buf, H4_CMD);
        g_hci_debug_counter.cmd_out_count++;
    } else {
        BT_ERR("Unknown buffer type");
        return -1;
    }

    BT_DBG("buf %p type %u len %u:%s", buf, type, buf->len, bt_hex(buf->data, buf->len));

    ret = hci_send(hci_h4.dev, buf->data, buf->len);

    net_buf_unref(buf);

    return ret;
}

static inline int is_adv_report_event(uint8_t *data, uint16_t len)
{
    return (data[0] == H4_EVT && data[1] == BT_HCI_EVT_LE_META_EVENT
            && data[3] == BT_HCI_EVT_LE_ADVERTISING_REPORT);
}

int hci_event_recv(uint8_t *data, uint16_t data_len)
{
    struct net_buf *buf;
    uint8_t *pdata = data;
    int32_t len = data_len;
    struct bt_hci_evt_hdr hdr;
    uint8_t sub_event = 0;
    uint8_t discardable = 0;

    if (pdata == NULL || len == 0) {
        return -1;
    }

    if (*pdata++ != H4_EVT) {
        goto err;
    }

    if (len < 3) {
        goto err;
    }

    hdr.evt = *pdata++;
    hdr.len = *pdata++;

    if (len < hdr.len + 3) {
        goto err;
    }

    if (hdr.evt == BT_HCI_EVT_LE_META_EVENT) {
        sub_event = *pdata++;

        if (sub_event == BT_HCI_EVT_LE_ADVERTISING_REPORT) {
            discardable = 1;
        }
    }

    if (hdr.evt == BT_HCI_EVT_CMD_COMPLETE ||
        hdr.evt  == BT_HCI_EVT_CMD_STATUS) {
        buf = bt_buf_get_cmd_complete(0);

        if (buf == NULL) {
            g_hci_debug_counter.event_in_is_null_count++;
            goto err;
        }
    }

    buf = bt_buf_get_rx(BT_BUF_EVT, 0);

    if (!buf && discardable) {
        g_hci_debug_counter.event_discard_count++;
        goto err;
    }

    if (!buf) {
        g_hci_debug_counter.event_in_is_null_count++;
        goto err;
    }

    bt_buf_set_type(buf, BT_BUF_EVT);

    net_buf_add_mem(buf, ((uint8_t *)(data)) + 1, hdr.len + sizeof(hdr));

    BT_DBG("event %s", bt_hex(buf->data, buf->len));
    g_hci_debug_counter.event_in_count++;

    if (bt_hci_evt_is_prio(hdr.evt)) {
        //bt_recv_prio(buf);
    } else {
        bt_recv(buf);
    }

    return 0;

err:
    return -1;
}

int hci_acl_recv(uint8_t *data, uint16_t data_len)
{
    struct net_buf *buf;
    uint8_t *pdata = data;
    int32_t len = data_len;
    uint16_t handle;
    uint16_t acl_len;

    if (pdata == NULL || len == 0) {
        return -1;
    }

    if (*pdata++ != H4_ACL_UP) {
        goto err;
    }

    if (len < 5) {
        goto err;
    }

    handle = ((*pdata + 1) << 16) | (*(pdata));
    pdata += 2;
    acl_len = ((*pdata + 1) << 16) | (*(pdata));
    pdata += 2;

    (void)handle;

    if (len < acl_len + 5) {
        goto err;
    }

    buf = bt_buf_get_rx(BT_BUF_ACL_IN, 0);

    if (!buf) {
        g_hci_debug_counter.hci_in_is_null_count++;
        goto err;
    }

    bt_buf_set_type(buf, BT_BUF_ACL_IN);

    net_buf_add_mem(buf, data + 1, acl_len + 4);
    g_hci_debug_counter.acl_in_count++;

    bt_recv(buf);
    return 0;

err:
    return -1;
}

static int h4_open(void)
{
    hci_h4.dev = hci_open_id(hci_h4.dev_name, 0);

    if (hci_h4.dev == NULL) {
        BT_ERR("device open fail");
        return -1;
    }

    return 0;
}

int hci_driver_init(char *name)
{
    int ret;

    hci_h4.dev_name = name;

    ret = bt_hci_driver_register(&drv);

    if (ret) {
        return ret;
    }

    return 0;
}
