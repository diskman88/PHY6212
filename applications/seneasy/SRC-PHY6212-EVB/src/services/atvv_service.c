#include <yoc_config.h>
#include <aos/log.h>
#include <aos/kernel.h>
#include <aos/aos.h>
#include <aos/ble.h>
#include <errno.h>
#include "ll_def.h"
#include "yoc/uart_server.h"

#include "atvv_service.h"

typedef struct _atv_voice_t
{
    uint16 conn_handle;
    uint16 svc_handle;
    uint16 cccd;
}_atv_voice_t;

static _atv_voice_t atvv;

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

    ATVV_IDX_MAX,
};

// service uuid
#define ATVV_SERVICE_UUID       UUID128_DECLARE(0x64, 0xB6, 0x17, 0xF6, 0x01, 0xAF, 0x7D, 0xBC, 0x05, 0x4F, 0x21, 0x5A, 0x01, 0x00, 0x5E, 0xAB)
#define ATVV_TX_UUID            UUID128_DECLARE(0x64, 0xB6, 0x17, 0xF6, 0x01, 0xAF, 0x7D, 0xBC, 0x05, 0x4F, 0x21, 0x5A, 0x02, 0x00, 0x5E, 0xAB)
#define ATVV_RX_UUID            UUID128_DECLARE(0x64, 0xB6, 0x17, 0xF6, 0x01, 0xAF, 0x7D, 0xBC, 0x05, 0x4F, 0x21, 0x5A, 0x03, 0x00, 0x5E, 0xAB)
#define ATVV_CTL_UUID           UUID128_DECLARE(0x64, 0xB6, 0x17, 0xF6, 0x01, 0xAF, 0x7D, 0xBC, 0x05, 0x4F, 0x21, 0x5A, 0x04, 0x00, 0x5E, 0xAB)
// ble stack service data
static struct bt_gatt_ccc_cfg_t g_atvv_cccd[2] = {};
static  gatt_service g_atvv_service;
// static char ctl_char_des[] = "audio control";
// static char data_char_des[] = "audio data";
// attribute table
gatt_attr_t atvv_attrs[ATVV_IDX_MAX] = {
    [ATVV_IDX_SVC] = GATT_PRIMARY_SERVICE_DEFINE(ATVV_SERVICE_UUID),

    [ATVV_IDX_TX_CHAR] = GATT_CHAR_DEFINE(ATVV_TX_UUID,  GATT_CHRC_PROP_WRITE),
    [ATVV_IDX_TX_VAL] = GATT_CHAR_VAL_DEFINE(ATVV_TX_UUID, GATT_PERM_WRITE),

    [ATVV_IDX_RX_CHAR] = GATT_CHAR_DEFINE(ATVV_RX_UUID,  GATT_CHRC_PROP_NOTIFY | GATT_CHRC_PROP_READ),
    [ATVV_IDX_RX_VAL] = GATT_CHAR_VAL_DEFINE(YOC_UART_TX_UUID, GATT_PERM_READ),
    [ATVV_IDX_RX_CCC] = GATT_CHAR_CCC_DEFINE(g_atvv_cccd),

    [ATVV_IDX_CTL_CHAR] = GATT_CHAR_DEFINE(ATVV_CTL_UUID, GATT_CHRC_PROP_READ | GATT_CHRC_PROP_WRITE),
    [ATVV_IDX_CTL_VAL] = GATT_CHAR_VAL_DEFINE(ATVV_CTL_UUID, GATT_PERM_READ | GATT_PERM_READ),
};

static void atvv_cccd_change(uint16_t index, void *event_data)
{
    evt_data_gatt_char_ccc_change_t *e = (evt_data_gatt_char_ccc_change_t *)event_data;

    LOGI("AUDIO", "cccd change:%x", e->ccc_value);
}

static void atvv_char_write(uint16_t index, void *event_data)
{
    evt_data_gatt_char_write_t *e = (evt_data_gatt_char_write_t *)event_data;

}

static void atvv_char_read(uint16_t index, void *event_data)
{

}

static int atvv_event_callback(ble_event_en event, void *event_data)
{
    int16_t idx;
    switch (event) {
        // case EVENT_GAP_CONN_CHANGE:
        //     conn_change(event, event_data);
        //     break;

        // case EVENT_GATT_MTU_EXCHANGE:
        //     mtu_exchange(event, event_data);
        //     break;
        case EVENT_GATT_CHAR_READ:
            idx = ((evt_data_gatt_char_read_t *)event_data)->char_handle - atvv.svc_handle;
            if (idx >= 0 && idx < ATVV_IDX_MAX) {
                atvv_char_read(idx, event_data);
            }
            break;
        case EVENT_GATT_CHAR_WRITE:
            idx = ((evt_data_gatt_char_write_t *)event_data)->char_handle - atvv.svc_handle;
            if (idx >= 0 && idx < ATVV_IDX_MAX) {
                atvv_char_write(idx, event_data);
            }
            break;

        case EVENT_GATT_CHAR_CCC_CHANGE:    
            idx = ((evt_data_gatt_char_ccc_change_t *)event_data)->char_handle - atvv.svc_handle;
            if (idx >= 0 && idx < ATVV_IDX_MAX) {
                atvv_cccd_change(idx, event_data);
            }
            break;
        default:
            break;
    }

    return 0;
}

static ble_event_cb_t ble_cb = {
    .callback = atvv_event_callback,
};

atv_voice_handle_t atvv_service_init()
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

    atvv.conn_handle = -1;

    return &atvv;
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