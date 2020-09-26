/*
 * Copyright (C) 2017 C-SKY Microsystems Co., Ltd. All rights reserved.
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
#ifndef __DUT_UART_DRIVER_H_
#define __DUT_UART_DRIVER_H_
#include "drv_usart.h"

typedef struct {
    int idx;
    uint32_t                baud_rate;
    usart_data_bits_e       data_width;
    usart_parity_e          parity;
    usart_stop_bits_e       stop_bits;
} dut_uart_cfg_t;

int dut_uart_init(dut_uart_cfg_t *uart_cfg);
void dut_uart_putchar(unsigned char data);
int dut_at_send(const char *command, ...);

#endif
