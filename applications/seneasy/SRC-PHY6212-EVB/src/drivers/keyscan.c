
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

static key_def_t keys[KSCAN_ROW_NUM*KSCAN_COL_NUM];
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

/**
 * @brief 扫描并读取按键值
 * 
 * @param keycodes 保存按键值的列表
 * @return int 按键个数
 */
int kscan_read_keycode() 
{
	int key_cnt = 0;
	// 扫描键盘
	for (int c = 0; c < KSCAN_COL_NUM; c++) {
		// 列[c]输出0
		phy_gpio_write(pins_for_col[c], 0);	
		// 扫描行输入
		for (int r = 0; r < KSCAN_ROW_NUM; r++) {
			key_def_t *skey = &keys[c * KSCAN_ROW_NUM + r]; 
			skey->code = c * KSCAN_ROW_NUM + r;
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
			} else {
				skey->is_changed = false;
			}
		}
		// 列[c]恢复1
		phy_gpio_write(pins_for_col[c], 1);
	}
	return key_cnt;
}

/**
 * @brief 按键中断处理
 * 
 */
void GPIO_IRQ_handler()
{
    uint32 polarity = AP_GPIOA->int_polarity;
    uint32 st = AP_GPIOA->int_status;

    //clear interrupt
    AP_GPIOA->porta_eoi = st;
	AP_GPIOA->inten &= ~st;

	LOGI("IT", "INT_STATUS=%x, polarity=%x", st, polarity);
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

	drv_irq_register(GPIO_IRQ, GPIO_IRQ_handler);
	kscan_row_interrupt_enable();
	drv_irq_enable(GPIO_IRQ);

	#ifdef DEBUG_CMD_KEYSCAN
	cli_reg_cmd_keyscan();
	#endif
	
	LOGI("KEYSCAN", "key scan start");

	while(1) {
		// 按键状态发生改变,睡眠状态唤醒/发生中断
		aos_sem_wait(&sem_keyscan_start, AOS_WAIT_FOREVER);
		// 禁止睡眠
		disableSleepInPM(0x02);
		kscan_row_interrupt_disable();
		// drv_irq_disable(GPIO_IRQ)
		// 按键消息
		bool new_message = false;
		io_msg_t msg;
		// 计时器
		uint64_t t1 = csi_kernel_get_ticks();
		// LOGI("KEYSCAN", "start with:%d", t1);

		while (1) {
			// 扫描按键,获取按键值
			uint8_t key_num = kscan_read_keycode();
			// LOGI("KEYSCAN", "kscan_read_keycode:%d", key_num);
			// 有键按下
			if (key_num != 0)
			{	
				msg.type = MSG_KSCAN_DOWN;
				// 单键按下
				if (key_num == 1) {
					if(keys_pressed[0]->is_changed) {
						// 单键按下
						if (keys_pressed[0]->state == KEY_DOWN) {
							msg.event = MSG_EVENT_ONE_KEY;
							msg.param = keys_pressed[0]->code;
							new_message = true;
						}
						// 单键长按
						if (keys_pressed[0]->state == KEY_HOLD) {
							msg.event = MSG_EVENT_HOLD_KEY;
							msg.param = keys_pressed[0]->code;
							new_message = true;			
						}
					}
				}

				// 双键按下
				if (key_num == 2) {
					if (keys_pressed[0]->is_changed || keys_pressed[1]->is_changed) {
						// 只处理按键按下事件
						if (keys_pressed[0]->state == KEY_DOWN || keys_pressed[1]->state == KEY_DOWN) {
							msg.event = MSG_EVENT_COMBO_KEY;
							msg.param = 0;
							if (keys_pressed[0]->code > keys_pressed[1]->code) {
								msg.param |= keys_pressed[0]->code;
								msg.param = msg.param << 8;
								msg.param |= keys_pressed[1]->code;							
							} else
							{
								msg.param |= keys_pressed[1]->code;
								msg.param = msg.param << 8;
								msg.param |= keys_pressed[0]->code;	
							}
							new_message = true;
						}
					}
				}

				// 3键及更多按键按下
				if (key_num > 2) {
					continue;
				}

				// 发送按键消息
				if (new_message) {
					LOGI("KSCAN", "key down:%d", keys_pressed[0]->code);

					if(io_send_message(&msg) == 0) {
						if(app_event_set(APP_EVENT_KEYSCAN) == 0) {
							// LOGI("KSCAN", "key down:%d", keys_pressed[0]->code);
						}
					}
					new_message = false;
				}				
			}
			// 按键松开
			else {
				msg.type = MSG_KSCAN_RELEASE;
				msg.event = KSCAN_COL_NUM * KSCAN_ROW_NUM;
				msg.lpMsgBuff = (void *)keys;
				LOGI("KSCAN", "key releas");

				if(io_send_message(&msg) == 0) {
					if(app_event_set(APP_EVENT_KEYSCAN) == 0) {
						// LOGI("KSCAN", "key releas");
					}
				}
				break;		// 退出循环
			}

			// 按键超时, 任意按键按下超过5S
			if ((csi_kernel_get_ticks() - t1) > 5000) {
				break;
			}

			// 延时
			csi_kernel_delay_ms(50);
		}

		// 使能中断,以启动键盘扫描
		// csi_kernel_delay_ms(1000); 	// 消除按键松开抖动
		kscan_row_interrupt_enable();
		// drv_irq_enable(GPIO_IRQ);

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