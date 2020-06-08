#include <yoc_config.h>
#include <stdbool.h>
#include <aos/aos.h>
#include <yoc/yoc.h>
#include <pinmux.h>
#include <mm.h>
#include <umm_heap.h>
#include <common.h>
#include <pwrmgr.h>
#include <aos/log.h>
#include <gpio.h>
#include <pm.h>
#include <io.h>
#include <yoc/atserver.h>
#include "at_ble.h"
#include "uart_server.h"
#include "uart_client.h"
#include "gpio_usart.h"
#include "ll_sleep.h"
#include "module_board_config.h"
#include "drv_pmu.h"


#define TAG "INIT"

//for board

#define AT_TASK_MSG_QUEUE_COUNT  QUEUE_MSG_COUNT

#define SLEEP_FLAG 0XFF

typedef uint8 llStatus_t;

extern uart_handle_t g_uart_handler;
gpio_pin_handle_t g_mode_ctrl, g_sleep_ctrl, g_connect_info, g_sleep_info;

uint8_t at_server_mode;
void board_cli_init(utask_t *task);

static utask_t cli_task;
static uint8_t cli_task_stack[2 * 1024];

UTASK_QUEUE_BUF_DEFINE(cli_task_queue, AT_TASK_MSG_QUEUE_COUNT);

at_server_handler *g_at_ble_handler = NULL;

static uint32_t mm_heap[10 * 1024 / 4];

static void mm_init()
{
    mm_initialize(&g_mmheap, mm_heap, sizeof(mm_heap));
}

extern void registers_save(uint32_t *mem, uint32_t *addr, int size);
static uint32_t usart_regs_saved[5];

static int at_mode_pin_init(pin_name_e pin);

#ifdef CONFIG_GPIO_UART
static void gpio_uart_init(void)
{
    drv_gpio_usart_init(LOG_PIN);
    drv_gpio_usart_config(38400, USART_MODE_SINGLE_WIRE, USART_PARITY_NONE, USART_STOP_BITS_1, USART_DATA_BITS_8);
}
#endif

static void usart_prepare_sleep_action(void)
{

    uint32_t addr = 0x40004000;
    uint32_t read_num = 0;

    while (*(volatile uint32_t *)(addr + 0x14) & 0x1) {
        *(volatile uint32_t *)addr;

        if (read_num++ >= 100) {
            break;
        }
    }

    uint32_t timeout = 0;

    while ((*(volatile uint32_t *)(addr + 0x7c) & 0x1) && (timeout++ < 1000000)) {
        *(volatile uint32_t *)addr;
    };

    *(volatile uint32_t *)(addr + 0xc) |= 0x80;

    registers_save((uint32_t *)usart_regs_saved, (uint32_t *)addr, 2);

    *(volatile uint32_t *)(addr + 0xc) &= ~0x80;

    registers_save(&usart_regs_saved[2], (uint32_t *)addr + 1, 1);

    registers_save(&usart_regs_saved[3], (uint32_t *)addr + 3, 2);
}

static void usart_wakeup_action(void)
{

    uint32_t addr = 0x40004000;

    uint32_t timeout = 0;

    while ((*(volatile uint32_t *)(addr + 0x7c) & 0x1) && (timeout++ < 1000000));

    *(volatile uint32_t *)(addr + 0xc) |= 0x80;
    registers_save((uint32_t *)addr, usart_regs_saved, 2);
    *(volatile uint32_t *)(addr + 0xc) &= ~0x80;
    registers_save((uint32_t *)addr + 1, &usart_regs_saved[2], 1);
    registers_save((uint32_t *)addr + 3, &usart_regs_saved[3], 2);
    *(volatile uint32_t *)(addr + 0x8) |= 0x41;
}

int pm_prepare_sleep_action()
{
    hal_ret_sram_enable(RET_SRAM0 | RET_SRAM1 | RET_SRAM2 | RET_SRAM3 | RET_SRAM4);
    usart_prepare_sleep_action();
    return 0;
}

static void sleep_mode_ctrl(int32_t idx)
{
    phy_gpio_wakeup_set(AT_SLEEP_CTRL_PIN, POSEDGE);

    if (g_at_ble_handler->at_config->sleep_mode == SLEEP) {
        enableSleepInPM(SLEEP_FLAG);
        csi_gpio_pin_config_mode(g_connect_info, GPIO_MODE_PULLUP);
    } else {
        csi_pmu_enter_sleep(NULL, PMU_MODE_STANDBY);
    }
}

static int set_sleep_pin_interrupt_mode()
{
    int ret;
    drv_pinmux_config(AT_SLEEP_CTRL_PIN, PIN_FUNC_GPIO);
    g_sleep_ctrl = csi_gpio_pin_initialize(AT_SLEEP_CTRL_PIN, sleep_mode_ctrl);
    ret = csi_gpio_pin_config_mode(g_sleep_ctrl, GPIO_MODE_PULLDOWN);

    if (ret != 0) {
        return -1;
    }

    ret = csi_gpio_pin_config_direction(g_sleep_ctrl, GPIO_DIRECTION_INPUT);

    if (ret != 0) {
        return -1;
    }

    ret = csi_gpio_pin_set_irq(g_sleep_ctrl, GPIO_IRQ_MODE_FALLING_EDGE, 1);

    if (ret != 0) {
        return -1;
    }

    return 0;

}


int pm_after_sleep_action()
{
    uint8_t at_mode = AT_MODE;
    drv_pinmux_config(P9, UART_TX);
    drv_pinmux_config(P10, UART_RX);
    usart_wakeup_action();

#ifdef CONFIG_GPIO_UART
    gpio_uart_init();
#endif

    if (phy_gpio_read(AT_SLEEP_CTRL_PIN)) {
        disableSleepInPM(SLEEP_FLAG);
        set_sleep_pin_interrupt_mode();
    }

    phy_gpio_write(AT_SLEEP_PIN, 1);

    at_mode_pin_init(AT_MODE_CTRL_PIN);

    if (phy_gpio_read(AT_MODE_CTRL_PIN)) {
        at_mode = AT_MODE;
    } else {
        at_mode = UART_MODE;
    }

    if (g_at_ble_handler->at_config->role == SLAVE) {
        ble_uart_server_t  *server_handler = (ble_uart_server_t *)g_uart_handler;

        if (server_handler) {
            if (server_handler->conn_handle < 0) {
                at_mode = AT_MODE;
            }
        } else {
            at_mode = AT_MODE;
        }

    } else {
        ble_uart_client_t  *client_handler = (ble_uart_client_t *)g_uart_handler;

        if (client_handler) {
            if (client_handler->conn_handle < 0) {
                at_mode = AT_MODE;
            }
        } else {
            at_mode = AT_MODE;
        }
    }

    if (at_mode == at_server_mode) {
        return 0;
    }

    at_server_mode = at_mode;

    if (at_server_mode == UART_MODE) {
        return atserver_passthrough_cb_register(at_ble_uartmode_recv);
    } else {
        return atserver_exit_passthrough();
    }

    return 0;
}

static void at_mode_ctrl(int32_t idx)
{
    uint8_t at_mode_pin;
    uint8_t at_mode = AT_MODE;
    int ret;

    if (!g_uart_handler) {
        return;
    }

    csi_gpio_pin_read(g_mode_ctrl, (bool *)&at_mode_pin);

    if (at_mode_pin) {
        at_mode = AT_MODE;
        ret = csi_gpio_pin_set_irq(g_mode_ctrl, GPIO_IRQ_MODE_LOW_LEVEL, 1);

        if (ret != 0) {
            return;
        }

    } else {
        at_mode = UART_MODE;
        ret = csi_gpio_pin_set_irq(g_mode_ctrl, GPIO_IRQ_MODE_HIGH_LEVEL, 1);

        if (ret != 0) {
            return;
        }
    }

    if (g_at_ble_handler->at_config->role == SLAVE) {
        ble_uart_server_t  *server_handler = (ble_uart_server_t *)g_uart_handler;

        if (server_handler) {
            if (server_handler->conn_handle < 0) {
                at_mode = AT_MODE;
            }
        } else {
            at_mode = AT_MODE;
        }

    } else {
        ble_uart_client_t  *client_handler = (ble_uart_client_t *)g_uart_handler;

        if (client_handler) {
            if (client_handler->conn_handle < 0) {
                at_mode = AT_MODE;
            }
        } else {
            at_mode = AT_MODE;
        }
    }

    if (at_mode == at_server_mode) {
        return;
    }

    at_server_mode = at_mode;

    if (at_server_mode == UART_MODE) {
        ret = atserver_passthrough_cb_register(at_ble_uartmode_recv);
    } else {
        ret = atserver_exit_passthrough();
    }

    if (ret != 0) {
        return;
    }
}

static int at_mode_pin_init(pin_name_e pin)
{
    int ret;
    drv_pinmux_config(pin, PIN_FUNC_GPIO);
    g_mode_ctrl = csi_gpio_pin_initialize(pin, at_mode_ctrl);

    ret = csi_gpio_pin_config_mode(g_mode_ctrl, GPIO_MODE_PULLDOWN);

    if (ret != 0) {
        return -1;
    }

    ret = csi_gpio_pin_config_direction(g_mode_ctrl, GPIO_DIRECTION_INPUT);

    if (ret != 0) {
        return -1;
    }

    bool at_mode = 0;
    csi_gpio_pin_read(g_mode_ctrl, &at_mode);

    if (at_mode) {
        ret = csi_gpio_pin_set_irq(g_mode_ctrl, GPIO_IRQ_MODE_LOW_LEVEL, 1);
    } else {
        ret = csi_gpio_pin_set_irq(g_mode_ctrl, GPIO_IRQ_MODE_HIGH_LEVEL, 1);
    }

    if (ret != 0) {
        return -1;
    }

    return 0;
}

int at_module_board_init(at_conf_load *conf_load)
{
    if (!conf_load) {
        return -1;
    }

    int ret = 0;
    //AT_MODE_CTRL_PIN
    ret = at_mode_pin_init(AT_MODE_CTRL_PIN);

    if (ret) {
        return ret;
    }

    //AT_CONNECT_PIN
    ret = drv_pinmux_config(AT_CONNECT_PIN, PIN_FUNC_GPIO);

    if (ret != 0) {
        return -1;
    }

    g_connect_info = csi_gpio_pin_initialize(AT_CONNECT_PIN, NULL);
    ret = csi_gpio_pin_config_mode(g_connect_info, GPIO_MODE_PULLDOWN);

    if (ret < 0) {
        return -1;
    }

    ret = csi_gpio_pin_config_direction(g_connect_info, GPIO_DIRECTION_OUTPUT);

    if (ret < 0) {
        return -1;
    }

    ret = csi_gpio_pin_write(g_connect_info, 0);

    if (ret < 0) {
        return -1;
    }

    if (conf_load->sleep_mode != NO_SLEEP) {
        ret = set_sleep_pin_interrupt_mode();

        if (ret != 0) {
            return -1;
        }

        bool sleep_control;
        csi_gpio_pin_read(g_sleep_ctrl, &sleep_control);

        if (conf_load->sleep_mode == SLEEP && !sleep_control) {
            enableSleepInPM(SLEEP_FLAG);
        }

        //AT_SLEEP_PIN
        ret = drv_pinmux_config(AT_SLEEP_PIN, PIN_FUNC_GPIO);

        if (ret != 0) {
            return -1;
        }

        g_sleep_info = csi_gpio_pin_initialize(AT_SLEEP_PIN, NULL);
        ret = csi_gpio_pin_config(g_sleep_info, GPIO_MODE_PULLUP, GPIO_DIRECTION_OUTPUT);

        if (ret < 0) {
            return -1;
        }

        csi_gpio_pin_write(g_sleep_info, 1);//work when inited
    }

    return 0;

}

#ifdef CONFIG_GPIO_UART
int fputc(int ch, FILE *stream)
{
    char data;
    (void)stream;

    if (ch == '\n') {
        data = '\r';
        drv_gpio_usart_send_char(data);
    }

    data = ch;
    drv_gpio_usart_send_char(data);

    return 0;
}

int fgetc(FILE *stream)
{
    (void)stream;
    return 0;
}

int os_critical_enter(unsigned int *lock)
{
    aos_kernel_sched_suspend();

    return 0;
}

int os_critical_exit(unsigned int *lock)
{
    aos_kernel_sched_resume();

    return 0;
}

#endif




int board_yoc_init(void)
{
    int ret = 0;


#ifdef CONFIG_WDT
    extern void wdt_init(void);
    wdt_init();
#endif

    mm_init();

    /* load partition */
    ret = partition_init();

    if (ret <= 0) {
        return -1;
    }

    ret = aos_kv_init("kv");

    if (ret < 0) {
        return -1;
    }

    /*at ble module init*/
    g_at_ble_handler = at_ble_init();

    if (!g_at_ble_handler) {
        return -1;
    }

    /*uart init*/
    drv_pinmux_config(P9, UART_TX);
    drv_pinmux_config(P10, UART_RX);

#ifdef CONFIG_GPIO_UART
    gpio_uart_init();
#endif

    /* uService init */
    utask_new_ext(&cli_task, "at", cli_task_stack, 2 * 1024, cli_task_queue, AT_TASK_MSG_QUEUE_COUNT, AOS_DEFAULT_APP_PRI - 24);

    /*init at server*/
    extern void at_server_init_baud(utask_t *task, uint32_t baud);
    at_server_init_baud(&cli_task, g_at_ble_handler->at_config->baud);

    /*set tx power*/
    extern llStatus_t LL_SetTxPowerLevel(int8);
    ret = LL_SetTxPowerLevel(g_at_ble_handler->at_config->tx_power);

    if (ret) {
        return -1;
    }

    /*disable low power*/
    disableSleepInPM(SLEEP_FLAG);

    /*board config init*/
    at_module_board_init(g_at_ble_handler->at_config);

    /*ble init*/
    extern void board_ble_init(void);
    board_ble_init();


    return 0;
}



