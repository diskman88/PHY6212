#include "keyboard.h"


#define PIN_FUNC_GPIO 99
#define TAG "KEYSCAN"

typedef enum {
    NOT_INIT,
    START,
    STOP
} timer_state;

typedef struct _key_scan_timer {
    aos_timer_t timer;
    timer_state timer_state;
} key_scan_timer;

typedef struct {
    uint8_t front;
    uint8_t rear;
    uint8_t queue_size;
    uint8_t data_size;
    uint8_t key_val[KEY_SCAN_BUFFER_SZIE];
} key_data_queue;

typedef struct _key_scanner {
    keyboard *key;
    key_scan_timer scan_timer;
    aos_timer_t idle_timer;
    uint8_t allow_stanby;
    uint8_t key_registered;
    key_data_queue queue;
    keyboard_cb scan_cb;
} key_scanner;


key_scanner g_key_map;


static void keyboard_queue_init(key_data_queue *queue)
{
    queue->front = 0;
    queue->rear = 0;
    queue->data_size = 0;
    queue->queue_size = KEY_SCAN_BUFFER_SZIE;
}



static bool is_keyboard_queue_full(key_data_queue *queue)
{
    if (queue->queue_size == queue->data_size) {
        return true;
    } else {
        return false;
    }
}

static int get_queue_data_size()
{
    return g_key_map.queue.data_size;
}

static int put_keyboard_queue_data(key_data_queue *queue, uint8_t data)
{
    if (!queue) {
        return -1;
    }

    if (is_keyboard_queue_full(queue)) {
        return -1;
    }

    queue->key_val[queue->rear] = data;
    queue->rear = (queue->rear + 1) % queue->queue_size;
    queue->data_size++;
    return 0;
}


inline static void keyboard_prepare_state_get(uint8_t row_id)
{
    int ret = 0;
    ret = csi_gpio_pin_config_direction(g_key_map.key->key_row[row_id].handler, GPIO_DIRECTION_OUTPUT);
    if(ret) {
        LOGE(TAG,"row %d config direction faild",row_id);
    }
    ret = csi_gpio_pin_config_mode(g_key_map.key->key_row[row_id].handler,GPIO_MODE_PULLDOWN);
    if(ret) {
        LOGE(TAG,"row %d config mode faild",row_id);
    }
    ret = csi_gpio_pin_write(g_key_map.key->key_row[row_id].handler, 0);
    if(ret) {
        LOGE(TAG,"row %d write faild",row_id);
    }
}

inline static void keyboard_after_state_get(uint8_t row_id)
{
    int ret = 0;
    ret = csi_gpio_pin_config_direction(g_key_map.key->key_row[row_id].handler, GPIO_DIRECTION_INPUT);
    if(ret) {
        LOGE(TAG,"row %d config direction faild",row_id);
    }
    ret = csi_gpio_pin_config_mode(g_key_map.key->key_row[row_id].handler,GPIO_MODE_PULLDOWN);
    if(ret) {
        LOGE(TAG,"row %d config mode faild",row_id);
    }
}


static void keyboard_state_recheck()
{
    bool key_value = 0;

    for(int row_id = 0; row_id < g_key_map.key->row_num; row_id++) {
        uint8_t raw_offset = row_id * g_key_map.key->column_num;
        keyboard_prepare_state_get(row_id);

        for(int col_id = 0; col_id < g_key_map.key->column_num; col_id++ ) {
            csi_gpio_pin_read(g_key_map.key->key_column[col_id].handler,&key_value);
            if(!key_value) {
                uint8_t map_offset = raw_offset + col_id;
                g_key_map.key->map[map_offset].state = 1;
                LOGD(TAG,"%d %d %02x\r\n",row_id,col_id,g_key_map.key->map[map_offset].func);
                put_keyboard_queue_data(&g_key_map.queue,g_key_map.key->map[map_offset].func);
            }

        }
        keyboard_after_state_get(row_id);
    }

    uint8_t data_len = get_queue_data_size();
    if( data_len >= KEY_SCAN_REPORT_THRESHOLD || !g_key_map.scan_cb) {
        g_key_map.scan_cb(DATA_RECV,data_len);
    }
    g_key_map.key->press_num = 0;

}


static void keyboard_scan_timer_stop()
{
    aos_timer_stop(&g_key_map.scan_timer.timer);
    g_key_map.scan_timer.timer_state = STOP;
}

static void _keyboard_timer_cb ( void* p_timer, void* args )
{
    keyboard_state_recheck();
    if(!g_key_map.key->press_num) {
        keyboard_scan_timer_stop();
    }
}


static void keyboard_enter_standby()
{
    LOGD(TAG,"Enter standby");
    csi_pmu_enter_sleep(NULL, PMU_MODE_STANDBY);
}


static void _enter_standby_timer_cb()
{
    if(g_key_map.allow_stanby) {
        keyboard_enter_standby();
    }
}

static int keyboard_scan_timer_init()
{
    if (g_key_map.scan_timer.timer_state == NOT_INIT ) {
        aos_timer_new ( &g_key_map.scan_timer.timer, _keyboard_timer_cb, NULL, KEY_SCAN_INTERVAL, 1 );
        aos_timer_stop(&g_key_map.scan_timer.timer);
        g_key_map.scan_timer.timer_state = STOP;
    }
    return 0;
}

static int keyboard_idle_timer_init()
{
    aos_timer_new_ext(&g_key_map.idle_timer, _enter_standby_timer_cb, NULL, KEY_MAX_IDLE_TIME * 1000, 1,0);
    return 0;
}

static int keyboard_idle_timer_restart()
{
    aos_timer_stop(&g_key_map.idle_timer);
    aos_timer_start(&g_key_map.idle_timer);
    return 0;
}


static void keyboard_scan_timer_start()
{
    if (g_key_map.scan_timer.timer_state == NOT_INIT ) {
        aos_timer_new (&g_key_map.scan_timer.timer, _keyboard_timer_cb, NULL, KEY_SCAN_INTERVAL, 1 );
    } else if(g_key_map.scan_timer.timer_state == START) {
        aos_timer_stop(&g_key_map.scan_timer.timer);
    }
    aos_timer_start(&g_key_map.scan_timer.timer);
    g_key_map.scan_timer.timer_state = START;
}


static int keyboard_register(keyboard *key)
{

    int ret = 0;
    if(!key) {
        return -1;
    }
    g_key_map.key = key;
    g_key_map.key->press_num = 0;

    for(int row_id = 0; row_id < g_key_map.key->row_num; row_id++) {
        drv_pinmux_config(g_key_map.key->key_row[row_id].pin,PIN_FUNC_GPIO);
        g_key_map.key->key_row[row_id].handler = csi_gpio_pin_initialize(g_key_map.key->key_row[row_id].pin, NULL);
        if(!g_key_map.key->key_row[row_id].handler) {
            LOGE(TAG,"row %d gpio init faild",g_key_map.key->key_row[row_id].pin);
            return -1;
        }
        ret = csi_gpio_pin_config_mode(g_key_map.key->key_row[row_id].handler,GPIO_MODE_PULLDOWN);
        if(ret) {
            LOGE(TAG,"row %d config mode faild",g_key_map.key->key_row[row_id].pin);
        }
        ret = csi_gpio_pin_config_direction(g_key_map.key->key_row[row_id].handler, GPIO_DIRECTION_INPUT);
        if(ret) {
            LOGE(TAG,"row %d config direction faild",g_key_map.key->key_row[row_id].pin);
        }
        phy_gpio_wakeup_set(g_key_map.key->key_row[row_id].pin,POSEDGE);
    }

    for(int col_id = 0; col_id < g_key_map.key->column_num; col_id++) {
        drv_pinmux_config(g_key_map.key->key_column[col_id].pin,PIN_FUNC_GPIO);
        g_key_map.key->key_column[col_id].handler = csi_gpio_pin_initialize(g_key_map.key->key_column[col_id].pin, NULL);
        if(!g_key_map.key->key_column[col_id].handler) {
            LOGE(TAG,"column %d gpio init faild",g_key_map.key->key_column[col_id].pin);
            return -1;
        }
        ret = csi_gpio_pin_config_mode(g_key_map.key->key_column[col_id].handler,GPIO_MODE_PULLUP);
        if(ret) {
            LOGE(TAG,"column %d config mode faild",g_key_map.key->key_column[col_id].pin);
        }
        ret = csi_gpio_pin_config_direction(g_key_map.key->key_column[col_id].handler, GPIO_DIRECTION_INPUT);
        if(ret) {
            LOGE(TAG,"column %d config direction faild",g_key_map.key->key_column[col_id].pin);
        }
        phy_gpio_wakeup_set(g_key_map.key->key_column[col_id].pin,NEGEDGE);
    }

    return 0;
}



int keyboard_init(keyboard *key, keyboard_cb cb)
{
    int ret = 0;
    if(!key || !cb ||keyboard_register(key)) {
        LOGE(TAG,"Key register faild");
        return -1;
    }

    ret = keyboard_scan_timer_init();
    if(ret) {
        LOGE(TAG,"Key scan timer init faild");
        return -1;
    }

    ret = keyboard_idle_timer_init();
    if(ret) {
        LOGE(TAG,"Key scan idle timer init faild");
        return -1;
    }

    keyboard_disable_standby();
    keyboard_queue_init(&g_key_map.queue);

    g_key_map.scan_cb = cb;

    return 0;
}


void keyboard_start_scan()
{
    bool key_value = 0;
    g_key_map.key->press_num = 0;
    for(int row_id = 0; row_id < g_key_map.key->row_num; row_id++) {
        uint8_t raw_offset = row_id * g_key_map.key->column_num;
        keyboard_prepare_state_get(row_id);
        for(int col_id = 0; col_id <  g_key_map.key->column_num; col_id++ ) {
            csi_gpio_pin_read(g_key_map.key->key_column[col_id].handler,&key_value);
            if(!key_value) {
                uint8_t map_offset = raw_offset + col_id;
                g_key_map.key->map[map_offset].state = 1;
                g_key_map.key->press_num++;
            }
        }
        keyboard_after_state_get(row_id);
    }

    if(g_key_map.key->press_num) {
        keyboard_scan_timer_start();
        keyboard_idle_timer_restart();
    }
}

void keyboard_enable_standby()
{
    aos_timer_start(&g_key_map.idle_timer);
    g_key_map.allow_stanby = 1;
}
void keyboard_disable_standby()
{
    aos_timer_stop(&g_key_map.idle_timer);
    g_key_map.allow_stanby = 0;
}



int keyboard_get_ranks_data(uint8_t *data,uint8_t len)
{
    if(!data) {
        return -1;
    }

    int min_len = len < g_key_map.queue.data_size ? len : g_key_map.queue.data_size;
    if(!min_len ) {
        return 0;
    }

    memcpy(data,&g_key_map.queue.key_val[g_key_map.queue.front],min_len);

    g_key_map.queue.front = (g_key_map.queue.front + min_len) % g_key_map.queue.queue_size;

    g_key_map.queue.data_size -= min_len;

    return min_len;
}



