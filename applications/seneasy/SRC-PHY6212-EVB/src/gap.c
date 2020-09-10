#include <aos/ble.h>
#include "gap.h"
#include <log.h>

#include "app_msg.h"

#define TAG "GAP"

ble_gap_state_t g_gap_data;

aos_timer_t adv_timer = {0};

// https://www.bluetooth.com/xml-viewer/?src=https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Characteristics/org.bluetooth.characteristic.gap.appearance.xml
static uint8_t adv_appearance_hid[] = {0xc0, 0x03};         // @key:960 @value:Human Interface Device (HID) @description:HID Generic
// static const uint8_t appearance_Keyboard[] = {0xc1, 0x03};    // @key:961 @value:Keyboard @description:HID subtype
// static const uint8_t appearance_mouse[] = {0xc2, 0x03};       // @key:962 @value:Mouse @description:HID subtype
static uint8_t adv_uuids_hids[] = {0x0f, 0x18};
static uint8_t adv_flags = AD_FLAG_LIMITED | AD_FLAG_NO_BREDR;

static ad_data_t app_scan_rsp_data[1] = {
    [0] = {
        .type = AD_DATA_TYPE_GAP_APPEARANCE,
        .data = adv_appearance_hid,            // keyboard:961
        .len = 2
    }
};

static ad_data_t app_adv_data[4] = {
    [0] = {
        .type = AD_DATA_TYPE_FLAGS,
        .data = &adv_flags,
        .len = 1
    },
    [1] = {
        .type = AD_DATA_TYPE_UUID16_ALL,
        .data = adv_uuids_hids,            // uuid_hids
        .len = 2,
    },
    [2] = {
        .type = AD_DATA_TYPE_GAP_APPEARANCE,
        .data = adv_appearance_hid,            // keyboard:961
        .len = 2
    },
    [3] = {
        .type = AD_DATA_TYPE_NAME_COMPLETE,
        .data = (uint8_t *)DEVICE_NAME,
        .len = strlen(DEVICE_NAME),
    },
};

static ad_data_t app_adv_data_power[5] = {
    [0] = {
        .type = AD_DATA_TYPE_FLAGS,
        .data = &adv_flags,
        .len = 1
    },
    [1] = {
        .type = AD_DATA_TYPE_SVC_DATA16,
        .data = (uint8 []) {0x01, 0xFD, 0x00, 0x01},
        .len = 5
    },
    [2] = {
        .type = AD_DATA_TYPE_UUID16_ALL,
        .data = adv_uuids_hids,            // uuid_hids
        .len = 2,
    },
    [3] = {
        .type = AD_DATA_TYPE_NAME_COMPLETE,
        .data = (uint8_t *)DEVICE_NAME,
        .len = sizeof(DEVICE_NAME),
    },
    [4] = {
        .type = AD_DATA_TYPE_GAP_APPEARANCE,
        .data = adv_appearance_hid,            // keyboard:961
        .len = 2
    }
};

bool stop_adv()
{
    if (ble_stack_adv_stop() != 0) {
        LOGE("GAP", "failed to stop adversting");
        return false;
    } else {
        return true;
    }
    // if (adv_timer.hdl.timer_state == TIMER_ACTIVE) {
    //     aos_timer_stop(&adv_timer);
    // }
}

static void adv_timer_callback(void *arg1, void *arg2)
{
    if (g_gap_data.state == GAP_STATE_ADVERTISING) {
        if (stop_adv() == true) {
            LOGI("GAP", "advertising stoped");
            g_gap_data.state = GAP_STATE_IDLE;
        } else {
            LOGE("GAP", "advertising cant not been stoped");
        }
    }
}

#define KEY_BOND_INFO   "BOND"
bond_info_t bond_info = {0};
bool save_bond_info(dev_addr_t *p_remote_addr)
{
    int ret;
    bond_info.is_bonded = true; 
    memcpy(&bond_info.remote_addr, p_remote_addr, sizeof(dev_addr_t));
    ret = aos_kv_set(KEY_BOND_INFO, (void *)&bond_info, sizeof(bond_info_t), 1);
    if (ret != 0) {
        LOGE("bond", "can`t write bond info, err=%d",ret);
        bond_info.is_bonded = false;
        return false;
    }
    ret = ble_stack_white_list_add(p_remote_addr);
    if (ret != 0) {
        LOGE("bond", "add whitelist failed, err=%d", ret);
        bond_info.is_bonded = false;
        return false;
    }
    bond_info.is_bonded = true;
    return true;
}

bool load_bond_info()
{
    int len,ret;
    bond_info.is_bonded = false;
    ret = aos_kv_get(KEY_BOND_INFO, (void *)&bond_info, &len);
    if (ret != 0 || len == sizeof(bond_info_t)) {
        LOGI("bond", "no bond info find");
        return false;
    } else {
        LOGI("bond", "has bonded with device::%02x-%02x-%02x-%02x-%02x-%02x", 
                                            bond_info.remote_addr.val[0],
                                            bond_info.remote_addr.val[1],
                                            bond_info.remote_addr.val[2],
                                            bond_info.remote_addr.val[3],
                                            bond_info.remote_addr.val[4],
                                            bond_info.remote_addr.val[5]);
        return true;
    }   
}

bool remove_bond_info()
{
    int ret;
    ret = ble_stack_dev_unpair(NULL);
    if (ret != 0) {
        LOGE("bond", "ble stack dev unpair failed, err=%d", ret);
        return false;
    }

    ret = ble_stack_white_list_remove(&bond_info.remote_addr);
    if (ret != 0) {
        LOGE("bond", "ble stack remove whitelist failed, err=%d", ret);
        return false;
    }

    bond_info.is_bonded = false;
    ret = aos_kv_set(KEY_BOND_INFO, (void *)&bond_info, sizeof(bond_info_t), 1);
    if (ret != 0) {
        LOGE("bond", "kv can`t remove bond info");
        return false;
    }
    return true;
}


static bool start_adv(adv_param_t * p_adv_param, uint32_t timeout)
{
    /**
     * ADV_IND =                 0x00, 
     * 非定向广播，可被连接，可被扫描；
     *  广播包间隔 <= 10ms, 广播事件间隔[20ms,10.24s];
     *  用于常规的广播，可携带不超过31bytes的广播数据：
     * 
     * ADV_DIRECT_IND =          0x01, 
     * 定向广播:
     *  广播包间隔 <= 3.75ms，没有间隔，1.28s后退出广播
     *  专门用于点对点连接，且已经知道双方的蓝牙地址，不可携带广播数据，可被指定的设备连接，不可被扫描：
     * 
     * ADV_SCAN_IND =            0x02,  
     * 非定向广播，不可以被连接，可以被扫描:
     *  广播包间隔 <= 10ms, 广播事件间隔范围[100ms, 10.24s]
     * 
     * ADV_NONCONN_IND =         0x03,  
     * 非定向广播，不可以被连接，不可以被扫描:
     *  广播包间隔 <= 10ms，广播事件间隔范围[100ms, 10.24s]
     * 
     * ADV_DIRECT_IND_LOW_DUTY = 0x04,  
     * 定向广播:
     *  广播包间隔 <= 3.75ms， 广播事件间隔[20, 10.24s]
     */
    // 启动广播
    int err = ble_stack_adv_start(p_adv_param);
    if (err == 0) {
        // 更新状态
        g_gap_data.state = GAP_STATE_ADVERTISING;
        g_gap_data.ad_type = p_adv_param->type;
        // 非直连广播，启动广播计时器
        if (g_gap_data.ad_type != ADV_DIRECT_IND) {
            int err = 0;
            err = aos_timer_stop(&adv_timer);
            err = aos_timer_change_without_repeat(&adv_timer, timeout);
            err = aos_timer_start(&adv_timer);
            if (err != 0) {
                LOGE(TAG, "advertising started!, but stop timer can`t work:%d", err);
            }
        }
        LOGI(TAG, "advertising started %s!, will stop after %d ms", (g_gap_data.ad_type == ADV_IND) ? "indirect":"direct", timeout);
        return true;
    } 
    // 广播启动失败
    else {
        LOGE(TAG, "advertising start failed: %d!", err);
        return false;
    } 
}

/*********************************************************************************
 * GAP 事件处理函数
 ********************************************************************************/
// #pragma regions
/**
 * @brief 直连广播超时事件(启动后1.2s没有连接)
 * 
 * @param event_data 
 */
static void gap_event_adv_timeout(void *event_data)
{
    if (g_gap_data.ad_type == ADV_DIRECT_IND) {
        LOGI(TAG, "advertising timerout");  // 什么情况下会发生?
        g_gap_data.state = GAP_STATE_IDLE;
        rcu_ble_start_adversting(ADV_START_TIMEOUT);
    }
}

/**
 * @brief 
 * 
 * @param event_data 
 */
static void gap_event_smp_pairing_passkey_display(evt_data_smp_passkey_display_t *event_data)
{
    LOGI(TAG, "passkey is %s", event_data->passkey);
}

static void gap_event_smp_pairing_complete(evt_data_smp_pairing_complete_t *event_data)
{
    if (event_data->err == 0) {
        g_gap_data.state = GAP_STATE_PAIRED;
        g_gap_data.paired_addr = event_data->peer_addr;
        if (event_data->bonded ) {
            save_bond_info(&(event_data->peer_addr));
            LOGI(TAG, "bond with remote device:%02x-%02x-%02x-%02x-%02x-%02x",   
                                                    event_data->peer_addr.val[0],
                                                    event_data->peer_addr.val[1],
                                                    event_data->peer_addr.val[2],
                                                    event_data->peer_addr.val[3],
                                                    event_data->peer_addr.val[4],
                                                    event_data->peer_addr.val[5]
                                                    );
        }
    } else {
        LOGI(TAG, "pairing %s!!!, error=%d", event_data->err ? "FAIL" : "SUCCESS", event_data->err);
        remove_bond_info();
    }
}

static void gap_event_smp_cancel(void *event_data)
{
    LOGI(TAG, "pairing cancel");
}

static void gap_event_smp_pairing_confirm(evt_data_smp_pairing_confirm_t *e)
{
    ble_stack_smp_passkey_confirm(e->conn_handle);    
    LOGI(TAG, "Confirm pairing for");
}

static void gap_event_conn_security_change(evt_data_gap_security_change_t *event_data)
{
    LOGI(TAG, "conn %d security level change to level %d", event_data->conn_handle, event_data->level);
}

static void gap_event_conn_change(evt_data_gap_conn_change_t *event_data)
{  
    LOGI("GAP", "%s, error=%d", (event_data->connected == CONNECTED) ? "connect":"disconect", event_data->err);
    if (event_data->connected == CONNECTED) {
        // 设置连接加密
        // ble_stack_security(event_data->conn_handle, SECURITY_LOW);
        // ble_stack_security(e->conn_handle, SECURITY_MEDIUM);
        g_gap_data.conn_handle = event_data->conn_handle;
        g_gap_data.state = GAP_STATE_CONNECTED;
        aos_timer_stop(&adv_timer);
        // led_set_status(BLINK_SLOW);
    } 

    if (event_data->connected == DISCONNECTED) {
        // 设置全局gap状态
        g_gap_data.conn_handle = -1;
        g_gap_data.state = GAP_STATE_DISCONNETED;
        // 如果远程主机主动断开连接(19)或者本机主动断开连接(22),则不启动广播,否则启动广播
        if (event_data->err == 19 || event_data->err == 22) {
            LOGI("GAP", "BT_HCI_ERR_REMOTE_USER_TERM_CONN or BT_HCI_ERR_LOCALHOST_TERM_CONN");
        }
        // 其他原因导致的断开连接,启动广播尝试回连
        else {
            rcu_ble_start_adversting(ADV_START_RECONNECT);
        }
    }


}

static void gap_event_conn_param_update(evt_data_gap_conn_param_update_t *event_data)
{
    LOGD(TAG, "LE conn param updated: int 0x%04x lat %d to %d\n", 
            event_data->interval,
            event_data->latency,
            event_data->timeout);
}

static void gap_event_mtu_exchange(evt_data_gatt_mtu_exchange_t *event_data)
{
    if (event_data->err == 0) {
        g_gap_data.mtu_size = ble_stack_gatt_mtu_get(event_data->conn_handle);
        LOGI(TAG, "mtu exchange, MTU %d", g_gap_data.mtu_size);
    } else {
        LOGE(TAG, "mtu exchange fail, %x", event_data->err);
    }
}

static void ota_event_callback(ota_state_en state)
{
    g_gap_data.ota_state = state;
    app_event_set(APP_EVENT_OTA);
}

static int gap_event_callback(ble_event_en event, void *event_data)
{
    switch (event) {
        case EVENT_GAP_CONN_CHANGE:
            gap_event_conn_change(event_data);
            break;

        case EVENT_GAP_CONN_PARAM_UPDATE:
            gap_event_conn_param_update(event_data);
            break;

        case EVENT_SMP_PASSKEY_DISPLAY:
            gap_event_smp_pairing_passkey_display(event_data);
            break;

        case EVENT_SMP_PAIRING_COMPLETE:
            gap_event_smp_pairing_complete(event_data);
            break;

        case EVENT_SMP_PAIRING_CONFIRM:
            gap_event_smp_pairing_confirm(event_data);
            break;

        case EVENT_SMP_CANCEL:
            gap_event_smp_cancel(event_data);
            break;

        case EVENT_GAP_CONN_SECURITY_CHANGE:
            gap_event_conn_security_change(event_data);
            break;

        case EVENT_GATT_MTU_EXCHANGE:
            gap_event_mtu_exchange(event_data);
            break;

        case EVENT_GAP_ADV_TIMEOUT:
            gap_event_adv_timeout(event_data);
            break;
        default:
            break;
    }

    return 0;
}

static ble_event_cb_t ble_cb = {
    .callback = gap_event_callback,
};

// #pragma endregion

/**
 * @brief 蓝牙协议栈初始化
 * 
 * @return true 
 * @return false 
 */
bool rcu_ble_init()
{
    int ret = 0;
    dev_addr_t addr = {DEV_ADDR_LE_PUBLIC, DEVICE_ADDR};
    // dev_addr_t addr = {DEV_ADDR_LE_RANDOM, DEVICE_ADDR};

    init_param_t init = {
        .dev_name = DEVICE_NAME,
        .dev_addr = &addr,
        .conn_num_max = 1,
    };
    // 协议栈初始化和配置
    ret = ble_stack_init(&init);
    ret = ble_stack_setting_load();
    ret = ble_stack_event_register(&ble_cb);
    ret = ble_stack_iocapability_set(IO_CAP_IN_NONE | IO_CAP_OUT_NONE);
    if (ret != 0) {
        LOGE(TAG, "ble stack init failed");
    }
    // 注册服务
    hid_service_init();
    dis_service_init();
    battary_service_init();
    g_gap_data.p_atvv = atvv_service_init();
    ble_ota_init(ota_event_callback);

    // 加载配对信息并启动广播
    ret = aos_timer_new_ext(&adv_timer, adv_timer_callback, NULL, 3000, 0, 0);
    if (ret != 0 ) {
        LOGE("GAP", "advertising timer create failed");
    }
    // start_adv(ADV_IND);
    return true;
}

void rcu_ble_start_adversting(adv_start_reson_t reson)
{
    adv_param_t param = {
        .channel_map = ADV_DEFAULT_CHAN_MAP,
        .direct_peer_addr = {
            .type = DEV_ADDR_LE_PUBLIC,
            .val = {0,0,0,0,0,0}
        },
        .interval_min = 10,
        .interval_max = 100,
        .ad = app_adv_data,
        .ad_num = BLE_ARRAY_NUM(app_adv_data),
        .sd = app_scan_rsp_data,
        .sd_num = BLE_ARRAY_NUM(app_scan_rsp_data),
        .type = ADV_IND,
        .filter_policy = ADV_FILTER_POLICY_ANY_REQ,
    };
    uint32_t adv_timeout = 30000;
    // 正在广播
    if (g_gap_data.state == GAP_STATE_ADVERTISING) {
        LOGI("GAP", "restart adversting, will stop after %dmS", adv_timeout);
        // 正在发直连广播,等待发送结束
        if (g_gap_data.ad_type == ADV_DIRECT_IND) { 
            return;
        } 
        // 其他广播方式, 重置计数器/停止广播
        else {
            aos_timer_stop(&adv_timer);
            aos_timer_change_without_repeat(&adv_timer, ADV_PAIRING_TIMEOUT);
            aos_timer_start(&adv_timer);
            return ;
        }
    } else if (g_gap_data.state == GAP_STATE_IDLE) {
        // 根据不同情况发起广播
        switch (reson)
        {
            /**
             * @brief 上电启动广播:
             *  1.如果有配对信息,发直连广播
             *  2.如果没有配对,发普通非直连广播
             */
            case ADV_START_POWER_ON:
                if (load_bond_info() && bond_info.is_bonded) {
                    param.type = ADV_DIRECT_IND;
                    param.filter_policy = ADV_FILTER_POLICY_ALL_REQ;
                    param.direct_peer_addr = bond_info.remote_addr;
                } else {
                    param.type = ADV_IND;
                    param.filter_policy = ADV_FILTER_POLICY_ANY_REQ;
                }
                break;
            /**
             * @brief power按键广播
             *  发送包含自定义数据的广播包
             */
            case ADV_START_POWER_KEY:
                param.type = ADV_IND;
                param.filter_policy = ADV_FILTER_POLICY_ANY_REQ;
                param.ad = app_adv_data_power;
                param.ad_num = BLE_ARRAY_NUM(app_adv_data_power);
                break;
            /**
             * @brief 配对按键启动广播
             * 
             */
            case ADV_START_PAIRING_KEY:     // 启动正常广播
                param.type = ADV_IND;
                param.filter_policy = ADV_FILTER_POLICY_ANY_REQ;
                break;
            /**
             * @brief 断连后启动广播
             * 
             */
            case ADV_START_RECONNECT:
                if (bond_info.is_bonded) {
                    param.type = ADV_DIRECT_IND;
                    param.filter_policy = ADV_FILTER_POLICY_ALL_REQ;
                    param.direct_peer_addr = bond_info.remote_addr;                    
                } else {
                    param.type = ADV_IND;
                    param.filter_policy = ADV_FILTER_POLICY_ANY_REQ;                    
                }
                break;
            /**
             * @brief 直连广播超时后,启动非直连广播
             * 
             */
            case ADV_START_TIMEOUT:
                param.type = ADV_IND;
                param.filter_policy = ADV_FILTER_POLICY_ANY_REQ;
                break;
            default:
                break;
        }
        start_adv(&param, adv_timeout);
    } else {
        LOGI("GAP", "rcu status %d is not allowed to start adv again!", g_gap_data.state);
        return;
    }
}

void rcu_ble_clear_pairing()
{

    // 其他状态：
    // 1.已经连接或配对: 断开连接
    if (g_gap_data.state == GAP_STATE_PAIRED || g_gap_data.state == GAP_STATE_CONNECTED) {
        if (ble_stack_disconnect(g_gap_data.conn_handle) != 0) {
            LOGE("GAP", "can`t disconect with remote device");
            return;
        }
    } 
    // 2.正在广播中: 停止广播
    else if (g_gap_data.state == GAP_STATE_ADVERTISING) {
        int err = ble_stack_adv_stop();
        if (err != 0) {
            LOGE("GAP", "can`t stop advertising, err=%d", err);
        }
    }
    // 3.等待处于空闲状态
    uint32_t timeout = 0;
    while (g_gap_data.state != GAP_STATE_IDLE) {
        aos_msleep(10);
        timeout += 10;
        if (timeout  > 1000){
            LOGE("GAP", "rcu status %d is not allowed to remove bond info", g_gap_data.state);
            return;
        }
    }
    // 4.清除配对信息
    if (bond_info.is_bonded) {
        remove_bond_info();
    }
    // 5.发起非直连广播，重启配对过程
    rcu_ble_start_adversting(ADV_START_PAIRING_KEY);
}

