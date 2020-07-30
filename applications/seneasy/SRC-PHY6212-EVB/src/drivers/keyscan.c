
#include "keyscan.h"

#include <drv_common.h>
#include <pinmux.h>
#include <csi_config.h>
#include <csi_kernel.h>
#include <drv_irq.h>
#include <drv_gpio.h>
#include <drv_pmu.h>
#include <dw_gpio.h>
#include <pin_name.h>
#include <clock.h>
#include <soc.h>
#include <yoc/cli.h>
#include <aos/cli.h>
#include <pm.h>
#include <ap_cp.h>

#include "../app_msg.h"

#define DEBUG_CMD_KEYSCAN		1

#define KSCAN_ROW_NUM	4
#define KSCAN_COL_NUM	4
#define KSCAN_MAX_KEY_NUM		6

const uint32_t pins_for_row[KSCAN_ROW_NUM] = {GPIO_P00, GPIO_P01, GPIO_P02, GPIO_P03};
const uint32_t pins_for_col[KSCAN_COL_NUM] = {GPIO_P15, GPIO_P14, GPIO_P13, GPIO_P34};

#define KEYSCAN_INTERVAL_TIME	50
#define KEYSCAN_HOLD_TIME		3000
#define KEYSCAN_HOLD_TIMEOUT	5000

enum {
	KEY_RELEASE,
	KEY_DOWN,
	KEY_HOLD,
	KEY_TIMEOUT
}KEY_STATES;

typedef struct key_def_t
{
	bool is_changed;
	uint8_t code;
	uint8_t state;
	uint8_t resv;
	uint32_t hold_time_ms;
}key_def_t;

static bool is_has_changed = false;
static key_def_t keys[KSCAN_COL_NUM][KSCAN_ROW_NUM];
static key_def_t * keys_pressed[KSCAN_MAX_KEY_NUM];

// static gpio_pin_handle_t kscan_rows[KSCAN_ROW_NUM];
// static gpio_pin_handle_t kscan_cols[KSCAN_COL_NUM];

aos_sem_t sem_keyscan_start;
// aos_timer_t timer_keyscan;

/**
 * @brief 配置keyscan输入引脚
 * 
 * @param rows 
 * @param cols 
 */
void kscan_gpio_row_init()
{
	for(int i = 0; i < KSCAN_ROW_NUM; i++) {
		drv_pinmux_config(pins_for_row[i], PIN_FUNC_GPIO);
		// kscan_rows[i] = csi_gpio_pin_initialize(pins_for_row[i], NULL);
		phy_gpio_pin_init(pins_for_row[i], IE);
		phy_gpio_pull_set(pins_for_row[i], WEAK_PULL_UP);
		// csi_gpio_pin_config(kscan_rows[i], GPIO_MODE_PULLDOWN, GPIO_DIRECTION_INPUT);
		// csi_gpio_pin_config_mode(kscan_rows[i], GPIO_MODE_PULLUP);
		// csi_gpio_pin_config_direction(kscan_rows[i], GPIO_DIRECTION_INPUT);
	}
}

/**
 * @brief 配置keyscan输出引脚
 * 
 * @param cols 
 * @param pins 
 */
void kscan_gpio_col_init()
{
	for(int i = 0; i < KSCAN_COL_NUM; i++) {
		drv_pinmux_config(pins_for_col[i], PIN_FUNC_GPIO);
		phy_gpio_pin_init(pins_for_col[i], OEN);
		phy_gpio_pull_set(pins_for_col[i], WEAK_PULL_UP);
		phy_gpio_write(pins_for_col[i], 1);
		// kscan_cols[i] = csi_gpio_pin_initialize(pins_for_col[i], NULL);
		// csi_gpio_pin_config(cols[i], GPIO_MODE_PULLUP, GPIO_DIRECTION_OUTPUT);
		// csi_gpio_pin_config_mode(kscan_cols[i], GPIO_MODE_PULLUP);
		// csi_gpio_pin_config_direction(kscan_cols[i], GPIO_DIRECTION_OUTPUT);
		// csi_gpio_pin_write(kscan_cols[i], 1);
	}
}

void _kscan_delay_us(int us) 
{
	volatile int m;
	while(us--){
		m = 10;
		while(m--);
	}
}
/**
 * @brief 扫描并读取按键值
 * 
 * @param keycodes 保存按键值的列表
 * @return int 按键个数
 */
int kscan_read_keycode() 
{
	int key_cnt = 0;
	is_has_changed = false;

	for (int c = 0; c < KSCAN_COL_NUM; c++) {
		phy_gpio_pin_init(pins_for_col[c], IE);
	}

	// 扫描键盘
	for (int c = 0; c < KSCAN_COL_NUM; c++) {
		// 列[c]输出0
		phy_gpio_pin_init(pins_for_col[c], OEN);
		phy_gpio_write(pins_for_col[c], 0);	
		_kscan_delay_us(10);
		// 扫描行输入
		for (int r = 0; r < KSCAN_ROW_NUM; r++) {
			key_def_t *skey = &keys[c][r]; 
			// skey->code = c * KSCAN_ROW_NUM + r + 1;
			skey->code = r * KSCAN_COL_NUM + c + 1;
			uint8_t pre_state = skey->state;
			// 该行输入为0,有键按下
			if (phy_gpio_read(pins_for_row[r]) == 0) {
				// 记录按下的按键
				keys_pressed[key_cnt++] = skey;
				key_cnt %= KSCAN_MAX_KEY_NUM;
				// 更新按键状态
				switch (skey->state)
				{
					case KEY_RELEASE:
						skey->state = KEY_DOWN;
						skey->hold_time_ms = 0;
						break;
					case KEY_DOWN:
						skey->hold_time_ms += KEYSCAN_INTERVAL_TIME;
						if (skey->hold_time_ms > KEYSCAN_HOLD_TIME) {
							skey->state = KEY_HOLD;
						}
						break;
					case KEY_HOLD:
						break;
					default:
						break;
				}
			} else {
				switch (skey->state)
				{
					case KEY_RELEASE:
						break;
					case KEY_DOWN:
					case KEY_HOLD:
						skey->state = KEY_RELEASE;
						break;
					default:
						break;
				}				
			}
			// 状态改变
			if (skey->state != pre_state) {
				skey->is_changed = true;
				is_has_changed = true;
			} else {
				skey->is_changed = false;
			}
		}
		// 列[c]恢复1
		phy_gpio_write(pins_for_col[c], 1);		
		phy_gpio_pin_init(pins_for_col[c], IE);		
		_kscan_delay_us(20);
	}
	return key_cnt;
}

/**
 * @brief 按键中断处理
 * 
 */
void GPIO_IRQ_handler()
{
    // uint32 polarity = AP_GPIOA->int_polarity;
    uint32 st = AP_GPIOA->int_status;

    //clear interrupt
    AP_GPIOA->porta_eoi = st;
	AP_GPIOA->inten &= ~st;

	// LOGI("IT", "INT_STATUS=%x, polarity=%x", st, polarity);
	aos_sem_signal(&sem_keyscan_start);
}

void kscan_row_interrupt_enable()
{
	uint32_t mask;
	// 所有列线设置为低电平
	for (int c = 0; c < KSCAN_COL_NUM; c++) {
		phy_gpio_write(pins_for_col[c], 0);
	}

	for (int i = 0; i < KSCAN_ROW_NUM; i++) {
		mask = 1 << pins_for_row[i];
		AP_GPIOA->porta_eoi = mask;		// 清除未处理的中断标志
		AP_GPIOA->inttype_level |= mask;	// set row[i] interrupt edge sensitive
		AP_GPIOA->debounce |= mask;		// debounce
		if (phy_gpio_read(pins_for_row[i])) {
			AP_GPIOA->int_polarity &= mask;		// interrupt active falling-edge
		} else {
			AP_GPIOA->int_polarity |= mask;		// interrupt active rasing-edge
		}
		AP_GPIOA->inten |= mask;	// enable interrupt for row[i]
	}
}

void kscan_row_interrupt_disable()
{
	uint32_t mask;
	for (int i = 0; i < KSCAN_ROW_NUM; i++) {
		mask = 1 << pins_for_row[i];
		AP_GPIOA->inten &= ~mask;	// disable interrupt for row[i]
	}

	// 所有列线设置为高电平
	for (int c = 0; c < KSCAN_COL_NUM; c++) {
		phy_gpio_write(pins_for_col[c], 1);
	}
}

/**********************************
 * 睡眠处理
 *********************************/
static uint16_t kscan_row_sleep_state;
void kscan_prepare_sleep_action()
{
	kscan_row_interrupt_disable();

	// 配置所有列线强上拉(1)
	for (int i = 0; i < KSCAN_COL_NUM; i++) {
		// csi_gpio_pin_config_mode(kscan_cols[i], GPIO_MODE_PULLDOWN);
		phy_gpio_pull_set(pins_for_col[i], STRONG_PULL_UP);
	}
	// 配置所有行线下拉(0),并根据当前的输入状态配置唤醒边沿(上升沿/下降沿)
	kscan_row_sleep_state = 0;
	for (int i = 0; i < KSCAN_ROW_NUM; i++) {
		// csi_gpio_pin_config_mode(kscan_rows[i], GPIO_MODE_PULLUP);
		kscan_row_sleep_state = kscan_row_sleep_state << 1;
		phy_gpio_pull_set(pins_for_row[i], PULL_DOWN);
		// 根据行线上的状态(0/1)设置唤醒(上升沿/下降沿)
		if (phy_gpio_read(pins_for_row[i])) {
			kscan_row_sleep_state = kscan_row_sleep_state | 0x0001;
			phy_gpio_wakeup_set(pins_for_row[i], NEGEDGE);
		} else {
			phy_gpio_wakeup_set(pins_for_row[i], POSEDGE);
		}
	}

	// LOGI("kscan", "sleep, row = %4X", kscan_row_sleep_state);	
}

void kscan_wakeup_action()
{
	int i;
	
	// LOGI("kscan", "wakeup, row = %4X", kscan_row_sleep_state);
	// 检查是否是kscan行输入发生了改变引起的唤醒
	for (i = KSCAN_ROW_NUM; i > 0; i--) {
		if (phy_gpio_read(pins_for_row[i-1]) != (kscan_row_sleep_state & 0x0001)) {
			break;
		}
		kscan_row_sleep_state = kscan_row_sleep_state >> 1;
	}
	// 行输入发生了改变,通知键盘扫描启动
	if (i != 0) {
		kscan_gpio_col_init();
		kscan_gpio_row_init();
		aos_sem_signal(&sem_keyscan_start);
		LOGI("KEYSCAN", "keyscan wakeup");
	}
}


#ifdef DEBUG_CMD_KEYSCAN

static void cmd_keysan_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
	if (argv[1] == NULL) {
		// app_event_set(APP_EVENT_KEYSCAN);
		aos_sem_signal(&sem_keyscan_start);
	}
}

void cli_reg_cmd_keyscan()
{
	static const struct cli_command cmd_info = {
		"keyscan",
		"test key scan",
		cmd_keysan_func,
	};
	aos_cli_register_command(&cmd_info);
}

#endif

static const uint8_t kscan_one_key_map[14][2] = {
	//scan code, key code
	{ 1,  VK_KEY_01 },
	{ 2,  VK_KEY_02 },
	{ 3,  VK_KEY_03 },
	{ 4,  VK_KEY_04 },
	{ 5,  VK_KEY_05 },
	{ 6,  VK_KEY_06 },
	{ 7,  VK_KEY_07 },
	{ 8,  VK_KEY_08 },
	{ 9,  VK_KEY_09 },
	{ 10, VK_KEY_10 },
	{ 11, VK_KEY_11 },
	{ 12, VK_KEY_12 },
	{ 13, VK_KEY_13 },
	{ 0, 0}
};

static const uint8_t kscan_hold_key_map[2][2] = {
	//scan code, key code
	{ 1, VK_KEY_FUNC1 },
	{ 0, 0 }
};

static const uint8_t kscan_combin_key_map[5][3] = {
	//scan code1, scan code2, key code
	{ 5,	3, 		VK_KEY_FUNC5},	
	{ 5,	2, 		VK_KEY_FUNC6},
	{ 0, 	0,		0},
};

kscan_key_t get_one_key(uint8_t code)
{
	int i = 0;
	while (kscan_one_key_map[i][1] != 0) {
		if (kscan_one_key_map[i][0] == code) {
			return kscan_one_key_map[i][1];
		}
		i++;
	}
	return 0;
}

kscan_key_t get_combin_key(uint8_t code1, uint8_t code2)
{
	int i = 0;
	uint8_t temp;
	
	// 降序排列
	if (code1 < code2) {
		temp = code2;
		code2 = code1;
		code1 = temp;
	}
	// 寻找匹配的VK
	while (kscan_combin_key_map[i][2] != 0) {
		if (kscan_combin_key_map[i][0] == code1 && kscan_combin_key_map[i][1] == code2 ) {
			return kscan_combin_key_map[i][2];
		}
		i++;
	}
	return 0;
}

kscan_key_t get_hold_key(uint8_t code) 
{
	int i = 0;
	while (kscan_hold_key_map[i][1] != 0) {
		if (kscan_hold_key_map[i][0] == code) {
			return kscan_hold_key_map[i][1];
		}
		i++;
	}
	return 0;	
}

static aos_timer_t kscan_timer = {0};
void kscan_timer_callback(void *arg1, void *arg2)
{
	aos_sem_signal(&sem_keyscan_start);
}

// extern uint64_t csi_kernel_get_ticks(void);
// extern k_status_t csi_kernel_delay_ms(uint32_t ms);
/**
 * @brief keyscan主线程
 * 
 * @param args 
 */
void keyscan_task(void * args)
{
	kscan_gpio_row_init();
	kscan_gpio_col_init();

	for (int c = 0; c < KSCAN_COL_NUM; c++) {
		for (int r = 0; r < KSCAN_ROW_NUM; r++) {
			keys[c][r].state = KEY_RELEASE;
			keys[c][r].is_changed = false;
			keys[c][r].hold_time_ms = 0;
			keys[c][r].code = 0;
		}
	}

	drv_irq_register(GPIO_IRQ, GPIO_IRQ_handler);
	kscan_row_interrupt_enable();
	drv_irq_enable(GPIO_IRQ);

	#ifdef DEBUG_CMD_KEYSCAN
	cli_reg_cmd_keyscan();
	#endif
	
	LOGI("KEYSCAN", "key scan start");

	bool is_vk_pressed = false;
	
	while(1) {
		// 按键状态发生改变,睡眠状态唤醒/发生中断
		aos_sem_wait(&sem_keyscan_start, AOS_WAIT_FOREVER);

		// 禁止睡眠
		disableSleepInPM(0x02);

		// 处理按键时,禁止中断
		kscan_row_interrupt_disable();	
		// aos_timer_start(&kscan_timer);

		// uint64_t t1 = csi_kernel_get_ticks();
		// LOGI("KEYSCAN", "start with:%d", t1);
		app_msg_t msg;
		msg.type = MSG_KEYSCAN;
		msg.subtype = MSG_KEYSCAN_KEY_RELEASE_ALL;
		msg.param = VK_KEY_NULL;
		while (1) {
			// 扫描按键,获取按键值
			uint8_t key_num = kscan_read_keycode();
			// LOGI("KEYSCAN", "kscan_read_keycode:%d,code1=%d,code2=%d", key_num, keys_pressed[0]->code, keys_pressed[1]->code);

			// 有键按下
			if (key_num != 0) {	
				// 单键按下
				if (key_num == 1) {		
					if(keys_pressed[0]->is_changed) {
						if (keys_pressed[0]->state == KEY_DOWN) {		// 单键按下
							msg.subtype = MSG_KEYSCAN_KEY_PRESSED;
							msg.param = get_one_key(keys_pressed[0]->code);
						}
						if (keys_pressed[0]->state == KEY_HOLD) {		// 单键长按
							msg.subtype = MSG_KEYSCAN_KEY_PRESSED;
							msg.param = get_hold_key(keys_pressed[0]->code);	
						}
						// 发送系统消息
						if (msg.param != 0) {
							if (app_send_message(&msg) == false) {
								LOGE("KEYSCAN", "send message failed");
							}
							is_vk_pressed = true;
						}	
						// LOGI("KEYSCAN", "vk: 1 key pressed:%2X, scan code = %d", msg.param, keys_pressed[0]->code);
					}
				}
				// 双键按下
				if (key_num == 2) {				
					if (keys_pressed[0]->is_changed || keys_pressed[1]->is_changed) {
						// 只处理按键按下事件
						if (keys_pressed[0]->state == KEY_DOWN || keys_pressed[1]->state == KEY_DOWN) {
							msg.subtype = MSG_KEYSCAN_KEY_PRESSED;
							msg.param = get_combin_key(keys_pressed[0]->code, keys_pressed[1]->code);
						}	
						// 发送系统消息
						if (msg.param != 0) {
							if (app_send_message(&msg) == false) {
								LOGE("KEYSCAN", "send message failed");
							}
							is_vk_pressed = true;
						}					
						// LOGI("KEYSCAN", "vk: 2 key pressed:%2X, scan code = %d,%d", msg.param, keys_pressed[0]->code, keys_pressed[1]->code);
					}
				}
				// 3键及更多按键按下
				if (key_num > 2) {
					continue;
				}						
			}
			// 按键松开
			else {
				if (is_vk_pressed) {
					msg.subtype = MSG_KEYSCAN_KEY_RELEASE_ALL;
					if (app_send_message(&msg)) {
						is_vk_pressed = false;
						break;	// 退出循环
					} else {
						// ！！！！按键释放未发送成功，需要再次发送
						LOGE("KEYSCAN", "send message failed");
					}
				}
			}
			// aos_sem_wait(&sem_keyscan_start, AOS_WAIT_FOREVER);
			// // 按键超时, 任意按键按下超过5S
			// if ((csi_kernel_get_ticks() - t1) > 5000) {
			// 	break;
			// }

			// 延时
			csi_kernel_delay_ms(50);
		}

		// 使能中断,以启动键盘扫描
		// csi_kernel_delay_ms(50); 	// 消除按键松开抖动
		kscan_row_interrupt_enable();
		enableSleepInPM(0x02);
	}
}

static uint32_t keyscan_stack[2048/4];
static aos_task_t hTaskKeyscan = {0};
int create_keyscan_task()
{
    int s = 0;
	// s = aos_task_new("keyscan task", keyscan_task, NULL, 1024);
	s += aos_task_new_ext(&hTaskKeyscan, "keyscan", keyscan_task, NULL, keyscan_stack, 2048, AOS_DEFAULT_APP_PRI);
	s += aos_sem_new(&sem_keyscan_start, 1);
	s += aos_timer_new_ext(&kscan_timer, kscan_timer_callback, NULL, 50, 1, 0);
	// s += aos_timer_new_ext(&timer_keyscan, kscan_period_scan, NULL, 50, 1, 1);

    // s = krhino_task_create(&hTaskKeyscan, "keyscan task", NULL,
    //                    AOS_DEFAULT_APP_PRI, 0, (cpu_stack_t *)keyscan_stack,
    //                    2048, keyscan_task, 1);
    if (s == RHINO_SUCCESS) {
        LOGI("main", "keyscan start successed");
		return 0;
    } else
    {
        LOGI("main", "keyscan start failed");
		return -1;
    }
}