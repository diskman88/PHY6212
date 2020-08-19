#ifndef _GPA_H

#include <stdbool.h>
#include <stdint.h>
#include <aos/ble.h>
#include "services/hid_service.h"
#include "services/battary_service.h"
#include "services/dis_service.h"
#include "services/atvv_service.h"

/*******************************************************************************
 * 广播
 ******************************************************************************/
#define ADV_RECONNECT_NORMAL_TIMEOUT    30000    // 1s
#define ADV_PAIRING_TIMEOUT             30000  /* 12s */


#define DEVICE_NAME "remote control"
#define DEVICE_ADDR {0xE8,0x3B,0xE3,0x88,0xB1,0xC8}


typedef enum {
    GAP_STATE_IDLE = 0,
    GAP_STATE_ADVERTISING,      /* RCU adversting status */
    GAP_STATE_STOP_ADVERTISING, /* temporary status of stop adversting */
    GAP_STATE_CONNECTED,        /* connect status but not start paring */
    GAP_STATE_PAIRED,           /* rcu paired success status */
    GAP_STATE_DISCONNECTING,    /* temporary status of disconnecting */
}gap_state_t;


typedef struct 
{
    gap_state_t state;
    adv_type_en ad_type;
    int16_t conn_handle;        // 链接id
    dev_addr_t paired_addr;     // 当前配对地址
    int mtu_size;

    // bas_handle_t *bas;
    // dis_handle_t *dis;
    // hids_handle_t *hids;
    atvv_service_t *p_atvv;
}ble_gap_state_t;

typedef struct 
{
    bool is_bonded;
    dev_addr_t remote_addr;
}bond_info_t;


extern ble_gap_state_t g_gap_data;
extern bond_info_t bond_info;

bool rcu_ble_init();

void rcu_ble_pairing();

#endif