/*
 * Copyright (C) 2017 C-SKY Microsystems Co., All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <yoc_config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <aos/kernel.h>
#include <devices/uart.h>
#include "drv_usart.h"

dev_t *g_console_handle = NULL;
static uint16_t g_console_buf_size = 128; 

const char *console_get_devname(void)
{
    static char console_devname[32] = {0};

    if (g_console_handle) {
        snprintf(console_devname, sizeof(console_devname), "%s%d",
                    ((driver_t *)g_console_handle->drv)->name, g_console_handle->id);
    }

    return console_devname;
}

uint16_t console_get_buffer_size(void)
{
    return g_console_buf_size;
}

void console_init(int idx, uint32_t baud, uint16_t buf_size)
{
    uart_config_t config;
    g_console_handle = uart_open_id("uart", idx);

    if (g_console_handle != NULL) {
        uart_config_default(&config);
        config.baud_rate = baud;
        uart_config(g_console_handle, &config);
        uart_set_type(g_console_handle, UART_TYPE_CONSOLE);

        g_console_buf_size = buf_size;
    }
}
extern usart_handle_t dev_get_handler(dev_t *dev);
__attribute__ ((weak)) int fputc(int ch, FILE *stream)

{
    int data;
    if ( g_console_handle== NULL) {
        return -1;
    }

    if (ch == '\n') {
        data = '\r';
        uart_send(g_console_handle, &data, 1);
    }

    uart_send(g_console_handle, &ch, 1);

    return 0;
}

__attribute__ ((weak)) int fgetc(FILE *stream)
{
    if (g_console_handle != NULL) {
        char ch;
        uart_recv(g_console_handle, &ch, 1, -1);

        return ch;
    }

    return 0;
}

__attribute__ ((weak)) int os_critical_enter(unsigned int *lock)
{
    aos_kernel_sched_suspend();

    return 0;
}

__attribute__ ((weak)) int os_critical_exit(unsigned int *lock)
{
    aos_kernel_sched_resume();

    return 0;
}
