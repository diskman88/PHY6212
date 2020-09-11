/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#ifndef _AT_BLE_H
#define _AT_BLE_H

#include "at_ble_def_config.h"
#include "aos/ble.h"
#include "auto_config.h"
#include <stddef.h>




typedef struct _slave_load {
    uint8_t adv_def_on;
    uint8_t conn_update_def_on;
    conn_param_t conn_param;
    adv_param_t param;
} slave_load;

typedef struct _master_load {
    uint8_t conn_def_on;
    dev_addr_t auto_conn_info[AUTO_CONN_BT_MAX];
    uint8_t auto_conn_num;
} master_load;

typedef struct _uart_service_load {
    slave_load slave_conf;
    master_load master_conf;
} uart_service_load;

typedef struct _at_conf_load {
    uint8_t bt_name[CONFIG_BT_DEVICE_NAME_MAX];
    dev_addr_t addr;
    uint8_t role;
    uint8_t tx_pow;
    uint8_t sleep_mode;
    int   baud;
    uart_service_load uart_conf;
} at_conf_load;

typedef  void *st_recv_cb;

typedef  int (*_at_at_mode_recv)(uint8_t, char *, int, st_recv_cb);
typedef  int (*_at_uart_mode_recv)(char *, int, st_recv_cb);
typedef char *at_head;

typedef struct _at_server_ble {
    uint8_t mode;
    _at_uart_mode_recv uart_mode_recv;
    _at_at_mode_recv at_mode_recv;
} at_server_ble;

enum {
    _AT_BT_ADV_ON,
    _AT_BT_ADV_OFF,
    _AT_BT_CONN,
    _AT_BT_CONN_UPDATE,
    _AT_BT_FIND,
    _AT_BT_REBOOT,
    _AT_BT_RST,
    _AT_BT_SLEEP,
    _AT_BT_TX,
    _AT_BT_FOTA,
};
enum BT_ROLE_EN {
    SLAVE,
    MASTER,
};
enum AT_MODE_EN {
    AT_MODE,
    UART_MODE,
};

enum SLEEP_MODE {
    NO_SLEEP,
    SLEEP,
    STANDBY,
};

enum {
    DEF_OFF,
    DEF_ON,
    LOAD_FAIL,
};


#ifdef CONFIG_UNITTEST_MODE
#define _STATIC
#else
#define _STATIC static
#endif


#define AT_ERR_BT_KV_SAVE_PARAM (-30)
#define AT_ERR_AT_SERVICE_NOT_INITIALIZED (-31)
#define AT_ERR_CMD_NOT_SUPPORTED (-32)
#define AT_ERR_BT_KV_UNLOAD (-33)
#define AT_ERR_BT_TX_LEN (-34)
#define AT_ERR_BT_TX_FAILED (-35)

#define BLE_BASE_ERR  0x00000000
#define AT_BASE_ERR   0x000003E8

#define ERR_BLE(errno) (-BLE_BASE_ERR + errno)
#define ERR_AT(errno)  (-AT_BASE_ERR  + errno)


#define AD_MAX_NUM 3
#define SD_MAX_NUM 4

int at_conf_load_from_kv(at_conf_load *conf_load);
int at_ble_service_init(at_server_ble *service);
int at_mode_send(at_head head, const char *data, size_t data_size);
int uart_mode_send(const char *data, size_t size);




#endif

