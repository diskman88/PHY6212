#include <yoc_config.h>
#include <aos/log.h>
#include <aos/kernel.h>
#include <aos/aos.h>
#include <aos/ble.h>
#include <errno.h>
#include "ll_def.h"
#include "yoc/uart_server.h"

#include "gap.h"
#include "atvv_service.h"
#include "../app_msg.h"


static atvv_service_t atvv;

/** ATV Voice Service  */
// service gatt index
enum ATVV_GATT_IDX {
    ATVV_IDX_SVC,

    ATVV_IDX_TX_CHAR,
    ATVV_IDX_TX_VAL,

    ATVV_IDX_RX_CHAR,
    ATVV_IDX_RX_VAL,
    ATVV_IDX_RX_CCC,

    ATVV_IDX_CTL_CHAR,
    ATVV_IDX_CTL_VAL,
    ATVV_IDX_CTL_CCC,

    ATVV_IDX_MAX,
};

// service uuid
#define ATVV_SERVICE_UUID       UUID128_DECLARE(0x64, 0xB6, 0x17, 0xF6, 0x01, 0xAF, 0x7D, 0xBC, 0x05, 0x4F, 0x21, 0x5A, 0x01, 0x00, 0x5E, 0xAB)
#define ATVV_TX_UUID            UUID128_DECLARE(0x64, 0xB6, 0x17, 0xF6, 0x01, 0xAF, 0x7D, 0xBC, 0x05, 0x4F, 0x21, 0x5A, 0x02, 0x00, 0x5E, 0xAB)
#define ATVV_RX_UUID            UUID128_DECLARE(0x64, 0xB6, 0x17, 0xF6, 0x01, 0xAF, 0x7D, 0xBC, 0x05, 0x4F, 0x21, 0x5A, 0x03, 0x00, 0x5E, 0xAB)
#define ATVV_CTL_UUID           UUID128_DECLARE(0x64, 0xB6, 0x17, 0xF6, 0x01, 0xAF, 0x7D, 0xBC, 0x05, 0x4F, 0x21, 0x5A, 0x04, 0x00, 0x5E, 0xAB)
// ble stack service data
static struct bt_gatt_ccc_cfg_t g_atvv_cccd_cfg1[1] = {};
static struct bt_gatt_ccc_cfg_t g_atvv_cccd_cfg2[1] = {};
static  gatt_service g_atvv_service;
// static char ctl_char_des[] = "audio control";
// static char data_char_des[] = "audio data";
// attribute table
gatt_attr_t atvv_attrs[ATVV_IDX_MAX] = {
    [ATVV_IDX_SVC] = GATT_PRIMARY_SERVICE_DEFINE(ATVV_SERVICE_UUID),

    [ATVV_IDX_TX_CHAR] = GATT_CHAR_DEFINE(ATVV_TX_UUID,  GATT_CHRC_PROP_WRITE),
    [ATVV_IDX_TX_VAL] = GATT_CHAR_VAL_DEFINE(ATVV_TX_UUID, GATT_PERM_WRITE),

    [ATVV_IDX_RX_CHAR] = GATT_CHAR_DEFINE(ATVV_RX_UUID,  GATT_CHRC_PROP_READ | GATT_CHRC_PROP_NOTIFY),
    [ATVV_IDX_RX_VAL] = GATT_CHAR_VAL_DEFINE(ATVV_RX_UUID, GATT_PERM_READ),
    [ATVV_IDX_RX_CCC] = GATT_CHAR_CCC_DEFINE(g_atvv_cccd_cfg1),

    [ATVV_IDX_CTL_CHAR] = GATT_CHAR_DEFINE(ATVV_CTL_UUID, GATT_CHRC_PROP_READ | GATT_CHRC_PROP_NOTIFY),
    [ATVV_IDX_CTL_VAL] = GATT_CHAR_VAL_DEFINE(ATVV_CTL_UUID, GATT_PERM_READ),
    [ATVV_IDX_CTL_CCC] = GATT_CHAR_CCC_DEFINE(g_atvv_cccd_cfg2)
};

static void atvv_cccd_change(evt_data_gatt_char_ccc_change_t *event_data)
{
    int16_t idx;
    idx = event_data->char_handle - atvv.svc_handle;
    switch(idx) {
        case ATVV_IDX_RX_CCC:
            LOGI("AUDIO", "ATVV_IDX_RX_CCC change:%x", event_data->ccc_value);
            atvv.char_rx_cccd = event_data->ccc_value;
            break;
        case ATVV_IDX_CTL_CCC:
            LOGI("AUDIO", "ATVV_IDX_CTL_CCC change:%x", event_data->ccc_value);
            atvv.char_ctl_cccd = event_data->ccc_value;
            break;
        default:
            break;
    }
}

static void atvv_char_write(evt_data_gatt_char_write_t *event_data)
{
    int16_t idx;
    idx = event_data->char_handle - atvv.svc_handle;
    if (idx == ATVV_IDX_TX_VAL) {
        app_msg_t msg;
        // ATV:mic open
        if (event_data->data[0] == 0x0C) {
            msg.type = MSG_BT_GATT,
            msg.subtype = MSG_BT_GATT_ATV_MIC_OPEN;
            msg.param = 0;
            app_send_message(&msg);
        }
        // ATV:mic stop
        if (event_data->data[0] == 0x0D) {
            msg.type = MSG_BT_GATT;
            msg.subtype = MSG_BT_GATT_ATV_MIC_STOP;
            msg.param = 0;
            app_send_message(&msg);
        }

        // ATV: get_caps
        if (event_data->data[0] == 0x0A) {
            const uint8_t caps[3] = {0x0B, 0x01, 0x01};
            atvv_ctl_send(caps, 3);
        }
        LOGI("ATVV", "write:len = %d, data[0]=0x%x", event_data->len, event_data->data[0]);
    }
}

static void atvv_char_read(evt_data_gatt_char_read_t *event_data)
{
    // int16_t idx;
    // LOGI("ATVV", "GATT read, handle=0x%x,atvv_handle=0x%x", event_data->char_handle, atvv.svc_handle);
    
    // idx = event_data->char_handle - atvv.svc_handle;
    // if (idx == ATVV_IDX_RX_VAL) {
    //     event_data->data = read_test_data;
    //     event_data->len = 4;
    //     LOGI("ATVV", "read:len = %d, data[0] = 0x%x", event_data->len, event_data->data[0]);
    // }
}

static int atvv_event_callback(ble_event_en event, void *event_data)
{
    switch (event) {
        // case EVENT_GAP_CONN_CHANGE:
        //     conn_change(event, event_data);
        //     break;
        // case EVENT_GATT_MTU_EXCHANGE:
        //     mtu_exchange(event, event_data);
        //     break;
        case EVENT_GATT_CHAR_READ:
            atvv_char_read(event_data);
            break;
        case EVENT_GATT_CHAR_WRITE:
            atvv_char_write(event_data);
            break;
        case EVENT_GATT_CHAR_CCC_CHANGE:    
            atvv_cccd_change(event_data);
            break;
        default:
            break;
    }

    return 0;
}

static ble_event_cb_t ble_cb = {
    .callback = atvv_event_callback,
};

atvv_service_t * atvv_service_init()
{
    int ret = 0;

    ret = ble_stack_event_register(&ble_cb);

    if (ret) {
        return NULL;
    }
    
    atvv.svc_handle = ble_stack_gatt_registe_service(&g_atvv_service, atvv_attrs,  BLE_ARRAY_NUM(atvv_attrs));

    if (atvv.svc_handle < 0) {
        LOGI("ATVV", "atvv service registe failed, ret = %x", ret);
        return NULL;
    }

    return &atvv;
}



int atvv_voice_send(const uint8_t * p_voice_data, uint16_t len)
{
    if (atvv.char_rx_cccd & CCC_VALUE_NOTIFY) {
        return ble_stack_gatt_notificate(g_gap_data.conn_handle, atvv.svc_handle + ATVV_IDX_RX_VAL, p_voice_data, len);
    } else {
        return ATT_ERR_WRITE_NOT_PERMITTED;
    }
}

int atvv_ctl_send(const uint8_t * p_data, uint16_t len) 
{
    if (atvv.char_ctl_cccd & CCC_VALUE_NOTIFY) {
        return ble_stack_gatt_notificate(g_gap_data.conn_handle, atvv.svc_handle + ATVV_IDX_CTL_VAL, p_data, len);
    } else {
        return ATT_ERR_WRITE_NOT_PERMITTED;
    }
}
/*
int uart_server_adv_control(uint8_t adv_on, adv_param_t *adv_param)
{
    int ret = 0;

    if (llState == LL_STATE_ADV_UNDIRECTED || llState == LL_STATE_ADV_DIRECTED \
        || llState == LL_STATE_ADV_SCAN || llState == LL_STATE_ADV_NONCONN) {
        ret = ble_stack_adv_stop();

        if (ret) {
            return ret;
        }
    }

    if (adv_on) {
        if (adv_param == NULL) {
            return -1;
        } else {
            ret = ble_stack_adv_start(adv_param);

            if (ret) {
                return ret;
            }
        }
    }

    return 0;
}



int uart_server_conn_param_update(uart_handle_t handle, conn_param_t *param)
{
    ble_uart_server_t *uart = (ble_uart_server_t *)handle;

    if (!uart) {
        return -1;
    }

    uart->conn_param->interval_min = param->interval_min;
    uart->conn_param->interval_max = param->interval_max;
    uart->conn_param->latency = param->latency;
    uart->conn_param->timeout = param->timeout;

    if (uart->conn_handle < 0) {
        uart->update_param_flag = 1;

    } else {
        return ble_stack_connect_param_update(uart->conn_handle, param);
    }

    return 0;
}




int uart_server_send(uart_handle_t handle, const       char *data, int length, bt_uart_send_cb *cb)
{

    uint32_t count = length;
    uint16_t wait_timer = 0;
    int ret = 0;
    ble_uart_server_t *uart = (ble_uart_server_t *)handle;

    if (!data || !length || !uart || uart->conn_handle < 0 || uart->server_data.tx_ccc_value != CCC_VALUE_NOTIFY) {
        return -1;
    }

    if (cb != NULL && cb->start != NULL) {
        cb->start(0, NULL);
    }

    while (count) {
        uint16_t send_count = (uart->mtu - 3) < count ? (uart->mtu - 3) : count;
        ret = ble_stack_gatt_notificate(uart->conn_handle, uart->uart_svc_handle + YOC_UART_IDX_TX_VAL, (uint8_t *)data, send_count);

        if (ret == -ENOMEM) {
            wait_timer++;

            if (wait_timer >= 100) {
                cb->end(-1, NULL);
                return -1;
            }

            aos_msleep(1);

            continue;
        }

        if (ret) {
            if (cb != NULL && cb->end != NULL) {
                cb->end(ret, NULL);
            }

            return ret;
        }

        data += send_count;
        count -= send_count;
    }

    if (cb != NULL && cb->end != NULL) {
        cb->end(0, NULL);
    }

    return 0;

}
*/