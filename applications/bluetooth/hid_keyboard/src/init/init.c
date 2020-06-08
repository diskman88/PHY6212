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

void at_server_init(utask_t *task);
void board_cli_init(utask_t *task);

#define TAG  "init"

#ifndef CONSOLE_ID
#define CONSOLE_ID 0
#endif

static utask_t cli_task;
static uint8_t cli_task_stack[2 * 1024];
UTASK_QUEUE_BUF_DEFINE(cli_task_queue, QUEUE_MSG_COUNT * 2);

#define INIT_TASK_STACK_SIZE 2048
static cpu_stack_t app_stack[INIT_TASK_STACK_SIZE / 2];

static uint32_t mm_heap[2 * 1024 / 4]  __attribute__((section("noretention_mem_area0")));
static void mm_init()
{
    mm_initialize(&g_mmheap, mm_heap, sizeof(mm_heap));
}

extern void registers_save(uint32_t *mem, uint32_t *addr, int size);
static uint32_t usart_regs_saved[5];

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
    *(volatile uint32_t *)(addr + 0x8) |= 0x41;
}

int pm_prepare_sleep_action()
{
    hal_ret_sram_enable(RET_SRAM0 | RET_SRAM1 | RET_SRAM2);

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

extern void app_main();
static void application_task_entry(void *arg)
{
    app_main();
}

int main(void)
{
    /* kernel init */
    aos_init();
    ktask_t app_task_handle = {0};
    /* init task */
    krhino_task_create(&app_task_handle, "app_task", NULL,
                       AOS_DEFAULT_APP_PRI, 0, app_stack,
                       sizeof(app_stack) / 4, application_task_entry, 1);

    /* kernel start */
    aos_start();

    return 0;
}

void board_yoc_init(void)
{
    int ret = 0;

#ifdef CONFIG_WDT
    extern void wdt_init(void);
    wdt_init();
#endif

    /* disable low power mode when use console */
    // disableSleepInPM(1);
    drv_pinmux_config(P9, UART_TX);
    drv_pinmux_config(P10, UART_RX);

    console_init(CONSOLE_ID, 115200, 128);

    mm_init();

    LOGI(TAG, "Build:%s,%s", __DATE__, __TIME__);

    /* load partition */
    ret = partition_init();

    if (ret <= 0) {
        LOGE(TAG, "partition init failed");
    } else {
        LOGI(TAG, "find %d partitions", ret);
    }

    ret = aos_kv_init("kv");

    if (ret < 0) {
        LOGE(TAG, "KV init failed - %d", ret);
    }

    extern void board_ble_init(void);
    board_ble_init();

    /* uService init */
    utask_new_ext(&cli_task, "at&cli", cli_task_stack, 2 * 1024, cli_task_queue, QUEUE_MSG_COUNT * 2, AOS_DEFAULT_APP_PRI);

    board_cli_init(&cli_task);
}
