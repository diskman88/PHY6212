
#include <stdint.h>
#include <stdbool.h>
#include <drv_common.h>
#include <pinmux.h>
#include <csi_config.h>
#include <stdbool.h>
#include <stdio.h>
#include <drv_irq.h>
#include <drv_gpio.h>
#include <drv_pmu.h>
#include <dw_gpio.h>
#include <pin_name.h>
#include <clock.h>
#include <soc.h>
#include <yoc/cli.h>
#include <aos/cli.h>
#include "board/ch6121_evb/msg.h"
#include "board/ch6121_evb/event.h"

#define DEBUG_CMD_KEYSCAN		1

#define KSCAN_ROW_NUM	4
#define KSCAN_COL_NUM	4

const uint32_t pins_for_row[KSCAN_ROW_NUM] = {GPIO_P00, GPIO_P01, GPIO_P02, GPIO_P03};
const uint32_t pins_for_col[KSCAN_COL_NUM] = {GPIO_P15, GPIO_P14, GPIO_P13, GPIO_P34};

static gpio_pin_handle_t kscan_rows[KSCAN_ROW_NUM];
static gpio_pin_handle_t kscan_cols[KSCAN_COL_NUM];
static uint16_t key_matrix[KSCAN_COL_NUM];

// const uint32_t pins_for_col[KSCAN_COL_NUM] = {GPIO_P31, GPIO_P14, GPIO_P13, GPIO_P34};

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
		kscan_rows[i] = csi_gpio_pin_initialize(pins_for_row[i], NULL);
		// csi_gpio_pin_config(kscan_rows[i], GPIO_MODE_PULLDOWN, GPIO_DIRECTION_INPUT);
		csi_gpio_pin_config_mode(kscan_rows[i], GPIO_MODE_PULLUP);
		csi_gpio_pin_config_direction(kscan_rows[i], GPIO_DIRECTION_INPUT);
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
		kscan_cols[i] = csi_gpio_pin_initialize(pins_for_col[i], NULL);
		// csi_gpio_pin_config(cols[i], GPIO_MODE_PULLUP, GPIO_DIRECTION_OUTPUT);
		csi_gpio_pin_config_mode(kscan_cols[i], GPIO_MODE_PULLUP);
		csi_gpio_pin_config_direction(kscan_cols[i], GPIO_DIRECTION_OUTPUT);
		csi_gpio_pin_write(kscan_cols[i], 1);
	}
}

/**
 * @brief 读取行输入
 * 
 * @param  
 * @return uint16_t 
 */
uint16_t kscan_read_row()
{
	uint16_t r = 0;
	for (int i = 0; i < KSCAN_ROW_NUM; i++) {
		bool bit;
		csi_gpio_pin_read(kscan_rows[i], &bit);
		r = r << 1;
		if (bit == 1) {
			r = r | 0x0001;
		}
	}
	return r;
}

/**
 * @brief 设置channel列为低,其他为高
 * 
 * @param cols 
 * @param channel 
 * @return uint16_t 
 */
void kscan_set_col(uint32_t channel)
{
	for (int i = 0; i < KSCAN_COL_NUM; i++) {
		csi_gpio_pin_write(kscan_cols[i], 1);
	}
	if (channel < KSCAN_COL_NUM) {
		csi_gpio_pin_write(kscan_cols[channel], 0);
	} 
}

static uint16_t kscan_row_state = 0;

void kscan_prepare_sleep_action()
{
	// 配置所有列线下拉(0)
	for (int i = 0; i < KSCAN_COL_NUM; i++) {
		csi_gpio_pin_config_mode(kscan_cols[i], GPIO_MODE_PULLDOWN);
	}
	// 配置所有行线上拉
	for (int i = 0; i < KSCAN_ROW_NUM; i++) {
		csi_gpio_pin_config_mode(kscan_rows[i], GPIO_MODE_PULLUP);
	}
	// 根据行线上的状态(0/1)设置唤醒(上升沿/下降沿)
	kscan_row_state = 0;
	for (int i = 0; i < KSCAN_ROW_NUM; i++) {
		bool value;
		csi_gpio_pin_read(kscan_rows[i], &value);
		kscan_row_state = kscan_row_state << 1;
		kscan_row_state |= value;
		if (value) {
			phy_gpio_wakeup_set(pins_for_row[i], NEGEDGE);
		} else {
			phy_gpio_wakeup_set(pins_for_row[i], POSEDGE);
		}	
	}
	LOGI("kscan", "enter sleep, input = %4X", kscan_row_state);	
}

void kscan_wakeup_action()
{
	kscan_gpio_row_init();
	uint16_t in = kscan_read_row();
	in = in & 0x000F;
	LOGI("kscan", "wakeup, input = %4X", in);
	if (in != kscan_row_state) {
		kscan_gpio_col_init();
		app_event_set(APP_EVENT_KEYSCAN);
	}
}


#ifdef DEBUG_CMD_KEYSCAN

static void cmd_keysan_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
	if (argv[1] == NULL) {
		app_event_set(APP_EVENT_KEYSCAN);
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

/**
 * @brief keyscan主线程
 * 
 * @param args 
 */
void keyscan_task(void * args)
{
	// kscan_iomux_set();
	// kscan_config();
	kscan_gpio_row_init();
	kscan_gpio_col_init();

	#ifdef DEBUG_CMD_KEYSCAN
	cli_reg_cmd_keyscan();
	#endif
	
	LOGI("KEYSCAN", "key scan start");
	while(1) {
		uint32_t actl_flags;
		app_event_get(APP_EVENT_KEYSCAN, &actl_flags, AOS_WAIT_FOREVER);
		// actl_flags |= APP_EVENT_KEYSCAN;
		if (actl_flags & APP_EVENT_KEYSCAN) 
		{
			for (int retry = 0; retry < 10; retry++) {
				for (int c = 0; c < KSCAN_COL_NUM; c++) {
					kscan_set_col(c);
					key_matrix[c] = kscan_read_row();
				}
			}
			LOG_HEXDUMP("KEYSCAN", (char *)key_matrix, sizeof(key_matrix));
			
		}

	}
}

static uint32_t keyscan_stack[2048/4];
static aos_task_t hTaskKeyscan = {0};
int create_keyscan_task()
{
    int s = 0;
	// s = aos_task_new("keyscan task", keyscan_task, NULL, 1024);
	s = aos_task_new_ext(&hTaskKeyscan, "keyscan", keyscan_task, NULL, keyscan_stack, 2048, AOS_DEFAULT_APP_PRI);

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