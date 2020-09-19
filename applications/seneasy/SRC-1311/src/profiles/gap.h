#ifndef __GAP_H
#define __GAP_H

#include <stdbool.h>
#include <stdint.h>
#include <aos/ble.h>

/* SDK 提供的profile */
#include <yoc/dis.h>
#include <yoc/bas.h>
#include <yoc/ota_server.h>
/* 自定义profile */
// #include "services/atvv_service.h"
#include "atvv_service.h"

#define ADV_RECONNECT_NORMAL_TIMEOUT    30000    // 1s
#define ADV_PAIRING_TIMEOUT             30000  /* 12s */
#define DEVICE_NAME "remote"
#define DEVICE_ADDR {0x02,0x00,0x15,0xCB,0x7A,0x3B}

/**
 * @brief 蓝牙状态
 * 
 */
typedef enum {
    GAP_STATE_IDLE = 0,
    GAP_STATE_ADVERTISING,      /* RCU adversting status */
    GAP_STATE_STOP_ADVERTISING, /* temporary status of stop adversting */
    GAP_STATE_CONNECTED,        /* connect status but not start paring */
    GAP_STATE_PAIRED,           /* rcu paired success status */
    GAP_STATE_DISCONNETED,    /* temporary status of disconnecting */
}gap_state_t;


typedef struct 
{
    bool is_bonded;
    dev_addr_t remote_addr;
}bond_info_t;

typedef struct 
{
    gap_state_t state;
    adv_type_en ad_type;
    int16_t conn_handle;        // 链接id
    bond_info_t bond;
    // dev_addr_t paired_addr;     // 当前配对地址
    int mtu_size;
    ota_state_en ota_state;
    bas_handle_t h_bas;
    // hids_handle_t *hids;
    atvv_service_t *p_atvv;
}ble_gap_state_t;

typedef enum {
    ADV_START_POWER_ON,
    ADV_START_POWER_KEY,
    ADV_START_TIMEOUT,
    ADV_START_PAIRING_KEY,
    ADV_START_RECONNECT,
}adv_start_reson_t;


extern ble_gap_state_t g_gap_data;

int rcu_ble_init();

int rcu_ble_start_adversting(adv_start_reson_t reson);

int rcu_ble_clear_pairing();

#endif