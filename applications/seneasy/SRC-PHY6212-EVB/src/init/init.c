#include <yoc_config.h>

#include <stdbool.h>
#include <aos/aos.h>
#include <yoc/yoc.h>
#include <pinmux.h>
#include <mm.h>
#include <umm_heap.h>
#include <common.h>
#include <yoc/init.h>
#include <drv_gpio.h>
#include <drv_usart.h>

#define TAG  "init"

#ifndef CONSOLE_ID
#define CONSOLE_ID 0
#endif

#ifdef __DEBUG__
void at_server_init(utask_t *task);
void board_cli_init(utask_t *task);
static utask_t cli_task;
static uint8_t cli_task_stack[2 * 1024];
UTASK_QUEUE_BUF_DEFINE(cli_task_queue, QUEUE_MSG_COUNT * 2);
#endif

/*
 * 堆和栈初始化，内存分配
 * 
 */
#define INIT_TASK_STACK_SIZE 2048
static cpu_stack_t app_stack[INIT_TASK_STACK_SIZE / 2];
static uint32_t mm_heap[2 * 1024 / 4]  __attribute__((section("noretention_mem_area0")));
static void mm_init()
{
    mm_initialize(&g_mmheap, mm_heap, sizeof(mm_heap));
}

extern void board_ble_init(void);
/**
 * @brief 系统底层初始化
 * 
 */
void board_yoc_init(void)
{
    int ret = 0;

#ifdef CONFIG_WDT
    extern void wdt_init(void);
    wdt_init();
#endif

    // disable low power mode when use console
    // disableSleepInPM(1);
    // 串口
    phy_gpio_pull_set(P9, WEAK_PULL_UP);
    phy_gpio_pull_set(P10, WEAK_PULL_UP);
    drv_pinmux_config(P9, UART_TX);
    drv_pinmux_config(P10, UART_RX);
    console_init(CONSOLE_ID, 768000, 128);
    // 堆内存分配
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
#ifdef __DEBUG__
    utask_new_ext(&cli_task, "at&cli", cli_task_stack, 2 * 1024, cli_task_queue, QUEUE_MSG_COUNT * 2, AOS_DEFAULT_APP_PRI);
    board_cli_init(&cli_task);
#endif

}

/*
 * 主任务进程
 */
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