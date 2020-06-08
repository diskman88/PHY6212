#include <yoc_config.h>

#include <stdbool.h>
#include <aos/aos.h>
#include <yoc/yoc.h>
#include <gpio.h>
#include <aos/log.h>
#include <pinmux.h>
#include <mm.h>
#include <umm_heap.h>
#include <common.h>
#include <pwrmgr.h>
#include <drv_irq.h>
#include <drv_usart.h>
#include <ble_dut_test.h>

#define TAG "INIT"
#define CONSOLE_ID 0

/*
    Disable LPM according to bit value of flag
*/
extern void disableSleepInPM(uint8_t flag);


extern void registers_save(uint32_t *mem, uint32_t *addr, int size);
static uint32_t usart_regs_saved[5];
static uint32_t retention_config = RET_SRAM0 | RET_SRAM2;

static void usart_prepare_sleep_action(void)
{
    uint32_t addr = 0x40004000;
    uint32_t read_num = 0;

    while (*(volatile uint32_t *)(addr + 0x14) & 0x1) {
        *(volatile uint32_t *)addr;

        if (read_num++ >= 16) {
            break;
        }
    }

    while (*(volatile uint32_t *)(addr + 0x7c) & 0x1);

    *(volatile uint32_t *)(addr + 0xc) |= 0x80;
    registers_save((uint32_t *)usart_regs_saved, (uint32_t *)addr, 2);
    *(volatile uint32_t *)(addr + 0xc) &= ~0x80;
    registers_save(&usart_regs_saved[2], (uint32_t *)addr + 1, 1);
    registers_save(&usart_regs_saved[3], (uint32_t *)addr + 3, 2);
}

static void usart_wakeup_action(void)
{
    uint32_t addr = 0x40004000;

    while (*(volatile uint32_t *)(addr + 0x7c) & 0x1);

    *(volatile uint32_t *)(addr + 0xc) |= 0x80;
    registers_save((uint32_t *)addr, usart_regs_saved, 2);
    *(volatile uint32_t *)(addr + 0xc) &= ~0x80;
    registers_save((uint32_t *)addr + 1, &usart_regs_saved[2], 1);
    registers_save((uint32_t *)addr + 3, &usart_regs_saved[3], 2);
}

int pm_retention_config(uint32_t retention_area)
{
    retention_config = retention_area;
    return 0;
}

int pm_prepare_sleep_action()
{
#ifdef CONFIG_TEST_MODE

    if (get_dut_test_flag() == 1) {
        hal_ret_sram_enable(retention_config | RET_SRAM3 | RET_SRAM4);
    } else {
        hal_ret_sram_enable(retention_config);
    }

#else
    hal_ret_sram_enable(retention_config);
#endif
    usart_prepare_sleep_action();
    return 0;
}

int pm_after_sleep_action()
{
    drv_pinmux_config(P9, UART_TX);
    drv_pinmux_config(P10, UART_RX);
    usart_wakeup_action();
    return 0;
}

//void at_server_init(utask_t *task);
void board_cli_init(utask_t *task);

usart_handle_t console_handle;
int fputc(int ch, FILE *stream)
{
    int data;

    if (console_handle == NULL) {
        return -1;
    }

    if (ch == '\n') {
        data = '\r';
        csi_usart_putchar(console_handle, data);
    }

    csi_usart_putchar(console_handle, ch);

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

void board_yoc_init(void)
{
    drv_pinmux_config(P9, UART_TX);
    drv_pinmux_config(P10, UART_RX);

    console_handle = csi_usart_initialize(0, NULL);
    csi_usart_config(console_handle, 115200, USART_MODE_ASYNCHRONOUS, USART_PARITY_NONE, USART_STOP_BITS_1, USART_DATA_BITS_8);

    LOGV(TAG, "Build:%s,%s\n", __DATE__, __TIME__);

    /* Disbale LPM if need enter DUT mode */
    disableSleepInPM(0xFF);

    /* Enter DUT mode */
    ble_dut_start();
}
