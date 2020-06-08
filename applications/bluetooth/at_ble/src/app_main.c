/*
 * Copyright (C) 2017 C-SKY Microsystems Co., All rights reserved.
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

#include <yoc_config.h>
#include <stdio.h>
#include <aos/log.h>
#include <aos/kernel.h>
#include <aos/ble.h>
#include <app_init.h>
#include <gpio.h>
#include <errno.h>
#include <at_ble.h>
#include <drv_gpio.h>
#include "uart_server.h"
#include "uart_client.h"


#define TAG "AT MODULE"
#define GET_MAX_MAC_SIZE 5


typedef void *gpio_pin_handle_t;
typedef void (*uart_service_init_cb)(int err);

typedef struct {
    uint8_t  val[6];
} bt_addr_t;

extern at_server_handler *g_at_ble_handler;
uart_handle_t g_uart_handler = NULL;
extern uint8_t at_server_mode;
extern gpio_pin_handle_t g_connect_info, g_mode_ctrl, g_sleep_ctrl;

static aos_sem_t g_sync_sem;

typedef union _uart_service_t {
    ble_uart_client_t client;
    ble_uart_server_t server;
} uart_service_t;

uart_service_t g_uart;
typedef char *at_head;

static adv_param_t *adv_param_temp = NULL;
static uint8_t g_adv_enable = 0;

int at_mode_send(at_head head, const char *data, size_t size)
{
    int ret;

    if (head != NULL) {
        ret = atserver_rawdata_send(head, strlen(head));

        if (ret) {
            return -1;
        }
    }

    if (data != NULL && size > 0) {
        ret = atserver_rawdata_send(data, size);
    } else {
        return -1;
    }

    return ret;
}

int uart_mode_send(const char *data, size_t size)
{
    if (data == NULL || size == 0) {
        return -1;
    }

    return atserver_rawdata_send(data, size);
}

static int send_dev_info(uint8_t cnt)
{
    uint8_t dev_list_info[25] = {0};

    if (cnt == 0) {
        return -1;
    }

    dev_addr_t *addr = NULL;
    int i = 0;
    uint8_t size = MIN(cnt, GET_MAX_MAC_SIZE);

    do {
        addr = found_dev_get();

        if (addr) {
            snprintf((char *)(dev_list_info), sizeof(dev_list_info), "%02x:%02x:%02x:%02x:%02x:%02x,%d;", \
                     addr->val[5], addr->val[4], addr->val[3], addr->val[2], addr->val[1], addr->val[0], addr->type);

            if (!i) {
                at_mode_send("\r\n+BTFIND:", (char *)dev_list_info, strlen((char *)dev_list_info));
            } else {
                at_mode_send("\r\n", (char *)dev_list_info, strlen((char *)dev_list_info));
            }
        }

    } while (addr && i++ < size);

    if (!i) {
        return -1;
    }

    return 0;
}

static int atmode_recv(uint8_t event, char *data, int length, at_bt_uart_send_cb *cb)
{
    int ret = 0;

    switch (event) {
        case AT_BT_ADV_ON:
            if (data) {
                adv_param_temp = (adv_param_t *)data;
            }

            g_adv_enable = 1;
            aos_sem_signal(&g_sync_sem);
            break;

        case AT_BT_ADV_OFF:
            ret = uart_server_adv_control(ADV_OFF, NULL);
            break;

        case AT_BT_CONNECT:
            ret = uart_client_conn((dev_addr_t *)data, NULL);
            break;

        case AT_BT_TX_DATA: {
            if (g_at_ble_handler->at_config->role == MASTER) {
                ret = uart_client_send(g_uart_handler, data, length, (bt_uart_send_cb *)cb);
            } else {
                ret = uart_server_send(g_uart_handler, data, length, (bt_uart_send_cb *)cb);
            }
        }
        break;

        case AT_BT_DEV_FIND: {
            uint8_t size = GET_MAX_MAC_SIZE;
            return send_dev_info(size);
        }
        break;

        case AT_BT_CONNECT_UPDATE: {
            if (g_at_ble_handler->at_config->role == MASTER) {
                ret = uart_client_conn_param_update(g_uart_handler, (conn_param_t *)data);
            } else {
                ret = uart_server_conn_param_update(g_uart_handler, (conn_param_t *)data);
            }

        }
        break;

        case AT_BT_DISCONNECT: {
            if (g_at_ble_handler->at_config->role == MASTER) {
                ret = uart_client_disconn(g_uart_handler);
            } else {
                ret = uart_server_disconn(g_uart_handler);
            }
        }
        break;

        case AT_BT_CONN_INFO_GET: {
            uint8_t conn_info[37];
            int16_t conn_handle = -1;

            if (g_at_ble_handler->at_config->role == SLAVE) {
                ble_uart_server_t *server_handle = NULL;
                server_handle = (ble_uart_server_t *)g_uart_handler;

                if (!server_handle || server_handle->conn_handle == -1) {
                    return -1;
                }

                conn_handle = server_handle->conn_handle;
            } else {
                ble_uart_client_t *client_handle = NULL;
                client_handle = (ble_uart_client_t *)g_uart_handler;

                if (!client_handle || client_handle->conn_handle == -1) {
                    return -1;
                }

                conn_handle = client_handle->conn_handle;
            }

            connect_info_t info = {0};
            ret = ble_stack_connect_info_get(conn_handle, &info);

            if (ret) {
                return -1;
            }

            snprintf((char *)conn_info, sizeof(conn_info), "BTCONNINFO:%02x:%02x:%02x:%02x:%02x:%02x,%d", \
                     info.peer_addr.val[5], info.peer_addr.val[4], info.peer_addr.val[3], info.peer_addr.val[2],
                     info.peer_addr.val[1], info.peer_addr.val[0], info.peer_addr.type);
            at_mode_send("\r\n+", (char *)conn_info, strlen((char *)conn_info));

        }
        break;

        default:
            break;
    }

    return ret;
}

static int uartmode_recv(char *data, int length, at_bt_uart_send_cb *cb)
{
    int ret;

    //don't forwarding data from uart when in sleep mode
    extern uint8 isSleepAllowInPM(void);

    if (isSleepAllowInPM()) {
        return 0;
    }

    if (g_at_ble_handler->at_config->role == MASTER) {
        ret = uart_client_send(g_uart_handler, data, length, NULL);
    } else {
        ret = uart_server_send(g_uart_handler, data, length, NULL);
    }

    return ret;
}

static int uart_profile_recv(const uint8_t *data, int length)
{
    int ret = 0;

    if (at_server_mode == AT_MODE) {
        ret = at_mode_send("+DATA:", (char *)data, length);

        if (ret) {
            return ret;
        }

        return at_mode_send("\r\n", NULL, 0);
    } else {
        return uart_mode_send((char *)data, length);
    }
}

static void conn_change(ble_event_en event, void *event_data)
{

    evt_data_gap_conn_change_t *e = (evt_data_gap_conn_change_t *)event_data;

    if (e->connected == DISCONNECTED) {
        atserver_exit_passthrough();
        uint8_t errCode[25];
        snprintf((char *)errCode, sizeof(errCode), "+DISCONNECTED:%d\r\n", e->err);
        at_mode_send("\r\n", (char *)errCode, strlen((char *)errCode));
        csi_gpio_pin_config_mode(g_connect_info, GPIO_MODE_PULLDOWN);
        csi_gpio_pin_write(g_connect_info, 0);
        aos_sem_signal(&g_sync_sem);
    }
}

static void mtu_exchange(ble_event_en event, void *event_data)
{
    uint8_t at_pin;
    uint8_t conn_info[37];
    evt_data_gatt_mtu_exchange_t *e = (evt_data_gatt_mtu_exchange_t *)event_data;

    csi_gpio_pin_read(g_mode_ctrl, (bool *)&at_pin);

    if (at_pin) {
        atserver_exit_passthrough();
        at_server_mode = AT_MODE;
    } else {
        atserver_passthrough_cb_register(at_ble_uartmode_recv);
        at_server_mode = UART_MODE;
    }

    connect_info_t info = {0};

    ble_stack_connect_info_get(e->conn_handle, &info);

    snprintf((char *)conn_info, sizeof(conn_info), "+CONNECTED:%02x:%02x:%02x:%02x:%02x:%02x,%d\r\n", \
             info.peer_addr.val[5], info.peer_addr.val[4], info.peer_addr.val[3], info.peer_addr.val[2],
             info.peer_addr.val[1], info.peer_addr.val[0], info.peer_addr.type);
    at_mode_send("\r\n", (char *)conn_info, strlen((char *)conn_info));

    if (g_at_ble_handler->at_config->sleep_mode != NO_SLEEP) {
        bool sleep_ctrl_pin;
        csi_gpio_pin_read(g_sleep_ctrl, &sleep_ctrl_pin);

        if (g_at_ble_handler->at_config->sleep_mode == SLEEP && sleep_ctrl_pin == 0) {
            csi_gpio_pin_config_mode(g_connect_info, GPIO_MODE_PULLUP);
        }
    }

    csi_gpio_pin_write(g_connect_info, 1);
}

static int user_event_callback(ble_event_en event, void *event_data)
{
    switch (event) {
        //common event
        case EVENT_GAP_CONN_CHANGE:
            conn_change(event, event_data);
            break;

        case EVENT_GATT_MTU_EXCHANGE:
            mtu_exchange(event, event_data);
            break;

        default:
            break;
    }

    return 0;
}

static uint8_t *get_data_type_in_sd(uint8_t type, ad_data_t *sd, uint8_t sd_num)
{
    if (!sd || !sd_num) {
        return NULL;
    }

    for (int i = 0; i < sd_num ; i++) {
        if (sd[i].type == type && sd[i].data != NULL) {
            return sd[i].data;
        }
    }

    return NULL;
}


static char *set_dev_name(at_conf_load *conf)
{
    if (conf == NULL) {
        return NULL;
    }

    adv_param_t *adv_param = & conf->uart_conf.slave_conf.param;

    /*if it is master dev or dev name info not exist at the sd data then use the config dev name*/
    if (conf->role == MASTER || !get_data_type_in_sd(AD_DATA_TYPE_NAME_COMPLETE, adv_param->sd, adv_param->sd_num)) {
        return (char *)conf->bt_name;
    } else {
        return NULL;
    }
}

static int at_bt_init()
{
    int ret;

    init_param_t init;

    init.dev_name = set_dev_name(g_at_ble_handler->at_config);

    if (g_at_ble_handler->at_config->addr.type) {
        init.dev_addr = &g_at_ble_handler->at_config->addr;
    } else {
        init.dev_addr = NULL;
    }

    init.conn_num_max  = 1;

    ret = ble_stack_init(&init);

    if (ret) {
        return ret;
    }

    at_ble_cb recv_cb = {
        .uart_mode_recv_cb = uartmode_recv,
        .at_mode_recv_cb = atmode_recv,
    };

    ret = at_ble_event_register(recv_cb);

    if (ret) {
        return ret;
    }

    if (g_at_ble_handler->at_config->role == SLAVE) {
        g_uart.server.uart_recv = uart_profile_recv;
        g_uart.server.uart_event_callback = user_event_callback;
        g_uart.server.conn_update_def_on = g_at_ble_handler->at_config->conn_update_def_on;

        if (g_uart.server.conn_update_def_on) {
            g_uart.server.conn_param = &g_at_ble_handler->at_config->conn_param;
        } else {
            g_uart.server.conn_param = NULL;
        }

        g_uart_handler = uart_server_init(&g_uart.server);
    } else {
        g_uart.client.uart_recv = uart_profile_recv;
        g_uart.client.uart_event_callback = user_event_callback;
        g_uart.client.client_data.client_conf.conn_def_on = g_at_ble_handler->at_config->uart_conf.master_conf.conn_def_on;
        g_uart.client.client_data.client_conf.auto_conn_mac_size = g_at_ble_handler->at_config->uart_conf.master_conf.auto_conn_num;
        g_uart.client.client_data.client_conf.auto_conn_mac = g_at_ble_handler->at_config->uart_conf.master_conf.auto_conn_info;
        g_uart.client.conn_update_def_on = g_at_ble_handler->at_config->conn_update_def_on;

        if (g_uart.client.conn_update_def_on) {
            g_uart.client.conn_param = &g_at_ble_handler->at_config->conn_param;
        } else {
            g_uart.client.conn_param = NULL;
        }

        g_uart_handler = uart_client_init(&g_uart.client);
    }

    g_adv_enable = g_at_ble_handler->at_config->uart_conf.slave_conf.adv_def_on;///////

    return ret;

}

static int at_bt_start()
{
    if (g_at_ble_handler->at_config->role == MASTER) {
        return uart_client_scan_start();
    } else {
        if (!g_adv_enable) {
            return -1;
        }

        if (adv_param_temp) {
            uart_server_adv_control(ADV_ON, adv_param_temp);
        } else {
            uart_server_adv_control(ADV_ON, &g_at_ble_handler->at_config->uart_conf.slave_conf.param);
        }
    }

    return 0;
}

int app_main(int argc, char *argv[])

{
    int ret;
    char reboot_info[13] = {0};
    board_yoc_init();
    ret = at_bt_init();
    snprintf(reboot_info, sizeof(reboot_info), "+REBOOT:%d", ret);
    at_mode_send("\r\n", reboot_info, strlen(reboot_info));

    if (ret) {
        LOGE(TAG, "at bt init fail %d\n", ret);
        return ret;
    }

    LOGI(TAG, "at ble startup\n");

    aos_sem_new(&g_sync_sem, 1);

    while (1) {
        aos_sem_wait(&g_sync_sem, AOS_WAIT_FOREVER);

        ret = at_bt_start();

        if (ret) {
            LOGE(TAG, "at bt start fail %d\n", ret);
            return ret;
        }
    }

    return 0;
}



