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
#include <drv_gpio.h>
#include <drv_usart.h>


void at_server_init(utask_t *task);
void board_cli_init(utask_t *task);

extern void board_keys_init(void);
extern void board_leds_init(void);
extern void board_sys_init(void);

#define TAG  "init"

#ifndef CONSOLE_ID
#define CONSOLE_ID 0
#endif

static utask_t cli_task;
static uint8_t cli_task_stack[2 * 1024];
UTASK_QUEUE_BUF_DEFINE(cli_task_queue, QUEUE_MSG_COUNT * 2);

#define INIT_TASK_STACK_SIZE 2048
static cpu_stack_t app_stack[INIT_TASK_STACK_SIZE / 2];
/*
 * 堆栈初始化
 * 
 */
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
    // 清空 Receive FIFO
    while (*(volatile uint32_t *)(addr + 0x14) & 0x1) {
        *(volatile uint32_t *)addr;

        if (read_num++ >= 16) {
            break;
        }
    }
    // 等待串口空闲
    while (*(volatile uint32_t *)(addr + 0x7c) & 0x1);
    /**
     * @brief !!! PHY6212 串口的发送寄存器,接收寄存器, 中断寄存器,分频寄存器地址复用,需要特殊读写序列..
     * 
     */
    // 允许读写波特率分频寄存器
    *(volatile uint32_t *)(addr + 0xc) |= 0x80;
    // 保存波特率分频寄存器
    registers_save((uint32_t *)usart_regs_saved, (uint32_t *)addr, 2);
    // 禁止读写波特率分频寄存器(地址映射为读写和中断寄存器)
    *(volatile uint32_t *)(addr + 0xc) &= ~0x80;
    // 保存中断使能
    registers_save(&usart_regs_saved[2], (uint32_t *)addr + 1, 1);
    registers_save(&usart_regs_saved[3], (uint32_t *)addr + 3, 2);
}

static void usart_wakeup_action(void)
{
    drv_pinmux_config(P9, UART_TX);
    drv_pinmux_config(P10, UART_RX);

    uint32_t addr = 0x40004000;

    while (*(volatile uint32_t *)(addr + 0x7c) & 0x1);

    *(volatile uint32_t *)(addr + 0xc) |= 0x80;
    registers_save((uint32_t *)addr, usart_regs_saved, 2);
    *(volatile uint32_t *)(addr + 0xc) &= ~0x80;
    registers_save((uint32_t *)addr + 1, &usart_regs_saved[2], 1);
    registers_save((uint32_t *)addr + 3, &usart_regs_saved[3], 2);
}

/*
 * 准备进入休眠
 */
int pm_prepare_sleep_action()
{
    hal_ret_sram_enable(RET_SRAM0 | RET_SRAM1 | RET_SRAM2);
    usart_prepare_sleep_action();
    csi_pinmux_prepare_sleep_action();
    return 0;
}
/* 
 * 休眠唤醒
 */
int pm_after_sleep_action()
{
    csi_pinmux_wakeup_sleep_action();
    drv_pinmux_config(P9, UART_TX);
    drv_pinmux_config(P10, UART_RX);
    usart_wakeup_action();
    return 0;
}

/*
 * 主任务进程
 */
extern void board_yoc_init(void);
extern void board_ble_init(void);
extern void app_main();

static void application_task_entry(void *arg)
{
    app_main();
}


/*
 * C函数入口
 */
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
    disableSleepInPM(1);
    

    phy_gpio_pull_set(P9, WEAK_PULL_UP);
    phy_gpio_pull_set(P10, WEAK_PULL_UP);
    drv_pinmux_config(P9, UART_TX);
    drv_pinmux_config(P10, UART_RX);

    // console_init(CONSOLE_ID, 115200, 128);
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

    aos_kv_init("kv");

    board_ble_init();
    
    /* uService init */
    utask_new_ext(&cli_task, "at&cli", cli_task_stack, 2 * 1024, cli_task_queue, QUEUE_MSG_COUNT * 2, AOS_DEFAULT_APP_PRI);
    board_cli_init(&cli_task);
}
