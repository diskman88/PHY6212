#include <yoc_config.h>

#include <stdbool.h>
#include <aos/aos.h>
#include <yoc/yoc.h>
#include <pinmux.h>
#include <mm.h>
#include <umm_heap.h>
#include <common.h>
#include <pwrmgr.h>
#include <yoc/init.h>
#include <pm.h>

#include "msg.h"
#include "event.h"

gpio_pin_handle_t GPIO14 = NULL;
gpio_pin_handle_t GPIO15 = NULL;
gpio_pin_handle_t LED_Y = NULL;
gpio_pin_handle_t LED_R = NULL;
gpio_pin_handle_t LED_G = NULL;

/**
 * @brief CB6212板载LED初始化
 * 
 */
void board_leds_init()
{
    drv_pinmux_config(P23, PIN_FUNC_GPIO);
    drv_pinmux_config(P31, PIN_FUNC_GPIO);
    drv_pinmux_config(P32, PIN_FUNC_GPIO);

    LED_Y = csi_gpio_pin_initialize(P23, NULL);
    csi_gpio_pin_config(LED_Y, GPIO_MODE_PUSH_PULL, GPIO_DIRECTION_OUTPUT);
    LED_R = csi_gpio_pin_initialize(P31, NULL);
    csi_gpio_pin_config(LED_R, GPIO_MODE_PUSH_PULL, GPIO_DIRECTION_OUTPUT);
    LED_G = csi_gpio_pin_initialize(P32, NULL);
    csi_gpio_pin_config(LED_G, GPIO_MODE_PUSH_PULL, GPIO_DIRECTION_OUTPUT);
}

void board_led_set(uint8_t led, bool new_state)
{
    if (led == 0) csi_gpio_pin_write(LED_R, new_state);
    if (led == 1) csi_gpio_pin_write(LED_G, new_state);
    if (led == 2) csi_gpio_pin_write(LED_Y, new_state);
}


static void p15_irq_handler(int32 idx)
{
    csi_gpio_pin_set_irq(GPIO15, GPIO_IRQ_MODE_LOW_LEVEL, false);
    io_msg_t msg;
    msg.type = MSG_BUTTON;
    msg.event = 0;
    msg.param = 15;
    if (io_send_message(&msg) == 0) {
        app_event_set(APP_EVENT_IO);
    }
}

static void p14_irq_handler(int32 idx)
{
    csi_gpio_pin_set_irq(GPIO14, GPIO_IRQ_MODE_LOW_LEVEL, false);
    io_msg_t msg;
    msg.type = MSG_BUTTON;
    msg.event = 1;
    msg.param = 14;
    if (io_send_message(&msg) == 0) {
        app_event_set(APP_EVENT_IO);
    }
}

static uint32_t porta_state;
static uint32_t portb_state;
void user_prepare_sleep_action()
{
    porta_state = *reg_gpio_ext_porta;
    portb_state = *reg_gpio_ext_portb;
}

void keys_wakeup_action()
{
    // 睡眠期间外部IO发生了改变,通知应用处理IO事件
    io_msg_t msg;
    msg.type = MSG_BUTTON;
    if (phy_gpio_read(P14) == 0) {
        msg.event = 14;
    }
    if (phy_gpio_read(P15) == 0) {
        msg.event = 15;
    }
    msg.param = 0;
    if (io_send_message(&msg) == 0) {
        app_event_set(APP_EVENT_IO);
    }
}

/**
 * @brief CB6121板载按键初始化
 * 
 */
void board_keys_init()
{
    int32_t ret;

    // IOMUX: connect PAD to GPIO
    phy_gpio_pull_set(P14, WEAK_PULL_UP);
    phy_gpio_pull_set(P15, WEAK_PULL_UP);
    drv_pinmux_config(P14, PIN_FUNC_GPIO);
    drv_pinmux_config(P15, PIN_FUNC_GPIO);

    GPIO14 = csi_gpio_pin_initialize(P14, (gpio_event_cb_t)p14_irq_handler);
    // csi_gpio_pin_config(GPIO14, GPIO_MODE_PULLUP, GPIO_DIRECTION_INPUT);
    csi_gpio_pin_config_mode(GPIO14, GPIO_MODE_PULLUP);
    csi_gpio_pin_config_direction(GPIO14, GPIO_DIRECTION_INPUT);
    ret = csi_gpio_pin_set_irq(GPIO14, GPIO_IRQ_MODE_LOW_LEVEL, true);
    if (ret != 0) {
        LOGE("BOARD", "gpio irq setting failed:%d", ret);
    }

    GPIO15 = csi_gpio_pin_initialize(P15, (gpio_event_cb_t)p15_irq_handler);
    // csi_gpio_pin_config(GPIO15, GPIO_MODE_PULLUP, GPIO_DIRECTION_INPUT);
    csi_gpio_pin_config_mode(GPIO15, GPIO_MODE_PULLUP);
    csi_gpio_pin_config_direction(GPIO15, GPIO_DIRECTION_INPUT);
    ret = csi_gpio_pin_set_irq(GPIO15, GPIO_IRQ_MODE_LOW_LEVEL, true);
    if (ret != 0) {
        LOGE("BOARD", "gpio irq setting failed:%d", ret);
    }

    LOGI("BOARD", "keys iniit sucesses");
}


void board_sys_init()
{
    if (io_message_queue_init() != 0) {
        LOGE("BOARD", "can`t init message queue");
    }

    if (app_event_init() != 0) {
        LOGE("BOARD", "can`t init application event");
    }
}