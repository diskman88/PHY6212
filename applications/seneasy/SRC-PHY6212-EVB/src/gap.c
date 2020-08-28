#include <aos/ble.h>
#include "gap.h"
#include <log.h>



#include "app_msg.h"

#define TAG "GAP"

ble_gap_state_t g_gap_data;

aos_timer_t adv_timer = {0};

// https://www.bluetooth.com/xml-viewer/?src=https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Characteristics/org.bluetooth.characteristic.gap.appearance.xml
// static uint8_t appearance_hid[] = {0xc0, 0x03};         // @key:960 @value:Human Interface Device (HID) @description:HID Generic
// static uint8_t appearance_Keyboard[] = {0xc1, 0x03};    // @key:961 @value:Keyboard @description:HID subtype
// static uint8_t appearance_mouse[] = {0xc2, 0x03};       // @key:962 @value:Mouse @description:HID subtype
// static uint8_t adv_uuids[] = {0x0f, 0x18};
static ad_data_t app_scan_rsp_data[1] = {
    [0] = {
        .type = AD_DATA_TYPE_GAP_APPEARANCE,
        .data = (uint8_t[]){0xc1, 0x03},            // keyboard:961
        .len = 2
    }
};

static ad_data_t app_adv_data[4] = {
    [0] = {
        .type = AD_DATA_TYPE_FLAGS,
        .data = (uint8_t[]){AD_FLAG_GENERAL | AD_FLAG_NO_BREDR},
        .len = 1
    },
    [1] = {
        .type = AD_DATA_TYPE_UUID16_ALL,
        .data = (uint8_t[]){0x12, 0x18},            // uuid_hids
        .len = 2,
    },
    [2] = {
        .type = AD_DATA_TYPE_NAME_COMPLETE,
        .data = (uint8_t *)DEVICE_NAME,
        .len = sizeof(DEVICE_NAME),
    },
    [3] = {
        .type = AD_DATA_TYPE_GAP_APPEARANCE,
        .data = (uint8_t[]){0xc1, 0x03},            // keyboard:961
        .len = 2
    }
};

static adv_param_t param = {
    .type = ADV_IND,
    .ad = app_adv_data,
    // .sd = app_scan_rsp_data,
    .sd = NULL,
    .ad_num = BLE_ARRAY_NUM(app_adv_data),
    // .sd_num = BLE_ARRAY_NUM(app_scan_rsp_data),
    .sd_num = 0,
    .filter_policy = ADV_FILTER_POLICY_ANY_REQ,
    .channel_map = ADV_DEFAULT_CHAN_MAP,
    .direct_peer_addr = {0},
    .interval_min = ADV_FAST_INT_MIN_2,
    .interval_max = ADV_FAST_INT_MAX_2,
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

static bool start_adv(adv_type_en type)
{
    int adv_timeout = 0;
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
    // 根据不同的广播类型配置广播参数
    switch (type)
    {
        // 配对广播
        case ADV_IND:
            param.type = ADV_IND;
            param.filter_policy = ADV_FILTER_POLICY_ANY_REQ;
            param.interval_min = ADV_FAST_INT_MIN_1;    // 广播间隔在[160ms,240ms]之间
            param.interval_max = ADV_FAST_INT_MAX_1;
            adv_timeout = ADV_PAIRING_TIMEOUT;
            break;
        // 回连广播
        case ADV_DIRECT_IND:
            param.type = ADV_DIRECT_IND;
            param.direct_peer_addr = bond_info.remote_addr;
            param.filter_policy = ADV_FILTER_POLICY_CONN_REQ; 
            adv_timeout = 0;
            break;
        default:
            break;
    }
    // 启动广播
    if (ble_stack_adv_start(&param) == 0) {   
        // 更新状态
        g_gap_data.state = GAP_STATE_ADVERTISING;
        g_gap_data.ad_type = type;
        // 非直连广播，启动广播计时器
        if (type != ADV_DIRECT_IND) {
            int err = 0;
            err = aos_timer_stop(&adv_timer);
            err = aos_timer_change_without_repeat(&adv_timer, adv_timeout);
            err = aos_timer_start(&adv_timer);
            if (err != 0) {
                LOGE(TAG, "advertising started!, but stop timer can`t work:%d", err);
            }
        }
        LOGI(TAG, "advertising started %s!, will stop after %d ms", (type == ADV_IND) ? "indirect":"direct", adv_timeout);
        return true;
    } 
    // 广播启动失败
    else {
        LOGE(TAG, "advertising start failed!");
        return false;
    } 
}


#define KEY_BOND_INFO   "BOND"
bond_info_t bond_info = {0};
bool save_bond_info()
{
    if (aos_kv_set(KEY_BOND_INFO, (void *)&bond_info, sizeof(bond_info_t), 1) != 0) {
        // TO-DO: 保存失败
        return false;
    }
    return true;
}

bool load_bond_info()
{
    int len;
    if (aos_kv_get(KEY_BOND_INFO, (void *)&bond_info, &len) != 0) {
        return false;
    }
    if (len == sizeof(bond_info_t)) {
        return true;
    } else {
        return false;
    }
}

bool remove_bond_info()
{
    int err;
    bond_info.is_bonded = false;
    if (aos_kv_set(KEY_BOND_INFO, (void *)&bond_info, sizeof(bond_info_t), 1) == 0) {
        err = ble_stack_dev_unpair(&bond_info.remote_addr);
        err = ble_stack_white_list_remove(&bond_info.remote_addr);
        if (err != 0) {
            LOGE("GAP", "remove paired info failed");
        }
    }
    return true;
}

void rcu_ble_pairing()
{
    // 正在广播： 重置广播计时器，从新开始计时
    if(g_gap_data.state == GAP_STATE_ADVERTISING) {
        aos_timer_stop(&adv_timer);
        aos_timer_change_without_repeat(&adv_timer, ADV_PAIRING_TIMEOUT);
        aos_timer_start(&adv_timer);
        return ;
    }
    // 其他状态：
    // 1.已经连接或配对： 断开连接
    if (g_gap_data.state == GAP_STATE_PAIRED || g_gap_data.state == GAP_STATE_CONNECTED) {
        if (ble_stack_disconnect(g_gap_data.conn_handle) != 0) {
            LOGE("GAP", "can`t disconect with remote device");
            return;
        }
    }
    // 2.如果已经有绑定信息，则清除绑定信息
    if (bond_info.is_bonded) {
        if (remove_bond_info() == false) {
            LOGE("GAP", "remove bond info failed");
        }
    }
    // 3.发起非直连广播，重启配对过程
    start_adv(ADV_IND);
}

static void gap_event_adv_timeout(void *event_data)
{
    if (g_gap_data.ad_type == ADV_DIRECT_IND) {
        LOGI(TAG, "advertising timerout");  // 什么情况下会发生?
        start_adv(ADV_IND);
    }
}

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
            ble_stack_white_list_add(&event_data->peer_addr);
            bond_info.is_bonded = true;
            bond_info.remote_addr = event_data->peer_addr;
            save_bond_info();
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
        ble_stack_dev_unpair(NULL);
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
    if (event_data->connected == CONNECTED && event_data->err == 0) {
        // 设置连接加密
        // ble_stack_security(event_data->conn_handle, SECURITY_LOW);
        // ble_stack_security(e->conn_handle, SECURITY_MEDIUM);
        g_gap_data.conn_handle = event_data->conn_handle;
        g_gap_data.state = GAP_STATE_CONNECTED;
        aos_timer_stop(&adv_timer);
        // led_set_status(BLINK_SLOW);
        LOGI(TAG, "Connected");
    } else {
        // 设置全局gap状态
        g_gap_data.conn_handle = -1;
        g_gap_data.state = GAP_STATE_DISCONNECTING;
        LOGI(TAG, "Disconnected err %d", event_data->err);
        // 启动广播发送
        if (bond_info.is_bonded) {
            start_adv(ADV_DIRECT_IND);
        } else {
            start_adv(ADV_IND);
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
    LOGD(TAG, "GAP event %x\n", event);

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
    if (load_bond_info()) {
        LOGI("GAP", "load bond info from flash");
    }
    if (bond_info.is_bonded == true) {
        start_adv(ADV_DIRECT_IND);
    } else {
        start_adv(ADV_IND);
    }
    return true;
}
